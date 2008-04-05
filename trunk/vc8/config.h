/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Prevents deprecation warnings */
#define _CRT_SECURE_NO_DEPRECATE 1

/* Enables M_PI */
#define _USE_MATH_DEFINES

/* OpenGL uses some Windows stuff */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

/* Constants normally defined via configure.ac */
#define PACKAGE "plutocracy"
#define PACKAGE_STRING "Plutocracy 0.0.0"

/* Compatibility */
#define inline __inline
#define __func__ __FUNCTION__
#define snprintf _snprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp

