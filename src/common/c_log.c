/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "c_shared.h"
#include <stdio.h>
#include <stdarg.h>

/******************************************************************************\
 Prints a string to the debug log file or to standard output.
\******************************************************************************/
void C_debug_full(const char *file, int line, const char *function,
                  const char *fmt, ...)
{
        char fmt2[128];
        va_list va;

        va_start(va, fmt);
        snprintf(fmt2, sizeof(fmt2), "*** %s:%d, %s() -- %s\n",
                 file, line, function, fmt);
        vfprintf(stderr, fmt2, va);
        va_end(va);
}

