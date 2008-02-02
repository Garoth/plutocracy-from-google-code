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

/* Make sure TRUE and FALSE are defined */
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* c_log.c */
#define C_debug(fmt, ...) C_debug_full(__FILE__, __LINE__, __func__, \
                                       fmt ## __VA_ARGS__)
void C_debug_full(const char *file, int line, const char *function,
                 const char *fmt, ...);

/* c_variables.c */
int C_parse_config(const char *string);
void C_register_double(double *var, const char *path);
void C_register_integer(int *var, const char *path);
void C_register_string(char *var, const char *path);

