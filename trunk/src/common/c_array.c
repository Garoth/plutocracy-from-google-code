/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin, Devin Papineau

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

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
void C_array_append(c_array_t *ary, void* item)
{
        if(ary->len >= ary->capacity) {
                C_array_reserve(ary, ary->capacity * 2);
        }

        memcpy((char*)ary->elems + ary->len * ary->item_size,
               item,
               ary->item_size);

        ary->len++;
}

/******************************************************************************\
 Realloc so the array isn't overallocated, and return the pointer to the
 dynamic memory, otherwise cleaning up.
\******************************************************************************/
void* C_array_steal(c_array_t *ary)
{
        void* result = C_realloc(ary->elems, ary->len * ary->item_size);
        memset(ary, '\0', sizeof(*ary));
        return result;
}

/****************************************************************************** \
 Clean up after the array.
\******************************************************************************/
void C_array_cleanup(c_array_t *ary)
{
        C_free(ary->elems);
        memset(ary, '\0', sizeof(*ary));
}
