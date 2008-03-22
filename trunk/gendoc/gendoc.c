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

static entry_t *functions, *types, *defines;

/******************************************************************************\
 Parse a C header file for declarations.
\******************************************************************************/
static void parse_header(const char *filename)
{
        entry_t current;
        int i;

        while (D_parse_def()) {
                memset(&current, 0, sizeof (current));

                /* No statics */
                if (!strcmp(D_token(0), "static"))
                        continue;

                /* #define and macro functions */
                if (D_token(0)[0] == '#' && !strcmp(D_token(1), "define")) {
                        D_strncpy_buf(current.name, D_token(2));
                        D_strncpy_buf(current.def, D_def());
                        D_strncpy_buf(current.comment, D_comment());
                        D_entry_add(&current, D_token(3)[0] == '(' ?
                                              &functions : &defines);
                        continue;
                }

                /* struct or enum */
                if (!strcmp(D_token(0), "struct") ||
                    !strcmp(D_token(0), "enum")) {
                        D_strncpy_buf(current.name, D_token(1));
                        D_strncpy_buf(current.def, D_def());
                        D_strncpy_buf(current.comment, D_comment());
                        D_entry_add(&current, &types);
                        continue;
                }

                /* typedef */
                if (!strcmp(D_token(0), "typedef")) {
                        D_strncpy_buf(current.name, D_token(d_num_tokens - 2));

                        /* Function pointer */
                        if (D_token(d_num_tokens - 2)[0] == ')')
                                for (i = 1; i < d_num_tokens - 2; i++)
                                        if (D_token(i)[0] == '(' &&
                                            D_token(i + 1)[0] == '*')
                                                D_strncpy_buf(current.name,
                                                              D_token(i + 2));

                        D_strncpy_buf(current.def, D_def());
                        D_strncpy_buf(current.comment, D_comment());
                        D_entry_add(&current, &types);
                        continue;
                }

                /* Function */
                if (D_token(d_num_tokens - 2)[0] == ')') {
                        for (i = 2; i < d_num_tokens; i++)
                                if (D_token(i)[0] == '(')
                                        break;
                        if (i >= d_num_tokens)
                                continue;
                        D_strncpy_buf(current.name, D_token(i - 1));
                        D_strncpy_buf(current.def, D_def());
                        D_strncpy_buf(current.comment, D_comment());
                        D_entry_add(&current, &functions);
                        continue;
                }
        }
}

/******************************************************************************\
 Parse a C source file for comments. This is only done to pick up source file
 comments which are assumed to be more informative.
\******************************************************************************/
static void parse_source(const char *filename)
{
        entry_t *entry;
        int i;

        while (D_parse_def()) {

                /* Must be a non-static function body */
                if (!strcmp(D_token(0), "static") ||
                    !strcmp(D_token(0), "enum") ||
                    D_token(d_num_tokens - 1)[0] != '{')
                        continue;

                /* Find the function */
                for (i = 2; i < d_num_tokens; i++)
                        if (D_token(i)[0] == '(')
                                break;
                if (i >= d_num_tokens)
                        continue;
                entry = D_entry_find(functions, D_token(i - 1));
                if (!entry)
                        continue;

                /* Update the comment */
                D_strncpy_buf(entry->comment, D_comment());
        }
}

/******************************************************************************\
 Parse an input file.
\******************************************************************************/
static void parse_file(const char *filename)
{
        size_t len;

        if (!D_parse_open(filename)) {
                printf("    Failed to open for reading!\n");
                return;
        }
        len = strlen(filename);
        if (len > 2 && filename[len - 2] == '.' && filename[len - 1] == 'h')
                parse_header(filename);
        else if (len > 2 && filename[len - 2] == '.' &&
                 filename[len - 1] == 'c')
                parse_source(filename);
        else
                printf("    Unrecognized extension.\n");
        D_parse_close();
}

/******************************************************************************\
 Outputs HTML document.
\******************************************************************************/
static void output_html(const char *filename)
{
        FILE *file;

        file = fopen(filename, "w");
        if (!file) {
                printf("Failed to open for writing!\n");
                return;
        }

        /* Header */
        fprintf(file, "<!-- Plutocracy GenDoc -->\n<html><body>\n");

        /* Write definitions */
        fprintf(file, "<h1>Definitions</h1>");
        D_output_entries(file, defines);

        /* Write types */
        fprintf(file, "<h1>Types</h1>");
        D_output_entries(file, types);

        /* Write functions */
        fprintf(file, "<h1>Functions</h1>");
        D_output_entries(file, functions);

        /* Footer */
        fprintf(file, "</body></html>");

        fclose(file);
}

/******************************************************************************\
 Entry point. Prints out help directions.
\******************************************************************************/
int main(int argc, char *argv[])
{
        int i;

        /* Somebody didn't read directions */
        if (argc < 2) {
                printf("Usage: gendoc output.html [input headers] "
                       "[input sources]\nYou shouldn't need to run this "
                       "program directly, running 'make' should automatically "
                       "run this with the correct arguments.\n");
                return 1;
        }

        /* Process input files */
        printf("\n%s --\n", argv[1]);
        for (i = 2; i < argc; i++) {
                printf("%2d. %s\n", i - 1, argv[i]);
                parse_file(argv[i]);
        }
        printf("\n");

        /* Output HTML document */
        output_html(argv[1]);

        return 0;
}

