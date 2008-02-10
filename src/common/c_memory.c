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

/* This is the only file that legitimately uses standard library allocation
   and freeing functions */
#undef calloc
#undef free
#undef malloc
#undef realloc

/* When memory checking is enabled, this structure is prepended to every
   allocated block. There is also a no-mans-land chunk, filled with a specific
   byte, at the end of every allocated block as well. */
#define NO_MANS_LAND_BYTE 0x5a
#define NO_MANS_LAND_SIZE 64
typedef struct c_mem_tag {
        struct c_mem_tag *next;
        const char *alloc_file, *alloc_func, *free_file, *free_func;
        void *data;
        size_t size;
        int alloc_line, free_line, freed;
        char no_mans_land[NO_MANS_LAND_SIZE];
} c_mem_tag_t;

static c_mem_tag_t *mem_root;
extern c_var_t c_mem_check;

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
 Allocates new memory, similar to realloc_checked().
\******************************************************************************/
static void *malloc_checked(const char *file, int line, const char *function,
                            size_t size)
{
        c_mem_tag_t *tag;
        size_t real_size;

        real_size = size + sizeof (c_mem_tag_t) + NO_MANS_LAND_SIZE;
        tag = malloc(real_size);
        tag->data = (char *)tag + sizeof (c_mem_tag_t);
        tag->size = size;
        tag->alloc_file = file;
        tag->alloc_line = line;
        tag->alloc_func = function;
        tag->freed = FALSE;
        memset(tag->no_mans_land, NO_MANS_LAND_BYTE, NO_MANS_LAND_SIZE);
        memset((char *)tag + sizeof (c_mem_tag_t) + size,
               NO_MANS_LAND_BYTE, NO_MANS_LAND_SIZE);
        tag->next = NULL;
        if (mem_root)
                tag->next = mem_root;
        mem_root = tag;
        return tag->data;
}

/******************************************************************************\
 Finds the memory tag that holds [ptr].
\******************************************************************************/
static c_mem_tag_t *find_tag(const void *ptr, c_mem_tag_t **prev_tag)
{
        c_mem_tag_t *tag, *prev;

        prev = NULL;
        tag = mem_root;
        while (tag && tag->data != ptr) {
                prev = tag;
                tag = tag->next;
        }
        if (prev_tag)
                *prev_tag = prev;
        return tag;
}

/******************************************************************************\
 Reallocate [ptr] to [size] bytes large. Abort on error. String all the
 allocated chunks into a linked list and tracks information about the
 memory and where it was allocated from. This is used later in C_free() and
 C_check_leaks() to detect various errors.
\******************************************************************************/
static void *realloc_checked(const char *file, int line, const char *function,
                             void *ptr, size_t size)
{
        c_mem_tag_t *tag, *prev_tag;
        size_t real_size;

        if (!ptr)
                return malloc_checked(file, line, function, size);
        if (!(tag = find_tag(ptr, &prev_tag)))
                C_error_full(file, line, function,
                             "Trying to reallocate unallocated address (0x%x)",
                             ptr);
        real_size = size + sizeof (c_mem_tag_t) + NO_MANS_LAND_SIZE;
        tag = realloc(ptr - sizeof (c_mem_tag_t), real_size);
        if (!tag)
                C_error("Out of memory, %s() (%s:%d) tried to allocate %d "
                        "bytes", function, file, line, size );
        tag->size = size;
        tag->alloc_file = file;
        tag->alloc_line = line;
        tag->alloc_func = function;
        tag->data = (char *)tag + sizeof (c_mem_tag_t);
        return tag->data;
}

/******************************************************************************\
 Reallocate [ptr] to [size] bytes large. Abort on error. When memory checking
 is enabled, this calls realloc_checked() instead.
\******************************************************************************/
void *C_realloc_full(const char *file, int line, const char *function,
                     void *ptr, size_t size)
{
        if (c_mem_check.value.n)
                return realloc_checked(file, line, function, ptr, size);
        ptr = realloc(ptr, size);
        if (!ptr)
                C_error("Out of memory, %s() (%s:%d) tried to allocate %d "
                        "bytes", function, file, line, size );
        return ptr;
}

/******************************************************************************\
 Checks if a no-mans-land region has been corrupted.
\******************************************************************************/
static int check_no_mans_land(const char *ptr)
{
        int i;

        for (i = 0; i < NO_MANS_LAND_SIZE; i++)
                if (ptr[i] != NO_MANS_LAND_BYTE)
                        return FALSE;
        return TRUE;
}

/******************************************************************************\
 Frees memory. If memory checking is enabled, will check the following:
 - [ptr] was never allocated
 - [ptr] was already freed
 - [ptr] no-mans-land (upper or lower) was corrupted
\******************************************************************************/
void C_free_full(const char *file, int line, const char *function, void *ptr)
{
        c_mem_tag_t *tag, *prev_tag, *old_tag;

        if (!c_mem_check.value.n) {
                free(ptr);
                return;
        }
        if (!(tag = find_tag(ptr, &prev_tag)))
                C_error_full(file, line, function,
                             "Trying to free unallocated address (0x%x)", ptr);
        if (tag->freed)
                C_error_full(file, line, function,
                             "Address (0x%x), %d bytes allocated by "
                             "%s in %s:%d, already freed by %s() in %s:%d",
                             ptr, tag->size,
                             tag->alloc_func, tag->alloc_file, tag->alloc_line,
                             tag->free_func, tag->free_file, tag->free_line);
        if (!check_no_mans_land(tag->no_mans_land))
                C_error_full(file, line, function,
                             "Address (0x%x), %d bytes allocated by "
                             "%s in %s:%d, overran lower boundary",
                             ptr, tag->size,
                             tag->alloc_func, tag->alloc_file, tag->alloc_line);
        if (!check_no_mans_land((char *)ptr + tag->size + sizeof (*tag)))
                C_error_full(file, line, function,
                             "Address (0x%x), %d bytes allocated by "
                             "%s in %s:%d, overran upper boundary",
                             ptr, tag->size,
                             tag->alloc_func, tag->alloc_file, tag->alloc_line);
        tag->freed = TRUE;
        tag->free_file = file;
        tag->free_line = line;
        tag->free_func = function;
        old_tag = tag;
        tag = realloc(tag, sizeof (*tag));
        if (prev_tag)
                prev_tag->next = tag;
        if (old_tag == mem_root)
                mem_root = tag;
}

/******************************************************************************\
 If memory checking is enabled, checks for memory leaks and prints warnings.
\******************************************************************************/
void C_check_leaks(void)
{
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
 cleanup function is called on the data and the memory is freed.
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
        C_debug_full(file, line, function, "Freed '%s'", ref->name, ref->refs);
        if (ref->cleanup_func)
                ref->cleanup_func(ref);
        C_free(ref);
}

