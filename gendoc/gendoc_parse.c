/******************************************************************************\
 Plutocracy GenDoc - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "gendoc.h"

#define END_BRACE_MARK -2

int d_num_tokens;

static FILE *file;
static int last_nl;
static char last_ch, cur_ch, def[8192], comment[4096], tokens[32][64];

/******************************************************************************\
 Just returns the definition.
\******************************************************************************/
const char *D_def(void)
{
        return def;
}

/******************************************************************************\
 Just returns the comment.
\******************************************************************************/
const char *D_comment(void)
{
        return comment;
}

/******************************************************************************\
 Returns TRUE if the character is an operator.
\******************************************************************************/
static int is_operator(char ch)
{
        return (ch >= '!' && ch <= '/') || (ch >= ':' && ch <= '@') ||
               (ch >= '{' && ch <= '~') || (ch >= '[' && ch <= '^');
}

/******************************************************************************\
 Parse a token. Returns the buffer position of the first character after the
 token. Fixes any markers in the definition.
\******************************************************************************/
static int tokenize_def_from(int i)
{
        int j;

        if (!def[i])
                return i;

        /* Skip space */
        for (; def[i] <= ' '; i++);

        /* C comments */
        if (def[i] == '/' && def[i + 1] == '*') {
                for (i++; def[i] && (def[i] != '/' || def[i - 1] != '*'); i++);
                return i;
        }

        /* C++ comments */
        if (def[i] == '/' && def[i + 1] == '/') {
                for (i++; def[i] && (def[i] != '\n' || def[i - 1] != '*'); i++);
                return i;
        }

        /* Blocks count as a single token */
        if (def[i] == '{') {
                for (j = 0; def[i] && def[i] != END_BRACE_MARK; i++) {
                        if (j < sizeof (tokens[0]))
                                tokens[d_num_tokens][j++] = def[i];
                }
                def[i] = '}';
                d_num_tokens++;
                return i + 1;
        }

        /* String or character constant */
        if (def[i] == '"' || def[i] == '\'') {
                char start;

                tokens[d_num_tokens][0] = start = def[i];
                for (j = 1, ++i; def[i] && j < sizeof (tokens[0]) - 1; j++) {
                        tokens[d_num_tokens][j] = def[i++];
                        if (def[i] == start && def[i - 1] != '\\') {
                                i++;
                                break;
                        }
                }
                d_num_tokens++;
                return i;
        }

        /* Operator */
        if (is_operator(def[i])) {
                tokens[d_num_tokens][0] = def[i];
                tokens[d_num_tokens][1] = NUL;
                d_num_tokens++;
                return i + 1;
        }

        /* Identifier or constant */
        for (j = 0; j < sizeof (tokens[0]) - 1; j++) {
                if (is_operator(def[i]) || def[i] <= ' ')
                        break;
                tokens[d_num_tokens][j] = def[i++];
        }
        d_num_tokens++;
        return i;
}

/******************************************************************************\
 Tokenize the definition string.
\******************************************************************************/
static void tokenize_def(void)
{
        int pos, new_pos;

        memset(tokens, 0, sizeof (tokens));
        for (d_num_tokens = 0, pos = 0;
             d_num_tokens < sizeof (tokens) / sizeof (tokens[0]); ) {
                new_pos = tokenize_def_from(pos);
                if (new_pos == pos)
                        break;
                pos = new_pos;
        }
}

/******************************************************************************\
 Returns the nth token in the definition.
\******************************************************************************/
const char *D_token(int n)
{
        if (n < 0 || n > sizeof (tokens) / sizeof (tokens[0]))
                return "";
        return tokens[n];
}

/******************************************************************************\
 Read in one character.
\******************************************************************************/
static void read_ch(void)
{
        if (cur_ch == EOF)
                return;
        last_ch = cur_ch;
        cur_ch = fgetc(file);
        if (cur_ch == '\n')
                last_nl = 0;
        else
                last_nl++;
}

/******************************************************************************\
 "Unread" one character.
\******************************************************************************/
static void unread_ch(void)
{
        cur_ch = last_ch;
        last_ch = NUL;
}

