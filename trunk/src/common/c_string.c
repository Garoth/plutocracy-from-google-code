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

/******************************************************************************\
 Skips any space characters in the string.
\******************************************************************************/
char *C_skip_spaces(const char *str)
{
        char ch;

        ch = *str;
        while (ch && ch <= ' ')
                ch = *(++str);
        return (char *)str;
}

/******************************************************************************\
 Read an entire file into memory. Returns number of bytes read or -1 if an
 error occurs.
\******************************************************************************/
int C_read_file(const char *filename, char *buffer, int size)
{
        FILE *file;
        size_t bytes_read;

        file = fopen(filename, "r");
        if (!file) {
                C_warning("Failed to open '%s'", filename);
                return -1;
        }
        bytes_read = fread(buffer, sizeof (*buffer), size, file);
        if (bytes_read > size - 1)
                bytes_read = size - 1;
        buffer[bytes_read] = NUL;
        fclose(file);
        C_debug("Read '%s' (%d bytes)", filename, bytes_read);
        return bytes_read;
}

