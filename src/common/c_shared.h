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

/* Standard library */
#include <stddef.h> /* size_t */
#include <math.h> /* sqrt() */

/* Vectors */
#include "c_vectors.h"

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

/* Debug log levels, errors are fatal and will always abort */
typedef enum {
        C_LOG_ERROR,
        C_LOG_WARNING,
        C_LOG_STATUS,
        C_LOG_DEBUG,
        C_LOG_TRACE,
} c_log_level_t;

/* Holds all possible variable types */
typedef union {
        int n;
        double f;
        char *s;
} c_var_value_t;

/* Variable value types */
typedef enum {
        C_VAR_UNREGISTERED,
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

/* Resizing arrays for fun and profit. This is OH GOD SO UNTYPESAFE. */
typedef struct r_ary {
        size_t capacity;
        size_t len;
        size_t item_size;
        void *elems;
} c_array_t;

/* c_array.c */
#define C_array_elem(pary, type, i) (((type*)(pary)->elems)[i])
#define C_array_init(ary, type, cap) \
        C_array_init_real(ary, sizeof(type), cap)

void C_array_init_real(c_array_t *ary, size_t item_size, size_t cap);
void C_array_reserve(c_array_t *ary, size_t n);
void C_array_append(c_array_t *ary, void* item);
void C_array_deinit(c_array_t* ary);

/* c_log.c */
void C_close_log_file(void);
#define C_debug(fmt, ...) C_debug_full(C_LOG_DEBUG, __FILE__, __LINE__, \
                                       __func__, fmt, ## __VA_ARGS__)
void C_debug_full(c_log_level_t, const char *file, int line,
                  const char *function, const char *fmt, ...);
#define C_error(fmt, ...) C_debug_full(C_LOG_ERROR, __FILE__, __LINE__, \
                                       __func__, fmt, ## __VA_ARGS__)
void C_open_log_file(void);
#define C_status(fmt, ...) C_debug_full(C_LOG_STATUS, __FILE__, __LINE__, \
                                        __func__, fmt, ## __VA_ARGS__)
#define C_trace() C_debug_full(C_LOG_TRACE, __FILE__, __LINE__, __func__, NULL)
#define C_warning(fmt, ...) C_debug_full(C_LOG_WARNING, __FILE__, __LINE__, \
                                         __func__, fmt, ## __VA_ARGS__)

/* c_malloc.c */
void* C_malloc(size_t);
void* C_realloc(void*, size_t);
void C_free_void(void*);
#define C_free(p) do { C_free_void(p); (p) = NULL; } while(FALSE)

/* c_string.c */
#define C_is_digit(c) (((c) >= '0' && (c) <= '9') || c == '.' || c == '-')
int C_read_file(const char *filename, char *buffer, int size);
char *C_skip_spaces(const char *str);

/* c_variables.c */
int C_parse_config(const char *string);
int C_parse_config_file(const char *filename);
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
void C_register_variables(void);
void C_set_variable(c_var_t *var, const char *value);

