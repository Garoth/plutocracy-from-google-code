This style is based on the Linux kernel with some deviations:
http://pantransit.reptiles.org/prog/CodingStyle.html

# Source Files #

All source files need the license header and must include their namespace common header:
```
/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "r_common.h"
```
The copyright name should only include authors that actually worked on a significant portion of the file.

Large files are difficult to work with. If you find yourself working with a file with more than 1000 lines of code or larger than around 32kb, you need to split some code off of it. This may mean exposing some members or functions in the common header, remember to adjust their names appropriately and make sure the focus of each file is clear and consistent. If `c_my_file.c` grows too large, do not simply move half the code to `c_my_file2.c`!

Each file must have a specific purpose and be named appropriately. No namespace should have a generic "catch-all" source file (e.g. `g_main.c`)! Give the file a short, preferably one-word name general enough to cover the functions and variables within but not so generic as to be ambiguous.

# Spacing #

Use 8-space indents limited to 80-char width. If you run out of room, make a new function or:
```
C_my_obscenely_long_function(wrap, really_long_arguments,
                             like_this, please);
```
Put a space after every keyword (`if`, `while`, `do`, `goto`, `sizeof`, etc) and operator and use K&R braces. Do not put braces for one-line blocks.  Put type definitions at the top of every block and separate them with one blank line. Initialize them below, not in-line.
```
int my_first_int;

my_first_int = 1;
if (my_size > sizeof (c_my_type_t)) {
        int my_second_int;

        my_second_int = 2;
        static_function(my_first_int);
        C_error("Oh snap (%d)!", my_second_int);
}
```
Put one space after pointer types for consistency:
```
char *a_char_ptr, a_char, *another_char_ptr;

char *C_return_char_ptr(char *char_ptr)
{
        return char_ptr;
}
```
Do _not_ write one line blocks or have more than one statement per line:
```
/* Every time you write this sort of thing, Linus kills a kitten: */
if (condition) { short_function(); }
```
Instead, write it out and stay consistent:
```
if (condition)
        short_function();
```
When writing multi-line macros, do not put the backslash at the very end of the line,
it is impossible to maintain proper spacing. Instead, add a space and write the
backslash as if it was an operator:
```
#define A_VERY_LONG_MACRO if (you_like_macros) { \
                                  you.lazy = TRUE; \
                          }
```

# Comments #

At the top of every file, under the license, should be a brief explanation of what the file does. Use a block comment like this:
```
/* Block comments that spread over multiple lines
   should look kind of like this. Use full sentences,
   capitalize, and don't forget periods! */

/* No period for one sentence/short comments */
```
At the top of every function definition (for inline functions in headers too!), use a large fancy header:
```
/******************************************************************************\
 This is a large fancy header. It should describe in plain English what the
 function immediately below it does, how it is implemented, and any other
 considerations that are not immediately obvious. When referring to an [arg]
 use brackets. Don't forget to mention the return value. Generally, you want 
 more comments in here than in the code itself.
\******************************************************************************/
static int my_function(int arg)
{
       /* Comment in the code for clarification only */
       if (C_is_unclear(arg))
              return TRUE;

       return FALSE;
}
```
Do not use end-of-line or C++ style (`//`) comments.

# Namespaces #

Non-static functions and variables have prefixes:
```
c_var_t c_my_common_var;
void C_my_common_function();
```

We have the following namespaces:
```
/* Shared among all code */
void C_common();

/* Client and server game mechanics */
void G_game();

/* Client interface */
void I_interface();

/* Client rendering (OpenGL here only) */
void R_render();

/* Client and server network calls */
void N_network();
```

Static variables and functions are never namespaced:
```
static int my_global;

static void my_func(void);
```

# Common and Shared Headers #

Global variables and functions should be included either in the namespace common or shared header. The common header is only shared within the namespace. The shared header may be included by other namespaces. Globals need a prefix regardless of which header they fall under. Alphabetize headers by file (include comment header naming the file each function/variable came from) then by type and name:
```
/* Includes first */
#include "config.h"

/* Then definitions */
#define TRUE 1

/* Then type definitions */
typedef enum {
        C_TYPE,
} c_type_t;

/* c_afile.c */
void C_afunction();
#define C_function(p) C_function_full(__FILE__, p)
void C_function_full(const char *file, int arg);
void C_zfunction();

extern int avar, bvar, cvar;

/* c_zfile.c */
void C_blah_blah_blah();
```

# Class and Function Names #

Type names are prefixed and end with `_t`, try not to abbreviate names:
```
typedef struct c_my_type c_my_type_t;
```

Function pointer types end with `_f`:
```
typedef void (*c_my_func_f)(void);
```

Class constructors that return the type, not a pointer take the name of the class:
```
c_my_type_t C_my_type();
```

Class functions contain the full name of the class first, then the action verbs (this only applies to classes):
```
void C_my_type_mogrify(c_my_type_t *type, other, args);
```
Functions that allocate and return memory that must be `free`'d should have `_alloc` in the name and have an appropriate `_free` function if they need special treatment to free:
```
c_my_type_t *C_my_type_alloc();
void C_my_type_free(c_my_type_t *);
```
If your type is allocated on the stack and require initialization/cleanup and the actual pointer is not dynamically allocated or `free`'d:
```
void C_my_type_init(c_my_type_t *);
void C_my_type_cleanup(c_my_type_t *);
```
If the function allocates memory and may involve file I/O, use `load` instead of `alloc`:
```
c_my_type_t *C_my_type_load();
```
If you return a pointer that should not be `free`'d, put `get` somewhere in the name:
```
void *C_get_static_pointer();
```

# Standard Library Functions #

A fair number of the standard library functions have been wrapped in `C_` namespace equivalents. Using these functions will generate a compiler warning. Read the top of `c_shared.h` for the full list of these functions:
```
/* Certain functions should not be used. Files that legitimately use these
   should undefine these replacements */
#define calloc(s) ERROR_use_C_calloc
#define fclose(f) ERROR_use_C_file_close
#define free(s) ERROR_use_C_free
#define malloc(s) ERROR_use_C_malloc
#define realloc(p, s) ERROR_use_C_realloc
#define strdup(s) ERROR_use_C_strdup
#define strncpy(d, s, n) ERROR_use_C_strncpy
```