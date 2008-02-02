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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

static c_var_t *root;

/******************************************************************************\
 This function will register a static configurable variable. The data is stored
 in the c_var_t [var], which is guaranteed to always have the specified
 [type] and can be addressed using [name] from within a configuration string.
 Note that [name] must be static and cannot be deallocated! The variable is
 assigned [value].

 These variables cannot be deallocated or have any member other than the value
 modified. The variables are tracked as a linked list.
\******************************************************************************/
void C_register_variable(c_var_t *var, const char *name, c_var_type_t type,
                         c_var_value_t value)
{
        var->next = root;
        var->type = type;
        var->name = name;
        var->value = value;
        root = var;
}

/******************************************************************************\
 Tries to find variable [name] (case insensitive) in the variable linked list.
 Returns NULL if it fails.
\******************************************************************************/
c_var_t *C_resolve_variable(const char *name)
{
        c_var_t *var;

        var = root;
        while (var)
        {
                if (!strcasecmp(var->name, name))
                        return var;
                var = var->next;
        }
        return NULL;
}

/******************************************************************************\
 Parses and sets a variable's value according to its type. [value] can safely
 be deallocated.
\******************************************************************************/
void C_set_variable(c_var_t *var, const char *value)
{
        if (var->type == C_VAR_INTEGER)
                var->value.n = atoi(value);
        else if (var->type == C_VAR_FLOAT)
                var->value.f = atof(value);
        else if (var->type == C_VAR_STRING) {
                var->value.s = strdup(value);
                var->type = C_VAR_STRING_DYNAMIC;
        } else if (var->type == C_VAR_STRING_DYNAMIC) {
                free(var->value.s);
                var->value.s = strdup(value);
                var->type = C_VAR_STRING_DYNAMIC;
        } else
                C_warning("Variable '%s' has invalid type %d",
                          var->name, (int)var->type);
}

/******************************************************************************\
 Reads a string in Quake configuration format:

   // C++ comments allowed
   c_my_var_has_no_spaces "values always in quotes"
   c_numbers_not_in_quotes 123

 Ignores newlines and spaces. Returns FALSE if there was an error (but not a
 warning) while parsing the configuration string.

 TODO: "Concatenate" "strings"
\******************************************************************************/
int C_parse_config(const char *string)
{
        int parsing_name, parsing_string, skipped_comment;
        char name[256], value[4096], *pos;

        /* Start parsing name */
        name[0] = NUL;
        value[0] = NUL;
        pos = name;
        parsing_name = TRUE;

        for (; *string; string++) {
                const char *old_string;

                old_string = string;
                string = C_skip_spaces(string);

                /* Skip comment lines */
                if (!parsing_string && *string == '/' && *(string + 1) == '/') {
                        do {
                                string++;
                        } while (*string && *string != '\n');
                        skipped_comment = TRUE;
                        continue;
                }

                if (parsing_name) {
                        parsing_string = string[0] == '"';

                        /* Did we switch to parsing a value? */
                        if (((skipped_comment || old_string != string) &&
                             isdigit(string[0])) || parsing_string) {
                                if (!name[0]) {
                                        C_warning("Parsing variable value "
                                                  "without a name");
                                        return FALSE;
                                }
                                parsing_name = FALSE;
                                *pos = NUL;
                                pos = value;
                                if (!parsing_string)
                                        string--;
                                continue;
                        }

                        /* Check buffer limit */
                        if (pos - name >= sizeof (name)) {
                                C_warning("Variable '%s' name too long", name);
                                return FALSE;
                        }
                } else {

                        /* Did we switch to parsing a name? */
                        if (!*string ||
                            (!parsing_string &&
                             (*string <= ' ' || !isdigit(*string))) ||
                            (parsing_string && *string == '"' &&
                             *(string - 1) != '\\')) {
                                c_var_t *var;

                                if (!parsing_string)
                                        string--;
                                *pos = NUL;
                                pos = name;
                                parsing_name = TRUE;
                                var = C_resolve_variable(name);
                                if (!var) {
                                        C_warning("variable '%s' not found",
                                                  name);
                                        continue;
                                }
                                C_set_variable(var, value);
                                continue;
                        }

                        /* Check buffer limit */
                        if (pos - value >= sizeof (value)) {
                                C_warning("Variable '%s' value too long", name);
                                return FALSE;
                        }
                }
                *(pos++) = *string;
                skipped_comment = FALSE;
        }
        return TRUE;
}

