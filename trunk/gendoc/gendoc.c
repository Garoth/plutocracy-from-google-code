/******************************************************************************\
 Plutocracy Gendoc - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

/* Boolean defines */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NUL
#define NUL '\0'
#endif

/* Copies string to a fixed-size buffer */
#define strncpy_buf(b, s) strncpy(b, s, sizeof (b))

/* Token types */
typedef enum {
        TOKEN_IDENTIFIER,
        TOKEN_NUMERIC,
        TOKEN_PREPROCESSOR,
        TOKEN_STRING,
        TOKEN_CHARACTER,
        TOKEN_OPERATOR,
        TOKEN_OP_BLOB,
        TOKENS,
} token_t;

/* List entry */
typedef struct entry {
        char pre[64], name[128], post[1024], comment[4096];
        struct entry *next;
} entry_t;

static FILE *file;
static entry_t *functions, *types, *defines;
static token_t token_type;
static char llast_ch, last_ch, cur_ch, next_ch,
            token[128], pre[64], name[128], post[1024], comment[4096];

/******************************************************************************\
 Sets up an entry.
\******************************************************************************/
static void entry_setup(entry_t *entry)
{
        strncpy_buf(entry->comment, comment);
        strncpy_buf(entry->pre, pre);
        strncpy_buf(entry->post, post);
}

/******************************************************************************\
 Allocates a new entry.
\******************************************************************************/
static entry_t *entry_new(void)
{
        entry_t *entry;

        entry = calloc(sizeof (*entry), 1);
        strncpy_buf(entry->name, name);
        entry_setup(entry);
        return entry;
}

/******************************************************************************\
 Searches for an entry.
\******************************************************************************/
static entry_t *find_entry(entry_t *entry, const char *name)
{
        while (entry) {
                int cmp;

                cmp = strcmp(name, entry->name);
                if (cmp < 0)
                        break;
                if (!cmp)
                        return entry;
                entry = entry->next;
        }
        return NULL;
}

/******************************************************************************\
 Updates an entry if it exists or creates a new one.
\******************************************************************************/
static void set_entry(entry_t **root)
{
        entry_t *entry, *prev, *cur;
        int cmp;

        /* Check the rest of the linked list */
        prev = NULL;
        cur = *root;
        while (cur) {
                cmp = strcmp(name, cur->name);
                if (cmp < 0)
                        break;
                else if (!cmp) {
                        entry_setup(cur);
                        return;
                }
                prev = cur;
                cur = cur->next;
        }
        if (!prev) {
                entry = entry_new();
                entry->next = *root ? (*root)->next : NULL;
                *root = entry;
                return;
        }
        entry = entry_new();
        entry->next = prev->next;
        prev->next = entry;
}

/******************************************************************************\
 Push the file position forward. It cannot be pushed back!
\******************************************************************************/
static void next(void)
{
        llast_ch = last_ch;
        last_ch = cur_ch;
        cur_ch = next_ch;
        if (cur_ch == EOF) {
                next_ch = EOF;
                return;
        }
        next_ch = (char)fgetc(file);
}

/******************************************************************************\
 Skips spaces.
\******************************************************************************/
static void skip_space(void)
{
        while (cur_ch <= ' ' && cur_ch != EOF)
                next();
}

/******************************************************************************\
 Skip a quote-enclosed string or character sequence.
\******************************************************************************/
static void skip_string(void)
{
        if (cur_ch == '"') {
                next();
                while (cur_ch != EOF && (cur_ch != '\"' || last_ch == '\\'))
                        next();
                next();
        } else if (cur_ch == '\'') {
                next();
                while (cur_ch != EOF && (cur_ch != '\'' || last_ch == '\\'))
                        next();
                next();
        }
}

/******************************************************************************\
 Returns TRUE if a character is valid for this token type.
\******************************************************************************/
static int valid_char(token_t type, int num)
{
        switch (type) {
        case TOKEN_IDENTIFIER:
                return isalpha(cur_ch) || cur_ch == '_';
        case TOKEN_NUMERIC:
                return (!num && cur_ch == '-') ||
                       (cur_ch >= '0' && cur_ch <= '9') ||
                       (num && (isalpha(cur_ch) || cur_ch == '.'));
        case TOKEN_PREPROCESSOR:
                return (!num && cur_ch == '#') || (num && isalpha(cur_ch));
        case TOKEN_STRING:
                return (!num && cur_ch == '"') ||
                       (num && (llast_ch == '\\' || last_ch != '"'));
        case TOKEN_CHARACTER:
                return (!num && cur_ch == '\'') ||
                       (num && (llast_ch == '\'' || last_ch != '\''));
        case TOKEN_OPERATOR:
                return cur_ch == '(' || cur_ch == ')' ||
                       cur_ch == '{' || cur_ch == '}' ||
                       cur_ch == '[' || cur_ch == ']' ||
                       cur_ch == ',' || cur_ch == ';';
        case TOKEN_OP_BLOB:
                return cur_ch == '~' || cur_ch == '|' || cur_ch == '*' ||
                       (cur_ch >= '[' && cur_ch <= '^') ||
                       (cur_ch >= ':' && cur_ch <= '@') ||
                       (cur_ch >= '!' && cur_ch <= '/');
        default:
                printf("Invalid token type %d\n", type);
                abort();
        }
}

