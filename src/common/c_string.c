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

/* This file legitimately uses standard library string and file I/O functions */
#ifdef PLUTOCRACY_LIBC_ERRORS
#undef strlen
#undef fclose
#endif

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
        int bytes_read;

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
        bytes_read = (int)C_file_read(file, buffer, size);
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
        tf->token = tf->pos = tf->buffer + sizeof (tf->buffer) - 3;
        tf->pos[1] = tf->swap = ' ';
        tf->pos[2] = NUL;
        tf->file = C_file_open_read(filename);
        tf->eof = FALSE;
        if (!tf->file) {
                C_warning("Failed to open token file '%s'", filename);
                return FALSE;
        }
        C_debug("Opened '%s'", filename);
        return TRUE;
}

/******************************************************************************\
 Initializes a token file structure without actually opening a file. The
 buffer is filled with [string].
\******************************************************************************/
void C_token_file_init_string(c_token_file_t *tf, const char *string)
{
        C_strncpy_buf(tf->buffer, string);
        tf->token = tf->pos = tf->buffer;
        tf->swap = tf->buffer[0];
        tf->filename[0] = NUL;
        tf->file = NULL;
        tf->eof = TRUE;
}

/******************************************************************************\
 Cleanup resources associated with a token file.
\******************************************************************************/
void C_token_file_cleanup(c_token_file_t *tf)
{
        if (!tf)
                return;
        if (tf->file) {
                C_file_close(tf->file);
                C_debug("Closed '%s'", tf->filename);
        }
}

/******************************************************************************\
 Fills the buffer with new data when necessary. Should be run before any time
 the token file buffer position advances.

 Because we need to check ahead one character in order to properly detect
 C-style block comments, this function will read in the next chunk while there
 is still one unread byte left in the buffer.
\******************************************************************************/
static void token_file_check_chunk(c_token_file_t *tf)
{
        size_t token_len, bytes_read, to_read;

        if ((tf->pos[1] && tf->pos[2]) || tf->eof)
                return;
        token_len = tf->pos - tf->token + 1;
        if (token_len >= sizeof (tf->buffer) - 2) {
                C_warning("Oversize token in '%s'", tf->filename);
                token_len = 0;
        }
        memmove(tf->buffer, tf->token, token_len);
        tf->token = tf->buffer;
        tf->buffer[token_len] = tf->pos[1];
        tf->pos = tf->buffer + token_len - 1;
        to_read = sizeof (tf->buffer) - token_len - 2;
        bytes_read = C_file_read(tf->file, tf->pos + 2, to_read);
        tf->buffer[token_len + bytes_read] = NUL;
        tf->eof = bytes_read < to_read;
}

/******************************************************************************\
 Returns positive if the string starts with a comment. Returns negative if the
 string is a block comment. Returns zero otherwise.
\******************************************************************************/
static int is_comment(const char *str)
{
        if (str[0] == '/' && str[1] == '*')
                return -1;
        if (str[0] == '#' || (str[0] == '/' && str[1] == '/'))
                return 1;
        return 0;
}

/******************************************************************************\
 Returns TRUE if the string ends the current comment type.
\******************************************************************************/
static int is_comment_end(const char *str, int type)
{
        return (type > 0 && str[0] == '\n') ||
               (type < 0 && str[0] == '/' && str[-1] == '*');
}

