/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Classes and functions that manipulate strings and files. */

#include "c_shared.h"

/* This file legitimately uses standard library file I/O functions */
#undef fclose

/******************************************************************************\
 These functions parse variable argument lists into a static buffer and return
 a pointer to it. You can also pass a pointer to an integer to get the length
 of the output. Not thread safe, but very convenient. Note that after repeated
 calls to these functions, C_vanv will eventually run out of buffers and start
 overwriting old ones. The idea for these functions comes from the Quake 3
 source code.
\******************************************************************************/
char *C_vanv(int *plen, const char *fmt, va_list va)
{
	static char buffer[C_VA_BUFFERS][C_VA_BUFFER_SIZE];
	static int which;
	int len;

	which++;
	if (which >= C_VA_BUFFERS)
	        which = 0;
	len = vsnprintf(buffer[which], sizeof (buffer[which]), fmt, va);
	if (plen)
		*plen = len;
	return buffer[which];
}

char *C_van(int *plen, const char *fmt, ...)
{
	va_list va;
	char *string;

	va_start(va, fmt);
	string = C_vanv(plen, fmt, va);
	va_end(va);
	return string;
}

char *C_va(const char *fmt, ...)
{
	va_list va;
	char *string;

	va_start(va, fmt);
	string = C_vanv(NULL, fmt, va);
	va_end(va);
	return string;
}

/******************************************************************************\
 Skips any space characters in the string.
\******************************************************************************/
char *C_skip_spaces(const char *str)
{
        char ch;

        ch = *str;
        while (C_is_space(ch))
                ch = *(++str);
        return (char *)str;
}

/******************************************************************************\
 Read an entire file into memory. Returns number of bytes read or -1 if an
 error occurs.
\******************************************************************************/
int C_read_file(const char *filename, char *buffer, int size)
{
        c_file_t *file;
        size_t bytes_read;

        C_assert(buffer);
        if (!filename || !filename[0]) {
                buffer[0] = NUL;
                return -1;
        }
        file = C_file_open_read(filename);
        if (!file) {
                C_warning("Failed to open '%s'", filename);
                buffer[0] = NUL;
                return -1;
        }
        bytes_read = C_file_read(file, buffer, size);
        if (bytes_read > size - 1)
                bytes_read = size - 1;
        buffer[bytes_read] = NUL;
        C_file_close(file);
        C_debug("Read '%s' (%d bytes)", filename, bytes_read);
        return bytes_read;
}

/******************************************************************************\
 Initializes a token file structure. Returns FALSE on failure.
\******************************************************************************/
int C_token_file_init(c_token_file_t *tf, const char *filename)
{
        C_strncpy_buf(tf->filename, filename);
        C_zero_buf(tf->buffer);
        tf->token = tf->pos = tf->buffer + sizeof (tf->buffer) - 2;
        tf->swap = ' ';
        tf->file = C_file_open_read(filename);
        if (!tf->file) {
                C_warning("Failed to open token file '%s'", filename);
                return FALSE;
        }
        C_debug("Opened '%s'", filename);
        return TRUE;
}

/******************************************************************************\
 Cleanup resources associated with a token file.
\******************************************************************************/
void C_token_file_cleanup(c_token_file_t *tf)
{
        C_file_close(tf->file);
        C_debug("Closed '%s'", tf->filename);
}

/******************************************************************************\
 Fills the buffer with new data when necessary. Should be run before any time
 the token file buffer position advances.
\******************************************************************************/
static void token_file_check_chunk(c_token_file_t *tf)
{
        size_t token_len, bytes_read;

        if (tf->pos[1])
                return;
        token_len = tf->pos - tf->token + 1;
        if (token_len >= sizeof (tf->buffer) - 1) {
                C_warning("Oversize token in '%s'", tf->filename);
                token_len = 0;
        }
        memmove(tf->buffer, tf->token, token_len);
        tf->token = tf->buffer;
        tf->pos = tf->token + token_len - 1;
        bytes_read = C_file_read(tf->file, tf->buffer + token_len,
                                 sizeof (tf->buffer) - token_len - 1);
        tf->buffer[token_len + bytes_read] = NUL;

}

/******************************************************************************\
 Read a token out of a token file. A token is either a series of characters
 unbroken by spaces or comments or a double-quote enclosed string. Enclosed
 strings are parsed for backslash symbols. A token file supports '#' line
 comments.
\******************************************************************************/
const char *C_token_file_read_full(c_token_file_t *tf, int *quoted)
{
        int parsing_comment, parsing_string;

        if (!tf->file || !tf->pos || tf->pos < tf->buffer ||
            tf->pos >= tf->buffer + sizeof (tf->buffer))
                C_error("Invalid token file");
        *tf->pos = tf->swap;

        /* Skip space */
        parsing_comment = FALSE;
        while (C_is_space(*tf->pos) || parsing_comment || *tf->pos == '#') {
                if (*tf->pos == '#')
                        parsing_comment = TRUE;
                if (*tf->pos == '\n')
                        parsing_comment = FALSE;
                token_file_check_chunk(tf);
                tf->token = ++tf->pos;
        }
        if (!*tf->pos)
                return "";

        /* Read token */
        if ((parsing_string = *tf->pos == '"')) {
                token_file_check_chunk(tf);
                tf->token = ++tf->pos;
        }
        while (*tf->pos) {
                if (parsing_string) {
                        if (*tf->pos == '"') {
                                token_file_check_chunk(tf);
                                *(tf->pos++) = NUL;
                                break;
                        }
                } else if (C_is_space(*tf->pos) || *tf->pos == '#')
                        break;
                if (tf->pos[-1] == '\\') {
                        if (*tf->pos == 'n')
                                *tf->pos = '\n';
                        else if (*tf->pos == 'r')
                                *tf->pos = '\r';
                        else if (*tf->pos == 't')
                                *tf->pos = '\t';
                        else
                                memmove(tf->pos - 1, tf->pos, tf->buffer +
                                        sizeof (tf->buffer) - tf->pos);
                }
                token_file_check_chunk(tf);
                tf->pos++;
        }

        tf->swap = *tf->pos;
        *tf->pos = NUL;
        if (quoted)
                *quoted = parsing_string;
        return tf->token;
}

/******************************************************************************\
 Equivalent to the standard library strncpy, but ensures that there is always
 a NUL terminator. Sometimes known as "strncpyz". Can copy overlapped strings.
 Returns the length of the source string.
\******************************************************************************/
size_t C_strncpy(char *dest, const char *src, size_t len)
{
        size_t src_len;

        if (!dest)
                return 0;
        if (!src) {
                if (len > 0)
                        dest[0] = NUL;
                return 0;
        }
        src_len = strlen(src);
        if (src_len > --len) {
                C_debug("dest (%d bytes) too short to hold src (%d bytes)",
                        len, src_len);
                src_len = len;
        }
        memmove(dest, src, src_len);
        dest[src_len] = NUL;
        return src_len;
}

/******************************************************************************\
 A simpler wrapper that won't cause a segfault when passed a NULL pointer.
\******************************************************************************/
void C_file_close(c_file_t *file)
{
        if (!file)
                return;
        fclose(file);
}

/******************************************************************************\
 Allocates new heap memory for and duplicates a string. Doesn't crash if the
 string passed in is NULL.
\******************************************************************************/
char *C_strdup_full(const char *file, int line, const char *function,
                    const char *str)
{
        char *new_str;
        size_t len;

        if (!str)
                return NULL;
        len = strlen(str) + 1;
        new_str = C_realloc_full(file, line, function, NULL, len);
        memcpy(new_str, str, len);
        return new_str;
}

