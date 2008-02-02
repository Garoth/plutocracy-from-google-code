/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Project configuration header */
#include "config.h"

/* Common definitions */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#ifndef NULL
#define NULL ((void*)0)
#endif
#ifndef NUL
#define NUL '\0'
#endif

/* Holds all possible variable types */
typedef union  {
        int n;
        double f;
        char *s;
        const char *cs;
} c_var_value_t;

/* Variable value types */
typedef enum {
        C_VAR_INTEGER,
        C_VAR_FLOAT,

        /* Distinguish statically and dynamically allocated strings */
        C_VAR_STRING,
        C_VAR_STRING_DYNAMIC,
} c_var_type_t;

/* Type for configurable variables */
typedef struct c_var {
        const char *name;
        c_var_value_t value;
        c_var_type_t type;
        struct c_var *next;
} c_var_t;

/* c_log.c */
#define C_debug(fmt, ...) C_debug_full(__FILE__, __LINE__, __func__, \
                                       fmt, ## __VA_ARGS__)
void C_debug_full(const char *file, int line, const char *function,
                 const char *fmt, ...);
#define C_error C_debug
#define C_warning C_debug

/* c_string.c */
char *C_skip_spaces(const char *str);

/* c_variables.c */
int C_parse_config(const char *string);
#define C_register_float(var, name, value) \
        C_register_variable(var, name, C_VAR_FLOAT, (c_var_value_t)(value))
#define C_register_integer(var, name, value) \
        C_register_variable(var, name, C_VAR_INTEGER, (c_var_value_t)(value))
#define C_register_string(var, name, value) \
        C_register_variable(var, name, C_VAR_STRING, (c_var_value_t)(value))
#define C_register_string_dynamic(var, name, value) \
        C_register_variable(var, name, C_VAR_STRING_DYNAMIC, \
                            (c_var_value_t)(value))
void C_register_variable(c_var_t *var, const char *name, c_var_type_t type,
                         c_var_value_t value);
void C_set_variable(c_var_t *var, const char *value);

