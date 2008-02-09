/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin, Devin Papineau

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Functions and structures that allocate or manage generic memory. */

#include "c_shared.h"

/******************************************************************************\
 Initialize an array.
\******************************************************************************/
void C_array_init_real(c_array_t *ary, size_t item_size, size_t cap)
{
        ary->item_size = item_size;
        ary->len = 0;
        ary->capacity = cap;
        ary->elems = C_malloc(cap * item_size);
}

/******************************************************************************\
 Ensure that enough space is allocated for n elems, but not necessarily more.
 Returns TRUE on success.
\******************************************************************************/
void C_array_reserve(c_array_t *ary, size_t n)
{
        ary->elems = C_realloc(ary->elems, ary->item_size * n);
        ary->capacity = n;
}

/******************************************************************************\
 Append something to an array. item points to something of size item_size.
 Returns TRUE on success.
\******************************************************************************/
void C_array_append(c_array_t *ary, void *item)
{
        if (ary->len >= ary->capacity) {
                if (ary->len > ary->capacity)
                        C_error("Invalid array");
                C_array_reserve(ary, ary->capacity * 2);
        }
        memcpy((char *)ary->elems + ary->len * ary->item_size,
               item, ary->item_size);
        ary->len++;
}

/******************************************************************************\
 Realloc so the array isn't overallocated, and return the pointer to the
 dynamic memory, otherwise cleaning up.
\******************************************************************************/
void *C_array_steal(c_array_t *ary)
{
        void *result;

        result = C_realloc(ary->elems, ary->len * ary->item_size);
        memset(ary, 0, sizeof (*ary));
        return result;
}

/****************************************************************************** \
 Clean up after the array.
\******************************************************************************/
void C_array_cleanup(c_array_t *ary)
{
        C_free(ary->elems);
        memset(ary, 0, sizeof (*ary));
}

/******************************************************************************\
 Reallocate [ptr] to [size] bytes large. Abort on error.
   TODO: String all the allocated chunks into a linked list and complain
         about memory leaks when the program closes
\******************************************************************************/
void *C_realloc_full(const char *file, int line, const char *function,
                     void *ptr, size_t size)
{
        void *result;

        result = realloc(ptr, size);
        if (!result)
                C_error("Out of memory, %s() (%s:%d) tried to allocate %d "
                        "bytes", function, file, line, size );
        return result;
}

/******************************************************************************\
 Allocate zero'd memory.
\******************************************************************************/
void *C_recalloc_full(const char *file, int line, const char *function,
                      void *ptr, size_t size)
{
        ptr = C_realloc_full(file, line, function, ptr, size);
        memset(ptr, 0, size);
        return ptr;
}

/******************************************************************************\
 Will first try to find the resource and reference if it has already been
 allocated. In this case *[found] will be set to TRUE. Otherwise, a new
 resource will be allocated and cleared and *[found] will be set to FALSE.
\******************************************************************************/
void *C_ref_alloc_full(const char *file, int line, const char *function,
                       size_t size, c_ref_t **root, c_ref_cleanup_f cleanup,
                       const char *name, int *found)
{
        c_ref_t *prev, *next, *ref;

        if (size < sizeof (c_ref_t) || !root || !name)
                C_error_full(file, line, function,
                             "Invalid reference structure initilization");

        /* Find a place for the object */
        prev = NULL;
        next = NULL;
        ref = *root;
        while (ref) {
                int cmp;

                cmp = strcmp(name, ref->name);
                if (!cmp) {
                        ref->refs++;
                        C_debug_full(file, line, function,
                                     "Loading '%s', cache hit (%d refs)",
                                     name, ref->refs);
                        if (found)
                                *found = TRUE;
                        return ref;
                }
                if (cmp < 0) {
                        next = ref;
                        ref = NULL;
                        break;
                }
                prev = ref;
                ref = ref->next;
        }
        if (found)
                *found = FALSE;

        /* Allocate a new object */
        ref = C_calloc(size);
        if (!*root)
                *root = ref;
        ref->prev = prev;
        if (prev)
                ref->prev->next = ref;
        ref->next = next;
        if (next)
                ref->next->prev = ref;
        ref->refs = 1;
        ref->cleanup_func = cleanup;
        ref->root = root;
        C_strncpy_buf(ref->name, name);
        C_debug_full(file, line, function, "Loading '%s', allocated new", name);
        return ref;
}

/******************************************************************************\
 Increases the reference count.
\******************************************************************************/
void C_ref_up_full(const char *file, int line, const char *function,
                   c_ref_t *ref)
{
        if (!ref)
                return;
        if (ref->refs < 1)
                C_error_full(file, line, function,
                             "Invalid reference structure");
        ref->refs++;
        C_debug_full(file, line, function,
                     "Referenced '%s' (%d refs)", ref->name, ref->refs);
}

/******************************************************************************\
 Decreases the reference count. If there are no references left, the
 cleanup function is called on the data and the memory is free'd.
\******************************************************************************/
void C_ref_down_full(const char *file, int line, const char *function,
                     c_ref_t *ref)
{
        if (!ref)
                return;
        if (ref->refs < 1 || !ref->root)
                C_error_full(file, line, function,
                             "Invalid reference structure");
        ref->refs--;
        if (ref->refs > 0) {
                C_debug_full(file, line, function,
                             "Dereferenced '%s' (%d refs)",
                             ref->name, ref->refs);
                return;
        }
        if (*ref->root == ref)
                *ref->root = ref->next;
        if (ref->prev)
                ref->prev->next = ref->next;
        if (ref->next)
                ref->next->prev = ref->prev;
        C_debug_full(file, line, function, "Free'd '%s'", ref->name, ref->refs);
        if (ref->cleanup_func)
                ref->cleanup_func(ref);
        C_free(ref);
}

