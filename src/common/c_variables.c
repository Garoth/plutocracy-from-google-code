/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* This file contains the common configurable variables and the framework
   for handling configurable variables system-wide. Variables are not set
   or referred to by their names often, so efficiency here is not important. */

#include "c_shared.h"

/* Message logging */
c_var_t c_log_level, c_log_file;

/* Never burn the user's CPU, if we are running through frames too quickly
   (faster than this rate), we need to take a nap */
c_var_t c_max_fps, c_show_fps;

/* We can do some detailed allocated memory tracking and detect double-free,
   memory under/overrun, and leaks on the fly. This variable cannot be changed
   after initilization! */
c_var_t c_mem_check;

static c_var_t *root;

/******************************************************************************\
 Registers the common configurable variables.
\******************************************************************************/
void C_register_variables(void)
{
        /* Message logging */
        C_register_integer(&c_log_level, "c_log_level", C_LOG_WARNING);
        C_register_string(&c_log_file, "c_log_file", "");

        /* FPS cap */
        C_register_integer(&c_max_fps, "c_max_fps", 120);
        C_register_integer(&c_show_fps, "c_show_fps", FALSE);

        /* Memory checking */
        C_register_integer(&c_mem_check, "c_mem_check", FALSE);
}

/******************************************************************************\
 Frees dynamically allocated string variables so the leak checker will not
 get angry.
\******************************************************************************/
void C_cleanup_variables(void)
{
        c_var_t *var;

        var = root;
        while (var) {
                if (var->type == C_VT_STRING_DYNAMIC) {
                        C_free(var->value.s);
                        var->value.s = "";
                        var->type = C_VT_STRING;
                }
                var = var->next;
        }
}

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
        if (var->type)
                C_error("Attempted to re-register '%s' with '%s'",
                        var->name, name);
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
 Parses and sets a variable's value according to its type. After this function
 returns [value] can be safely deallocated.
\******************************************************************************/
void C_set_variable(c_var_t *var, const char *value)
{
        c_var_value_t *var_value;

        /* Variable editing rules */
        if (var->edit == C_VE_LOCKED) {
                C_warning("Cannot set '%s' to '%s', variable is locked",
                          var->name, value);
                return;
        }
        else if (var->edit == C_VE_LATCHED)
                var_value = &var->latched;
        else if (var->edit == C_VE_ANYTIME)
                var_value = &var->value;
        else
                C_error("Variable '%s' is uninitialized", var->name);

        /* Set the variable's value */
        if (var->type == C_VT_INTEGER) {
                var_value->n = atoi(value);
                C_debug("Integer '%s' set to %d ('%s')", var->name,
                        var_value->n, value);
        } else if (var->type == C_VT_FLOAT) {
                var_value->f = atof(value);
                C_debug("Float '%s' set to %g ('%s')", var->name,
                        var_value->f, value);
        } else if (var->type == C_VT_STRING) {
                var_value->s = C_strdup(value);
                var->type = C_VT_STRING_DYNAMIC;
                C_debug("Static string '%s' set to '%s'",
                        var->name, var_value->s);
        } else if (var->type == C_VT_STRING_DYNAMIC) {
                C_free(var->value.s);
                var_value->s = C_strdup(value);
                var->type = C_VT_STRING_DYNAMIC;
                C_debug("Dynamic string '%s' set to '%s'",
                        var->name, var_value->s);
        } else
                C_error("Variable '%s' has invalid type %d",
                        var->name, (int)var->type);
}

/******************************************************************************\
 Skips any space characters and comment lines in the string.
\******************************************************************************/
static char *skip_unparseable(const char *string)
{
        char ch;

        for (ch = *string; ch; ch = *(++string)) {

                /* Skip comment lines */
                if (ch == '/') {
                        if (string[1] == '/') {
                                while (ch && ch != '\n')
                                        ch = *(++string);
                                continue;
                        }
                        if (string[1] == '*') {
                                do {
                                        ch = *(++string);
                                } while (ch && (ch != '/' ||
                                                string[-1] != '*'));
                                continue;
                        }
                }

                /* Skip spaces */
                if (ch > ' ')
                        break;
        }
        return (char *)string;
}

/******************************************************************************\
 Reads a string in Quake configuration format:

   // C and C++ comments allowed
   c_my_var_has_no_spaces "values always in quotes"
                          "can be concatenated"
   c_numbers_not_in_quotes -123.5

 Ignores newlines and spaces. Returns FALSE if there was an error (but not a
 warning) while parsing the configuration string.
\******************************************************************************/
int C_parse_config(const char *string)
{
        int parsed, parsing_name, parsing_string;
        char name[256], value[4096], *pos;

        /* Start parsing name */
        name[0] = NUL;
        value[0] = NUL;
        pos = name;
        parsing_name = TRUE;
        parsing_string = FALSE;
        parsed = 0;

        for (;; string++) {
                const char *old_string;

                old_string = string;
                if (!parsing_string)
                        string = skip_unparseable(string);
                if (parsing_name) {
                        parsing_string = *string == '"';

                        /* End of string */
                        if (!*string)
                                break;

                        /* Did we switch to parsing a value? */
                        if ((old_string != string && C_is_digit(string[0])) ||
                            parsing_string) {
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

                        /* Did we finish parsing the value? */
                        if (!*string ||
                            (!parsing_string && !C_is_digit(*string)) ||
                            (parsing_string && *string == '"' &&
                             string[-1] != '\\')) {
                                c_var_t *var;

                                /* Check for concatenation */
                                if (parsing_string) {
                                        string = skip_unparseable(string + 1);
                                        if (*string == '"')
                                                continue;
                                }

                                /* Switch to parsing the name */
                                string--;
                                *pos = NUL;
                                pos = name;
                                parsing_name = TRUE;

                                /* Set the variable */
                                var = C_resolve_variable(name);
                                if (var)
                                        C_set_variable(var, value);
                                else
                                        C_warning("variable '%s' not found",
                                                  name);
                                parsed++;

                                continue;
                        }

                        /* Check buffer limit */
                        if (pos - value >= sizeof (value)) {
                                C_warning("Variable '%s' value too long", name);
                                return FALSE;
                        }
                }

                /* Escape characters in strings */
                if (parsing_string && string[0] == '\\') {
                        if (string[1] == '\n') {
                                string++;
                                continue;
                        } else if (string[1] == 'n') {
                                *(pos++) = '\n';
                                string++;
                                continue;
                        } else if (string[1] == 't') {
                                *(pos++) = '\t';
                                string++;
                                continue;
                        } else if (string[1] == '\\') {
                                *(pos++) = '\\';
                                string++;
                                continue;
                        }
                }

                *(pos++) = *string;
        }
        C_debug("Parsed %d variables", parsed);
        return TRUE;
}

/******************************************************************************\
 Parses a configuration file (see above).
   TODO: Remove the file size limit
   TODO: Multiple file search paths
\******************************************************************************/
int C_parse_config_file(const char *filename)
{
        char buffer[32000];

        if (C_read_file(filename, buffer, sizeof (buffer)) < 0) {
                C_warning("Error reading config '%s'", filename);
                return FALSE;
        }
        return C_parse_config(buffer);
}