/******************************************************************************\
 Read a token out of a token file. A token is either a series of characters
 unbroken by spaces or comments or a single or double-quote enclosed string.
 The kind of encolosed string (or zero) is returned via [quoted]. Enclosed
 strings are parsed for backslash symbols. Token files support Bash, C, and
 C++ style comments.
\******************************************************************************/
const char *C_token_file_read_full(c_token_file_t *tf, int *quoted)
{
        int parsing_comment, parsing_string;

        if (!tf->pos || tf->pos < tf->buffer ||
            tf->pos >= tf->buffer + sizeof (tf->buffer))
                C_error("Invalid token file");
        *tf->pos = tf->swap;

        /* Skip space */
        parsing_comment = FALSE;
        while (C_is_space(*tf->pos) || parsing_comment || is_comment(tf->pos)) {
                if (!parsing_comment)
                        parsing_comment = is_comment(tf->pos);
                else if (is_comment_end(tf->pos, parsing_comment))
                        parsing_comment = 0;
                token_file_check_chunk(tf);
                tf->token = ++tf->pos;
        }
        if (!*tf->pos) {
                if (quoted)
                        *quoted = FALSE;
                return "";
        }

        /* Read token */
        parsing_string = FALSE;
        if (*tf->pos == '"' || *tf->pos == '\'') {
                parsing_string = *tf->pos;
                token_file_check_chunk(tf);
                tf->token = ++tf->pos;
        }
        while (*tf->pos) {
                if (parsing_string) {
                        if (*tf->pos == parsing_string) {
                                token_file_check_chunk(tf);
                                *(tf->pos++) = NUL;
                                break;
                        } else if (parsing_string == '"' && *tf->pos == '\\') {
                                token_file_check_chunk(tf);
                                memmove(tf->pos, tf->pos + 1, tf->buffer +
                                        sizeof (tf->buffer) - tf->pos - 1);
                                if (*tf->pos == 'n')
                                        *tf->pos = '\n';
                                else if (*tf->pos == 'r')
                                        *tf->pos = '\r';
                                else if (*tf->pos == 't')
                                        *tf->pos = '\t';
                        }
                } else if (C_is_space(*tf->pos) || is_comment(tf->pos))
                        break;
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
int C_strncpy(char *dest, const char *src, int len)
{
        int src_len;

        if (!dest)
                return 0;
        if (!src) {
                if (len > 0)
                        dest[0] = NUL;
                return 0;
        }
        src_len = (int)strlen(src);
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
 Equivalent to the standard library strlen but returns zero if the string is
 NULL.
\******************************************************************************/
int C_strlen(const char *string)
{
        if (!string)
                return 0;
        return (int)strlen(string);
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

/******************************************************************************\
 Parses an HTML-style hexadecimal color string (#AARRGGBB). It may or may not
 have a '#' in front.
\******************************************************************************/
static struct {
        const char *name;
        unsigned int value;
} colors[] = {
        { "clear", 0x00000000 },

        /* Full-bright colors */
        { "white", 0xffffffff },
        { "black", 0xff000000 },
        { "red", 0xffff0000 },
        { "green", 0xff00ff00 },
        { "blue", 0xff0000ff },
        { "yellow", 0xffffff00 },
        { "cyan", 0xff00ffff },
        { "pink", 0xffff00ff },

        /* Tango colors */
        { "butter1", 0xfffce94f },
        { "butter2", 0xffedd400 },
        { "butter3", 0xffc4a000 },
        { "orange1", 0xfffcaf3e },
        { "orange2", 0xfff57900 },
        { "orange3", 0xffce5c00 },
        { "chocolate1", 0xffe9b96e },
        { "chocolate2", 0xffc17d11 },
        { "chocolate3", 0xff8f5902 },
        { "chameleon1", 0xff8ae234 },
        { "chameleon2", 0xff73d216 },
        { "chameleon3", 0xff4e9a06 },
        { "skyblue1", 0xff729fcf },
        { "skyblue2", 0xff3465a4 },
        { "skyblue3", 0xff204a87 },
        { "plum1", 0xffad7fa8 },
        { "plum2", 0xff75507b },
        { "plum3", 0xff5c3566 },
        { "scarletred1", 0xffef2929 },
        { "scarletred2", 0xffcc0000 },
        { "scarletred3", 0xffa40000 },
        { "aluminium1", 0xffeeeeec },
        { "aluminium2", 0xffd3d7cf },
        { "aluminium3", 0xffbabdb6 },
        { "aluminium4", 0xff888a85 },
        { "aluminium5", 0xff555753 },
        { "aluminium6", 0xff2e3436 },
};

c_color_t C_color_string(const char *string)
{
        int i;

        if (string[0] == '#')
                string++;
        for (i = 0; i < sizeof (colors) / sizeof (*colors); i++)
                if (!strcasecmp(colors[i].name, string))
                        return C_color_32(colors[i].value);
        return C_color_32(strtoul(string, NULL, 16));
}

/******************************************************************************\
 Wraps a string in double-quotes and escapes any special characters. Returns
 a statically allocated string.
\******************************************************************************/
char *C_escape_string(const char *str)
{
        static char buf[4096];
        char *pbuf;

        buf[0] = '"';
        for (pbuf = buf + 1; *str && pbuf <= buf + sizeof (buf) - 3; str++) {
                if (*str == '"' || *str == '\\')
                        *pbuf++ = '\\';
                else if (*str == '\n') {
                        *pbuf++ = '\\';
                        *pbuf = 'n';
                        continue;
                } else if(*str == '\t') {
                        *pbuf++ = '\\';
                        *pbuf = 't';
                        continue;
                } else if (*str == '\r')
                        continue;
                *pbuf++ = *str;
        }
        *pbuf++ = '"';
        *pbuf = NUL;
        return buf;
}

/******************************************************************************\
 Returns the number of bytes in a single UTF-8 character:
 http://www.cl.cam.ac.uk/%7Emgk25/unicode.html#utf-8
\******************************************************************************/
int C_utf8_size(unsigned char first_byte)
{
        /* U-00000000 – U-0000007F (ASCII) */
        if (first_byte < 192)
                return 1;

        /* U-00000080 – U-000007FF */
        else if (first_byte < 224)
                return 2;

        /* U-00000800 – U-0000FFFF */
        else if (first_byte < 240)
                return 3;

        /* U-00010000 – U-001FFFFF */
        else if (first_byte < 248)
                return 4;

        /* U-00200000 – U-03FFFFFF */
        else if (first_byte < 252)
                return 5;

        /* U-04000000 – U-7FFFFFFF */
        return 6;
}

/******************************************************************************\
 Returns the index of the [n]th UTF-8 character in [str].
\******************************************************************************/
int C_utf8_index(char *str, int n)
{
        int i, len;

        for (i = 0; n > 0; n--)
                for (len = C_utf8_size(str[i]); len > 0; len--, i++)
                        if (!str[i])
                                return i;
        return i;
}

/******************************************************************************\
 Equivalent to the standard library strlen but returns zero if the string is
 NULL. If [chars] is not NULL, the number of UTF-8 characters will be output
 to it. Note that if the number of UTF-8 characters is not needed, C_strlen()
 is preferrable to this function.
\******************************************************************************/
int C_utf8_strlen(const char *str, int *chars)
{
        int i, str_len, char_len;

        if (!str || !str[0]) {
                if (chars)
                        *chars = 0;
                return 0;
        }
        char_len = C_utf8_size(*str);
        for (i = 0, str_len = 1; str[i]; char_len--, i++)
                if (char_len < 1) {
                        str_len++;
                        char_len = C_utf8_size(str[i]);
                }
        if (chars)
                *chars = str_len;
        return i;
}

/******************************************************************************\
 One UTF-8 character from [src] will be copied to [dest]. The index of the
 current position in [dest], [dest_i] and the index in [src], [src_i], will
 be advanced accordingly. [dest] will not be allowed to overrun [dest_sz]
 bytes. Returns the size of the UTF-8 character or 0 if there is not enough
 room or if [src] starts with NUL.
\******************************************************************************/
int C_utf8_append(char *dest, int *dest_i, int dest_sz, const char *src)
{
        int len, char_len;

        if (!*src)
                return 0;
        char_len = C_utf8_size(*src);
        if (*dest_i + char_len > dest_sz)
                return 0;
        for (len = char_len; *src && len > 0; len--)
                dest[(*dest_i)++] = *src++;
        return char_len;
}

/******************************************************************************\
 Convert a Unicode token to a UTF-8 character sequence. The length of the
 token in bytes is output to [plen], which can be NULL.
\******************************************************************************/
char *C_utf8_encode(unsigned int unicode, int *plen)
{
        static char buf[7];
        int len;

        /* ASCII is an exception */
        if (unicode <= 0x7f) {
                buf[0] = unicode;
                buf[1] = NUL;
                if (plen)
                        *plen = 1;
                return buf;
        }

        /* Size of the sequence depends on the range */
        if (unicode <= 0xff)
                len = 2;
        else if (unicode <= 0xffff)
                len = 3;
        else if (unicode <= 0x1fffff)
                len = 4;
        else if (unicode <= 0x3FFFFFF)
                len = 5;
        else if (unicode <= 0x7FFFFFF)
                len = 6;
        else {
                C_warning("Invalid Unicode character 0x%8x", unicode);
                buf[0] = NUL;
                if (plen)
                        *plen = 0;
                return buf;
        }
        if (plen)
                *plen = len;

        /* The first byte is 0b*110x* and the rest are 0b10xxxxxx */
        buf[0] = 0xfc << (6 - len);
        while (--len > 0) {
                buf[len] = 0x80 + (unicode & 0x3f);
                unicode >>= 6;
        }
        buf[0] += unicode;
        return buf;
}

