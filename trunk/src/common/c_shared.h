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
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

/* OpenGL */
#include <GL/gl.h>
#include <GL/glu.h>

/* SDL */
#include <SDL.h>
#include <SDL_image.h>

/* TODO: PhysicsFS? Do we need it or no? */
/* #include <physfs.h> */

/* Vectors */
#include "c_vectors.h"

/* Ensure common definitions */
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

/* Wrap file access functions */
#define C_file_close(f) fclose(f)
#define C_file_open_read(name) fopen(name, "r")
#define C_file_open_write(name) fopen(name, "w")
#define C_file_read(f, buf, len) fread(buf, 1, len, f)
#define C_file_write(f, buf, len) fwrite(buf, 1, len, f)

#define c_file_t FILE

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

/* Resizing arrays */
typedef struct c_array {
        size_t capacity;
        size_t len;
        size_t item_size;
        void *elems;
} c_array_t;

/* A structure to hold the data for a file that is being read in tokens */
typedef struct c_token_file {
        c_file_t *file;
        char *pos, *token, swap, filename[32], buffer[4000];
} c_token_file_t;

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

/* c_memory.c */
void C_array_append(c_array_t *ary, void *item);
void C_array_cleanup(c_array_t *ary);
#define C_array_elem(ary, type, i) (((type*)(ary)->elems)[i])
#define C_array_init(ary, type, cap) C_array_init_real(ary, sizeof(type), cap)
void C_array_init_real(c_array_t *ary, size_t item_size, size_t cap);
void C_array_reserve(c_array_t *ary, size_t n);
void *C_array_steal(c_array_t *ary);
#define C_calloc(s) C_recalloc(NULL, s)
#define C_free(p) free(p)
#define C_malloc(s) C_realloc(NULL, s)
#define C_realloc(p, s) C_realloc_full(__FILE__, __LINE__, __func__, p, s)
void *C_realloc_full(const char *file, int line, const char *function,
                     void *ptr, size_t size);
void *C_recalloc_full(const char *file, int line, const char *function,
                      void *ptr, size_t size);

/* c_string.c */
#define C_is_digit(c) (((c) >= '0' && (c) <= '9') || c == '.' || c == '-')
#define C_is_space(c) ((c) && (c) <= ' ')
int C_read_file(const char *filename, char *buffer, int size);
char *C_skip_spaces(const char *str);
size_t C_strncpy(char *dest, const char *src, size_t len);
void C_token_file_cleanup(c_token_file_t *);
int C_token_file_init(c_token_file_t *, const char *filename);
const char *C_token_file_read_full(c_token_file_t *, int *out_quoted);
#define C_token_file_read(f) C_token_file_read_full(f, NULL)

/* c_time.c */
void C_time_update(void);
unsigned int C_timer(void);

extern unsigned int c_time_msec;
extern float c_frame_sec;

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

