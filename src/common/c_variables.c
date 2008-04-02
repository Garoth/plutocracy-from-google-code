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

/* This should be set to TRUE when the main loop needs to exit properly */
int c_exit;

static c_var_t *root;

/******************************************************************************\
 Registers the common configurable variables.
\******************************************************************************/
void C_register_variables(void)
{
        /* Message logging */
        C_register_integer(&c_log_level, "c_log_level", C_LOG_WARNING);
        c_log_level.edit = C_VE_ANYTIME;
        C_register_string(&c_log_file, "c_log_file", "");

        /* FPS cap */
        C_register_integer(&c_max_fps, "c_max_fps", 120);
        c_max_fps.edit = C_VE_ANYTIME;
        C_register_integer(&c_show_fps, "c_show_fps", FALSE);
        c_show_fps.edit = C_VE_ANYTIME;

        /* Memory checking */
        C_register_integer(&c_mem_check, "c_mem_check", FALSE);
        c_mem_check.archive = FALSE;
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
                C_var_unlatch(var);
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
void C_var_register(c_var_t *var, const char *name, c_var_type_t type,
                    c_var_value_t value)
{
        if (var->type)
                C_error("Attempted to re-register '%s' with '%s'",
                        var->name, name);
        var->next = root;
        var->type = type;
        var->name = name;
        var->stock = var->latched = var->value = value;
        var->edit = C_VE_LATCHED;
        var->archive = TRUE;
        root = var;
}

void C_register_float(c_var_t *var, const char *name, float value_f)
{
        c_var_value_t value;

        value.f = value_f;
        C_var_register(var, name, C_VT_FLOAT, value);
}

void C_register_integer(c_var_t *var, const char *name, int value_n)
{
        c_var_value_t value;

        value.n = value_n;
        C_var_register(var, name, C_VT_INTEGER, value);
}

void C_register_string(c_var_t *var, const char *name, const char *value_s)
{
        c_var_value_t value;

        value.s = (char *)value_s;
        C_var_register(var, name, C_VT_STRING, value);
}

/******************************************************************************\
 Tries to find variable [name] (case insensitive) in the variable linked list.
 Returns NULL if it fails.
\******************************************************************************/
c_var_t *C_resolve_var(const char *name)
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
void C_var_set(c_var_t *var, const char *string)
{
        c_var_value_t *var_value, new_value;
        const char *set_string;

        /* Variable editing rules */
        if (var->edit == C_VE_LOCKED) {
                C_warning("Cannot set '%s' to '%s', variable is locked",
                          var->name, string);
                return;
        }
        var_value = &var->value;
        if (var->edit == C_VE_LATCHED)
                var_value = &var->latched;

        /* Get the new value and see if it changed */
        switch (var->type) {
        case C_VT_INTEGER:
                new_value.n = atoi(string);
                if (new_value.n == var_value->n)
                        return;
                break;
        case C_VT_FLOAT:
                new_value.f = (float)atof(string);
                if (new_value.f == var_value->f)
                        return;
                break;
        case C_VT_STRING:
        case C_VT_STRING_DYNAMIC:
                new_value.s = (char *)string;
                if (!strcmp(var_value->s, string))
                        return;
                break;
        default:
                C_error("Variable '%s' is uninitialized", var->name);
        }

        /* If this variable is controlled by an update function, call it now */
        if (var->edit == C_VE_FUNCTION) {
                if (!var->update)
                        C_error("Update function not set for '%s'", var->name);
                if (!var->update(var, new_value))
                        return;
        }

        /* Set the variable's value */
        set_string = "set to";
        if (var->edit == C_VE_LATCHED) {
                var->has_latched = TRUE;
                set_string = "latched";
                if (var->type == C_VT_STRING && var->value.s != var->latched.s)
                        C_free(var->latched.s);
        }
        switch (var->type) {
        case C_VT_INTEGER:
                *var_value = new_value;
                C_debug("Integer '%s' %s %d", var->name,
                        set_string, new_value.n);
                break;
        case C_VT_FLOAT:
                *var_value = new_value;
                C_debug("Float '%s' %s %g", var->name,
                        set_string, new_value.f);
                break;
        case C_VT_STRING_DYNAMIC:
                C_free(var_value->s);
        case C_VT_STRING:
                new_value.s = C_strdup(string);
                if (var->edit != C_VE_LATCHED)
                        var->type = C_VT_STRING_DYNAMIC;
                *var_value = new_value;
                C_debug("String '%s' %s '%s'", var->name,
                        set_string, new_value.s);
        default:
                break;
        }
}

/******************************************************************************\
 If the variable has a latched value.
\******************************************************************************/
void C_var_unlatch(c_var_t *var)
{
        if (var->type == C_VT_UNREGISTERED)
                C_error("Tried to unlatch an unregistered variable");
        if (!var->has_latched || var->edit != C_VE_LATCHED)
                return;
        var->value = var->latched;
        var->has_latched = FALSE;
        if (var->type == C_VT_STRING)
                var->type = C_VT_STRING_DYNAMIC;
}

/******************************************************************************\
 Prints out the current and latched values of a variable.
\******************************************************************************/
static void print_var(const c_var_t *var)
{
        const char *value, *latched;

        switch (var->type) {
        case C_VT_INTEGER:
                value = C_va("Integer '%s' is %d", var->name, var->value.n);
                break;
        case C_VT_FLOAT:
                value = C_va("Float '%s' is %g", var->name, var->value.f);
                break;
        case C_VT_STRING:
        case C_VT_STRING_DYNAMIC:
                value = C_va("String '%s' is '%s'", var->name, var->value.s);
                break;
        default:
                C_error("Tried to print out unregistered variable");
        }
        latched = "";
        if (var->edit == C_VE_LATCHED && var->has_latched) {
                switch (var->type) {
                case C_VT_INTEGER:
                        latched = C_va(" (%d latched)", var->latched.n);
                        break;
                case C_VT_FLOAT:
                        latched = C_va(" (%g latched)", var->latched.f);
                        break;
                case C_VT_STRING:
                case C_VT_STRING_DYNAMIC:
                        latched = C_va(" ('%s' latched)", var->latched.s);
                default:
                        break;
                }
        }
        C_print(C_va("%s%s", value, latched));
}

/******************************************************************************\
 Reads a string in Quake configuration format:

   // C and C++ comments allowed
   c_my_var_has_no_spaces "values always in quotes"
                          "can be concatenated"
   c_numbers_not_in_quotes -123.5

 Ignores newlines and spaces. Variable names entered without values will have
 their current values printed out.
\******************************************************************************/
static void parse_config_token_file(c_token_file_t *tf)
{
        c_var_t *var;
        const char *token;
        int quoted;
        char value[2048];

        var = NULL;
        value[0] = NUL;
        for (;;) {
                token = C_token_file_read_full(tf, &quoted);
                if (!token[0] && !quoted)
                        break;
                if (!C_is_digit(token[0]) && !quoted) {
                        if (var) {
                                if (value[0])
                                        C_var_set(var, value);
                                else
                                        print_var(var);
                        }
                        var = C_resolve_var(token);
                        if (!var)
                                C_print(C_va("No variable named '%s'", token));
                        value[0] = NUL;
                        continue;
                }
                strncat(value, token, sizeof (value));
        }
        if (var) {
                if (value[0])
                        C_var_set(var, value);
                else
                        print_var(var);
        }
}

/******************************************************************************\
 Parses a configuration file.
\******************************************************************************/
int C_parse_config_file(const char *filename)
{
        c_token_file_t tf;

        if (!C_token_file_init(&tf, filename))
                return FALSE;
        parse_config_token_file(&tf);
        return TRUE;
}

/******************************************************************************\
 Parses a configuration file from a string.
\******************************************************************************/
void C_parse_config_string(const char *string)
{
        c_token_file_t tf;

        C_token_file_init_string(&tf, string);
        parse_config_token_file(&tf);
}

/******************************************************************************\
 Writes a configuration file that contains the values of all modified,
 archivable variables.
\******************************************************************************/
void C_write_autogen(void)
{
        c_file_t *file;
        c_var_t *var;

        file = C_file_open_write(C_va("%s/autogen.cfg", C_user_dir()));
        if (!file) {
                C_warning("Failed to save variable config");
                return;
        }
        C_file_printf(file, "/*************************************************"
                            "*****************************\\\n"
                            " %s - Automatically generated config\n"
                            "\\************************************************"
                            "******************************/\n\n",
                            PACKAGE_STRING);
        for (var = root; var; var = var->next) {
                if (var->archive) {
                        const char *value;

                        C_var_unlatch(var);
                        value = NULL;
                        switch (var->type) {
                        case C_VT_INTEGER:
                                if (var->value.n == var->stock.n)
                                        continue;
                                value = C_va("%d", var->value.n);
                                break;
                        case C_VT_FLOAT:
                                if (var->value.f == var->stock.f)
                                        continue;
                                value = C_va("%g", var->value.f);
                                break;
                        case C_VT_STRING:
                        case C_VT_STRING_DYNAMIC:
                                if (!strcmp(var->value.s, var->stock.s))
                                        continue;
                                value = C_escape_string(var->value.s);
                                break;
                        default:
                                C_error("Unregistered variable in list");
                        }
                        if (value)
                                C_file_printf(file, "%s %s\n",
                                              var->name, value);
                }
        }
        C_file_printf(file, "\n");
        C_file_close(file);
        C_debug("Saved autogen config");
}

