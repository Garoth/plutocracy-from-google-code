/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* This file implements the PLUM (Plutocracy Model) model loading and rendering
   functions. */

#include "r_common.h"

/******************************************************************************\
 Allocate memory for and load a model and its textures.
\******************************************************************************/
r_model_t *R_model_load(const char *filename)
{
        c_token_file_t token_file;

        C_token_file_init(&token_file, filename);
        C_debug("1st token = '%s'", C_token_file_read(&token_file));
        C_debug("2nd token = '%s'", C_token_file_read(&token_file));
        C_debug("3rd token = '%s'", C_token_file_read(&token_file));
        C_token_file_cleanup(&token_file);
        return NULL;
}

/******************************************************************************\
 Free memory used by a model and decrease the reference count of its textures.
\******************************************************************************/
void R_model_free(r_model_t *model)
{
}

