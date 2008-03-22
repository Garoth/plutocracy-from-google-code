/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Some common definitions */
#ifndef TRUE
#define TRUE 1
#endif

/* Type for configurable variables */
struct c_var {
        const char *name;
        struct c_var *next;
        c_var_value_t value, latched;
        c_var_type_t type;
        c_var_edit_t edit;
        c_var_update_f update;
        char has_latched;
};

/* Variable value types. Note that there is a difference between statically and
   dynamically allocated strings. */
typedef enum {
        C_VT_UNREGISTERED,
        C_VT_INTEGER,
        C_VT_FLOAT,
        C_VT_STRING,
        C_VT_STRING_DYNAMIC,
} c_var_type_t;

typedef struct c_var durka_t;

/* Wrap the standard library file I/O */
typedef FILE c_file_t;

/* Callback for GUI log handler */
typedef void (*c_log_event_f)(c_log_level_t, int margin, const char *);

/* Type to hold neighbours of a vertex */
typedef struct g_vert_neighbors {
    int count;
    unsigned short indices[6]; /* Never more than 6 neighbours. */
} g_vert_neighbors_t;

struct test_struct {
        int a;
};


/* A comment */
#define macro_function()
first_t my_function(int a, float b, second_t);
int my_other_func(void);
char *a_sort_up(char *arg, char **b);