/******************************************************************************\
 Save comments out of the file.
\******************************************************************************/
static int parse_comment(void)
{
        if (cur_ch != '/')
                return FALSE;
        read_ch();
        if (cur_ch == '*') {
                int i, end;

                while (cur_ch == '*' || cur_ch == '\\')
                        read_ch();
                for (end = i = 0; i < sizeof (comment) && cur_ch != EOF; i++) {
                        if (cur_ch == '/' && last_ch == '*')
                                break;
                        comment[i] = cur_ch;
                        if (cur_ch != '*' && cur_ch != '\\')
                                end = i;
                        read_ch();
                }
                comment[end] = NUL;
                read_ch();
                return TRUE;
        } else if (cur_ch == '/') {
                while (cur_ch != EOF && (cur_ch != '\n' || last_ch == '\\'))
                        read_ch();
                return TRUE;
        }
        unread_ch();
        return FALSE;
}

/******************************************************************************\
 Read in a special section of the definition. [match_last] and [not_last] can
 be NUL to disable.
\******************************************************************************/
static int parse_def_section(int i, char match_cur, char match_last,
                             char not_last)
{
        do {
                def[i++] = cur_ch;
                read_ch();
        } while (cur_ch != EOF && i < sizeof (def) &&
                 (cur_ch != match_cur || last_ch == not_last ||
                  (match_last && last_ch != match_last)));
        def[i] = cur_ch;
        read_ch();
        return i;
}

/******************************************************************************\
 Read in the next definition.
\******************************************************************************/
int D_parse_def(void)
{
        int i, braces, stop_define, stop_typedef;

        /* Skip spaces and pickup the last comment */
        for (read_ch(); ; ) {
                if (cur_ch == EOF)
                        return FALSE;
                if (cur_ch <= ' ')
                        read_ch();
                else if (!parse_comment())
                        break;
        }

        /* The definition may not be the first on the line, pad it */
        if (last_nl < sizeof (def) - 1)
                for (i = 0; i < last_nl - 1; i++)
                        def[i] = ' ';

        /* Read the entire definition */
        stop_define = cur_ch == '#';
        stop_typedef = FALSE;
        for (braces = 0; i < sizeof (def) - 1 && cur_ch != EOF; i++) {

                /* Skip strings */
                if (cur_ch == '"') {
                        i = parse_def_section(i, '"', NUL, '\\');
                        continue;
                }

                /* Skip character constants */
                if (cur_ch == '\'') {
                        i = parse_def_section(i, '\'', NUL, '\\');
                        continue;
                }

                /* Skip C comments */
                if (cur_ch == '*' && last_ch == '/') {
                        i = parse_def_section(i, '/', '*', NUL);
                        continue;
                }

                /* Skip C++ comments */
                if (cur_ch == '/' && last_ch == '/') {
                        i = parse_def_section(i, '\n', NUL, '\\');
                        continue;
                }

                /* Track braces. If there is a newline before there are any
                   characters after the closing brace, assume this is a
                   function body and stop reading there.*/
                if (cur_ch == '{')
                        braces++;
                else if (cur_ch == '}') {
                        braces--;
                        if (braces <= 0) {
                                stop_typedef = TRUE;
                                def[i] = END_BRACE_MARK;
                                read_ch();
                                continue;
                        }
                }
                if ((stop_define || stop_typedef) &&
                    cur_ch == '\n' && last_ch != '\\')
                        break;
                if (cur_ch > ' ' && stop_typedef)
                        stop_typedef = FALSE;

                def[i] = cur_ch;
                if (braces <= 0 && cur_ch == ';') {
                        i++;
                        break;
                }
                read_ch();
        }
        def[i] = NUL;

        tokenize_def();
        return TRUE;
}

/******************************************************************************\
 Opens a file for parsing. Returns FALSE if the file failed to open.
\******************************************************************************/
int D_parse_open(const char *filename)
{
        file = fopen(filename, "r");
        if (!file)
                return FALSE;
        last_nl = 0;
        last_ch = NUL;
        cur_ch = NUL;
        return TRUE;
}

/******************************************************************************\
 Closes the current file.
\******************************************************************************/
void D_parse_close(void)
{
        fclose(file);
}

