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

        file = C_file_open_read(filename);
        if (!file) {
                C_warning("Failed to open '%s'", filename);
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
        strncpy(tf->filename, filename, sizeof (tf->filename));
        tf->token = tf->pos = tf->buffer + sizeof (tf->buffer) - 1;
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
 Fills the buffer with new data.
\******************************************************************************/
static void token_file_read_chunk(c_token_file_t *tf)
{
        size_t token_len, bytes_read;

        token_len = tf->pos - tf->token;
        if (token_len >= sizeof (tf->buffer) - 1) {
                C_warning("Oversize token in '%s'", tf->filename);
                token_len = 0;
        }
        if (token_len) {
                memmove(tf->buffer, tf->token, token_len);
                tf->token = tf->buffer;
                tf->pos = tf->token + token_len;
        } else
                tf->token = tf->pos = tf->buffer;
        bytes_read = C_file_read(tf->file, tf->buffer + token_len,
                                 sizeof (tf->buffer) - token_len - 1);
        tf->buffer[bytes_read] = NUL;

}

/******************************************************************************\
 Read a token out of a token file. A token is either a series of characters
 unbroken by spaces or comments or a double-quote enclosed string. Enclosed
 strings are parsed for backslash symbols. A token file supports '#' line
 comments.
\******************************************************************************/
const char *C_token_file_read(c_token_file_t *tf)
{
        int parsing_comment, parsing_string;

        if (!tf->file)
                return "";
        *tf->pos = tf->swap;

        /* Skip space */
        parsing_comment = FALSE;
        while (C_is_space(*tf->pos) || parsing_comment || *tf->pos == '#') {
                if (*tf->pos == '#')
                        parsing_comment = TRUE;
                if (*tf->pos == '\n')
                        parsing_comment = FALSE;
                if (tf->pos == tf->buffer + sizeof (tf->buffer) - 1) {
                        token_file_read_chunk(tf);
                        continue;
                }
                tf->pos++;
        }
        if (!*tf->pos)
                return "";

        /* Read token */
        if ((parsing_string = *tf->pos == '"'))
                tf->pos++;
        tf->token = tf->pos;
        while (*tf->pos) {
                if (parsing_string) {
                        if (*tf->pos == '"') {
                                *(tf->pos++) = NUL;
                                break;
                        }
                } else if (C_is_space(*tf->pos) || *tf->pos == '#')
                        break;
                if (*tf->pos == '\\' && tf->pos[1]) {
                        if (tf->pos[1] == 'n')
                                *tf->pos = '\n';
                        else if (tf->pos[1] == 'r')
                                *tf->pos = '\r';
                        else if (tf->pos[1] == 't')
                                *tf->pos = '\t';
                        else
                                memmove(tf->pos, tf->pos + 1,
                                        sizeof (tf->buffer) - (int)tf->pos - 1);
                }
                if (tf->pos == tf->buffer + sizeof (tf->buffer) - 1 ||
                    !*tf->pos) {
                        token_file_read_chunk(tf);
                        continue;
                }
                tf->pos++;
        }

        tf->swap = *tf->pos;
        *tf->pos = NUL;
        return tf->token;
}

