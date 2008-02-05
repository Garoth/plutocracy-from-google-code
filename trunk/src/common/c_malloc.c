/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Devin Papineau

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "c_shared.h"

/******************************************************************************\
 Reallocate [ptr] to [size] bytes large. Abort on error.
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
 Free memory pointed to by [ptr].
\******************************************************************************/
void C_free_ptr(void *ptr)
{
        free(ptr);
}