/******************************************************************************\
 Checks if a comment just started, parses it and saves it.
\******************************************************************************/
static void check_comment(void)
{
        char *end;
        int i;

        skip_space();
        while (cur_ch == '/' && next_ch == '*') {

                /* Skip long comment boundaries */
                do {
                        next();
                } while (cur_ch == '*' || cur_ch == '\\');
                skip_space();

                /* Save the comment */
                end = comment;
                for (i = 0; i < sizeof (comment) - 1; i++) {
                        comment[i] = cur_ch;
                        if (cur_ch == '\n')
                                skip_space();
                        else
                                next();
                        if (last_ch == '*' && cur_ch == '/')
                                break;
                        if (cur_ch != '\\' && cur_ch != '*')
                                end = comment + i + 1;
                }
                *end = NUL;

                next();
                skip_space();
        }
}

/******************************************************************************\
 Reads a C token out of a file.
\******************************************************************************/
static void read_token(void)
{
        int i;

        check_comment();

        /* Identify token type */
        token_type = 0;
        for (i = 1; i < TOKENS; i++)
                if (valid_char(i, 0)) {
                        token_type = i;
                        break;
                }

        /* Read in the token */
        token[0] = cur_ch;
        next();
        if (token_type == TOKEN_PREPROCESSOR)
                check_comment();
        if (token_type == TOKEN_OPERATOR) {
                token[1] = NUL;
                return;
        }
        for (i = 1; i < sizeof (token) - 1 && valid_char(token_type, i); i++) {
                token[i] = cur_ch;
                next();
        }
        token[i] = NUL;
}

/******************************************************************************\
 Skip a declaration or definition.

 There is a bit of a hack in this function that relies on convention. If we
 are skipping a block that ends with instances like this:

   struct my_struct {
           ...
   } instance1, instance2;

 Then the instances must start on the same line as the closing brace otherwise
 we can't tell them apart from declarations following a function body.
\******************************************************************************/
static void skip_def(void)
{
        int braces;

        for (braces = 0; cur_ch != EOF; next()) {
                skip_string();
                if (cur_ch == '{')
                        braces++;
                if (cur_ch == '}') {
                        braces--;
                        if (braces < 1) {
                                for (; next_ch <= ' '; next())
                                        if (cur_ch == '\n')
                                                return;
                                while (cur_ch != EOF &&
                                       token_type != TOKEN_OPERATOR &&
                                       token[0] != ';')
                                        read_token();
                                return;
                        }
                }
                if (braces < 1 && cur_ch == ';')
                        break;
        }
        next();
}

/******************************************************************************\
 Skip a preprocessor line.
\******************************************************************************/
static void skip_preproc(void)
{
        while (cur_ch != EOF && (cur_ch != '\n' || last_ch == '\\'))
                next();
}

/******************************************************************************\
 Read a preprocessor line.
\******************************************************************************/
static void read_preproc(char *buf, size_t size)
{
        while (cur_ch != EOF && (cur_ch != '\n' || last_ch == '\\')) {
                if (--size <= 0)
                        break;
                if (cur_ch != '\\' || next_ch != '\n')
                        *(buf++) = cur_ch;
                next();
        }
        *buf = NUL;
}

/******************************************************************************\
 Reads everything between a pair of operator delimiters into the post field.
\******************************************************************************/
static void read_until(char *buf, size_t size, char stop)
{
        int string, comment;
        char *end;

        string = FALSE;
        comment = FALSE;
        end = buf + size - 2;
        while (cur_ch != EOF && (string || comment || cur_ch != stop)) {
                if (!comment && cur_ch == '"' && last_ch != '\\')
                        string = !string;
                if (!string && cur_ch == '*' && last_ch == '/')
                        comment = TRUE;
                if (!string && cur_ch == '/' && last_ch == '*')
                        comment = FALSE;
                if (cur_ch == '\n') {
                        strncpy(buf, "<br>", end - buf);
                        buf += 4;
                        if (next_ch != stop) {
                                strncpy(buf, "&nbsp;&nbsp;&nbsp;&nbsp;",
                                        end - buf);
                                buf += 24;
                        }
                } else if (buf < end)
                        *(buf++) = cur_ch;
                next();
        }
        *(buf++) = cur_ch;
        *buf = NUL;
        next();
}

