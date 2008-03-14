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
#include <SDL_ttf.h>

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

/* If you are going to use the C_va* functions, keep in mind that after calling
   any of those functions [C_VA_BUFFERS] times, you will begin overwriting
   the buffers starting from the first. Each buffer also has a fixed size.
   Note that [C_VA_BUFFERS] * [C_VA_BUFFER_SIZE] has to be less than 32k or
   the function will not compile on some systems. */
#define C_VA_BUFFERS 16
#define C_VA_BUFFER_SIZE 2000

/* All angles should be in radians but there are some cases (OpenGL) where
   conversions are necessary */
#define C_rad_to_deg(a) ((a) * 180 / M_PI)
#define C_deg_to_rad(a) ((a) * M_PI / 180)

/* Certain functions should not be used. Files that legitimately use these
   should undefine these replacements */
#undef calloc
#define calloc(s) ERROR_use_C_calloc
#undef fclose
#define fclose(f) ERROR_use_C_file_close
#undef free
#define free(s) ERROR_use_C_free
#undef malloc
#define malloc(s) ERROR_use_C_malloc
#undef realloc
#define realloc(p, s) ERROR_use_C_realloc
#undef strdup
#define strdup(s) ERROR_use_C_strdup
#undef strncpy
#define strncpy(d, s, n) ERROR_use_C_strncpy

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
        float f;
        char *s;
} c_var_value_t;

/* Variable value types */
typedef enum {
        C_VT_UNREGISTERED,
        C_VT_INTEGER,
        C_VT_FLOAT,

        /* Distinguish statically and dynamically allocated strings */
        C_VT_STRING,
        C_VT_STRING_DYNAMIC,
} c_var_type_t;

/* Variables can be set at different times */
typedef enum {
        C_VE_ANYTIME,
        C_VE_LOCKED,
        C_VE_LATCHED,
} c_var_edit_t;

/* Type for configurable variables */
typedef struct c_var {
        const char *name;
        c_var_value_t value, latched;
        c_var_type_t type;
        c_var_edit_t edit;
        struct c_var *next;
} c_var_t;

/* Resizing arrays */
typedef struct c_array {
        size_t capacity;
        size_t len;
        size_t item_size;
        void *elems;
} c_array_t;

/* Wrap the standard library file I/O */
typedef FILE c_file_t;

/* A structure to hold the data for a file that is being read in tokens */
typedef struct c_token_file {
        c_file_t *file;
        char *pos, *token, swap, filename[32], buffer[4000];
} c_token_file_t;

/* Callback for cleaning up referenced memory */
typedef void (*c_ref_cleanup_f)(void *data);

/* Reference-counted linked-list. Memory allocated using the referenced
   memory allocator must have this structure as the first member! */
typedef struct c_ref {
        char name[256];
        struct c_ref *prev, *next, **root;
        c_ref_cleanup_f cleanup_func;
        int refs;
} c_ref_t;

/* A counter for counting how often something happens per frame */
typedef struct c_counter {
        int start_frame, start_time, last_time;
        float value;
} c_count_t;

/* c_glibc_rand.c */
long int C_glibc_rand(void);
void C_glibc_srand(unsigned int);
#define C_rand() C_glibc_rand()
#define C_rand_seed(s) C_glibc_srand(s)

/* c_log.c */
#define C_assert(s) C_assert_full(__FILE__, __LINE__, __func__, !(s), #s)
void C_assert_full(const char *file, int line, const char *function,
                   int boolean, const char *message);
void C_close_log_file(void);
#define C_debug(fmt, ...) C_log(C_LOG_DEBUG, __FILE__, __LINE__, \
                                __func__, fmt, ## __VA_ARGS__)
#define C_debug_full(f, l, fn, fmt, ...) C_log(C_LOG_DEBUG, f, l, fn, \
                                               fmt, ## __VA_ARGS__)
#define C_error(fmt, ...) C_log(C_LOG_ERROR, __FILE__, __LINE__, \
                                __func__, fmt, ## __VA_ARGS__)
#define C_error_full(f, l, fn, fmt, ...) C_log(C_LOG_ERROR, f, l, \
                                               fn, fmt, ## __VA_ARGS__)
void C_log(c_log_level_t, const char *file, int line,
           const char *function, const char *fmt, ...);
void C_open_log_file(void);
#define C_status(fmt, ...) C_log(C_LOG_STATUS, __FILE__, __LINE__, \
                                 __func__, fmt, ## __VA_ARGS__)
#define C_status_full(f, l, fn, fmt, ...) C_log(C_LOG_STATUS, f, l, \
                                                fn, fmt, ## __VA_ARGS__)
#define C_trace() C_log(C_LOG_TRACE, __FILE__, __LINE__, __func__, NULL)
#define C_trace_full(f, l, fn, fmt, ...) C_log(C_LOG_TRACE, f, l, \
                                               fn, fmt, ## __VA_ARGS__)
#define C_warning(fmt, ...) C_log(C_LOG_WARNING, __FILE__, __LINE__, \
                                  __func__, fmt, ## __VA_ARGS__)
#define C_warning_full(f, l, fn, fmt, ...) C_log(C_LOG_TRACE, f, l, fn, \
                                                 fmt, ## __VA_ARGS__)