/******************************************************************************\
 Parse a C header file for declarations.
\******************************************************************************/
static void parse_header(const char *filename)
{
        int entries;

        read_token();
        for (entries = 0; cur_ch != EOF; read_token(), entries++) {

                /* Don't bother with local stuff */
                if (!strcmp(token, "static") || !strcmp(token, "extern") ||
                    !strcmp(token, "inline")) {
                        skip_def();
                        continue;
                }

                /* preprocessor */
                if (token_type == TOKEN_PREPROCESSOR) {

                        /* #define (name) (post) */
                        if (!strcmp(token, "#define")) {
                                entry_t **root;

                                read_token();
                                strncpy_buf(pre, "#define");
                                strncpy_buf(name, token);

                                /* This could be a macro function, peek ahead */
                                while (cur_ch == ' ' || cur_ch == '\t')
                                        next();
                                root = &defines;
                                if (cur_ch == '(')
                                        root = &functions;

                                read_preproc(post, sizeof (post));
                                set_entry(root);
                                continue;
                        }

                        skip_preproc();
                        continue;
                }

                /* struct */
                if (!strcmp(token, "struct")) {
                        strncpy_buf(pre, "struct");
                        read_token();
                        strncpy_buf(name, token);
                        read_until(post, sizeof (post), '}');
                        set_entry(&types);
                        skip_def();
                        continue;
                }

                /* typedef */
                if (!strcmp(token, "typedef")) {
                        read_token();

                        /* typedef struct (name) { (post) } (type); */
                        if (!strcmp(token, "struct")) {
                                char struct_name[256];

                                strncpy_buf(pre, "struct");
                                read_token();
                                strncpy_buf(struct_name, token);

                                /* typedef struct (name) (type); */
                                read_token();
                                if (token[0] != '{') {
                                        entry_t *entry;

                                        entry = find_entry(types, struct_name);
                                        if (entry)
                                                strncpy_buf(entry->name, token);
                                        skip_def();
                                        continue;
                                }

                                post[0] = '{';
                                read_until(post + 1, sizeof (post) - 1, '}');
                                read_token();
                                strncpy_buf(name, token);
                                set_entry(&types);
                                skip_def();
                                continue;
                        }

                        /* typedef enum { (post) } (name);
                           typedef union { (post) } (name); */
                        if (!strcmp(token, "enum") || !strcmp(token, "union")) {
                                strncpy_buf(pre, token);
                                read_until(post, sizeof (post), '}');
                                read_token();
                                strncpy_buf(name, token);
                                set_entry(&types);
                                skip_def();
                                continue;
                        }

                        strncpy_buf(pre, token);
                        read_token();

                        /* Pointer stars */
                        if (token_type == TOKEN_OP_BLOB) {
                                strncat(pre, token, sizeof (pre));
                                read_token();
                        }

                        /* typedef (pre) (name); */
                        if (token_type != TOKEN_OPERATOR) {
                                post[0] = NUL;
                                strncpy_buf(name, token);
                                set_entry(&types);
                                skip_def();
                                continue;
                        }

                        /* typedef (pre) ( * (function) ) ( (post) ); */
                        read_token();
                        read_token();
                        strncpy_buf(name, token);
                        read_token();
                        read_until(post, sizeof (post), ')');
                        set_entry(&types);
                        skip_def();
                        continue;
                }

                /* Function type */
                strncpy_buf(pre, token);
                read_token();

                /* Pointer stars */
                if (token_type == TOKEN_OP_BLOB) {
                        strncat(pre, token, sizeof (pre));
                        read_token();
                }

                /* Function identifier */
                strncpy_buf(name, token);

                /* Function arguments */
                read_until(post, sizeof (post), ')');
                set_entry(&functions);
                skip_def();
        }
        printf("    Parsed %d entries.\n", entries);
}

/******************************************************************************\
 Parse a C source file for comments.
\******************************************************************************/
static void parse_source(const char *filename)
{
}

/******************************************************************************\
 Parse an input file.
\******************************************************************************/
static void parse_file(const char *filename)
{
        size_t len;

        file = fopen(filename, "r");
        if (!file) {
                printf("    Failed to open for reading!\n");
                return;
        }
        llast_ch = NUL;
        last_ch = NUL;
        cur_ch = fgetc(file);
        next_ch = fgetc(file);
        len = strlen(filename);
        if (len > 2 && filename[len - 2] == '.' && filename[len - 1] == 'h')
                parse_header(filename);
        else if (len > 2 && filename[len - 2] == '.' &&
                 filename[len - 1] == 'c')
                parse_source(filename);
        else
                printf("    Unrecognized extension.\n");
        fclose(file);
}

/******************************************************************************\
 Prints an entry linked list.
\******************************************************************************/
static void output_entries(entry_t *entry)
{
        while (entry) {
                fprintf(file, "%s <b>%s</b> %s<pre>%s</pre>\n",
                        entry->pre, entry->name, entry->post, entry->comment);
                entry = entry->next;
        }
}

/******************************************************************************\
 Outputs HTML document.
\******************************************************************************/
static void output_html(const char *filename)
{
        file = fopen(filename, "w");
        if (!file) {
                printf("Failed to open for writing!\n");
                return;
        }

        /* Header */
        fprintf(file, "<!-- Generated with GenDoc -->\n<html><body>\n");

        /* Write definitions */
        fprintf(file, "<h1>Definitions</h1>");
        output_entries(defines);

        /* Write types */
        fprintf(file, "<h1>Types</h1>");
        output_entries(types);

        /* Write functions */
        fprintf(file, "<h1>Functions</h1>");
        output_entries(functions);

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
                printf("Usage: gendoc output.html [input headers]"
                       "[input sources]\nYou shouldn't need to run this "
                       "program directly, running 'make' should automatically "
                       "run this with the correct arguments.");
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