/* c_memory.c */
void C_array_append(c_array_t *ary, void *item);
void C_array_cleanup(c_array_t *ary);
#define C_array_elem(ary, type, i) (((type*)(ary)->elems)[i])
#define C_array_init(ary, type, cap) C_array_init_real(ary, sizeof(type), cap)
void C_array_init_real(c_array_t *ary, size_t item_size, size_t cap);
void C_array_reserve(c_array_t *ary, size_t n);
void *C_array_steal(c_array_t *ary);
#define C_calloc(s) C_recalloc_full(__FILE__, __LINE__, __func__, NULL, s)
void C_check_leaks(void);
#define C_free(p) C_free_full(__FILE__, __LINE__, __func__, p)
void C_free_full(const char *file, int line, const char *function, void *ptr);
#define C_malloc(s) C_realloc(NULL, s)
#define C_realloc(p, s) C_realloc_full(__FILE__, __LINE__, __func__, p, s)
void *C_realloc_full(const char *file, int line, const char *function,
                     void *ptr, size_t size);
void *C_recalloc_full(const char *file, int line, const char *function,
                      void *ptr, size_t size);
#define C_ref_alloc(s, r, c, n, f) C_ref_alloc_full(__FILE__, __LINE__, \
                                                    __func__, s, r, c, n, f)
void *C_ref_alloc_full(const char *file, int line, const char *function,
                       size_t, c_ref_t **root, c_ref_cleanup_f,
                       const char *name, int *found);
#define C_ref_down(r) C_ref_down_full(__FILE__, __LINE__, __func__, r);
void C_ref_down_full(const char *file, int line, const char *function,
                     c_ref_t *ref);
#define C_ref_up(r) C_ref_up_full(__FILE__, __LINE__, __func__, r);
void C_ref_up_full(const char *file, int line, const char *function,
                   c_ref_t *ref);
void C_test_mem_check(void);
#define C_zero(s) memset(s, 0, sizeof (*(s)))
#define C_zero_buf(s) memset(s, 0, sizeof (s))

extern c_var_t c_mem_check;

/* c_noise.c */
void C_noise3_seed(unsigned int seed);
float C_noise3(float x, float y, float z);
float C_noise3_fractal(int levels, float x, float y, float z);

/* c_string.c */
#define C_bool_string(b) ((b) ? "TRUE" : "FALSE")
c_color_t C_color_string(const char *);
void C_file_close(c_file_t *);
#define C_file_gets(f, buf) fgets(buf, sizeof (buf), f)
#define C_file_open_read(name) fopen(name, "r")
#define C_file_open_write(name) fopen(name, "w")
#define C_file_read(f, buf, len) fread(buf, 1, len, f)
#define C_file_vprintf(f, fm, v) vfprintf(f, fm, v)
#define C_file_write(f, buf, len) fwrite(buf, 1, len, f)
#define C_is_digit(c) (((c) >= '0' && (c) <= '9') || (c) == '.' || (c) == '-')
#define C_is_print(c) ((c) > 0 && (c) < 0x7f)
#define C_is_space(c) ((c) && (c) <= ' ')
int C_read_file(const char *filename, char *buffer, int size);
char *C_skip_spaces(const char *str);
size_t C_strncpy(char *dest, const char *src, size_t len);
#define C_strncpy_buf(d, s) C_strncpy(d, s, sizeof (d))
void C_token_file_cleanup(c_token_file_t *);
int C_token_file_init(c_token_file_t *, const char *filename);
const char *C_token_file_read_full(c_token_file_t *, int *out_quoted);
#define C_token_file_read(f) C_token_file_read_full(f, NULL)
#define C_strdup(s) C_strdup_full(__FILE__, __LINE__, __func__, s)
char *C_strdup_full(const char *file, int line, const char *func, const char *);
char *C_va(const char *fmt, ...);
char *C_van(int *output_len, const char *fmt, ...);
char *C_vanv(int *output_len, const char *fmt, va_list);

/* c_time.c */
#define C_count_add(c, v) ((c)->value += (v))
float C_count_fps(const c_count_t *);
float C_count_per_frame(const c_count_t *);
float C_count_per_sec(const c_count_t *);
int C_count_poll(c_count_t *, int interval);
void C_count_reset(c_count_t *);
void C_time_init(void);
void C_time_update(void);
unsigned int C_timer(void);

extern unsigned int c_time_msec, c_frame_msec, c_frame;
extern float c_frame_sec;

/* c_variables.c */
void C_cleanup_variables(void);
int C_parse_config(const char *string);
int C_parse_config_file(const char *filename);
#define C_register_float(var, name, value) \
        C_register_variable(var, name, C_VT_FLOAT, \
                           (c_var_value_t)(float)(value))
#define C_register_integer(var, name, value) \
        C_register_variable(var, name, C_VT_INTEGER, \
                            (c_var_value_t)(int)(value))
#define C_register_string(var, name, value) \
        C_register_variable(var, name, C_VT_STRING, \
                            (c_var_value_t)(char *)(value))
#define C_register_string_dynamic(var, name, value) \
        C_register_variable(var, name, C_VT_STRING_DYNAMIC, \
                            (c_var_value_t)(char *)(value))
void C_register_variable(c_var_t *var, const char *name, c_var_type_t type,
                         c_var_value_t value);
void C_register_variables(void);
void C_set_variable(c_var_t *var, const char *value);

