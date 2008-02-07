/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Implements model, mesh, and texture loading and management. Textures are
   stored in a linked list and should only be accessed during loading time so
   efficiency here is not important. */

#include "r_common.h"

/* Render testing */
extern c_var_t r_test_mesh, r_test_model, r_test_globe;
r_static_mesh_t *r_test_mesh_data = NULL;
r_model_t *r_test_model_data = NULL;

/* Texture linked list */
static r_texture_t *root;

/******************************************************************************\
 Loads a texture using SDL_image. If the texture has already been loaded,
 just ups the reference count and returns the loaded surface.
   TODO: Support loading from sources other than files.
\******************************************************************************/
r_texture_t *R_texture_load(const char *filename)
{
        r_texture_t *prev, *next, *pt;
        SDL_Surface *surface;

        prev = NULL;
        next = NULL;
        pt = root;
        while (pt) {
                int cmp;

                cmp = strcmp(filename, pt->filename);
                if (!cmp) {
                        pt->refs++;
                        C_debug("Loading '%s', cache hit (%d refs)",
                                filename, pt->refs);
                        return pt;
                }
                if (cmp < 0) {
                        next = pt;
                        pt = NULL;
                        break;
                }
                prev = pt;
                pt = pt->next;
        }
        surface = IMG_Load(filename);
        if (!surface) {
                C_warning("Failed to load texture '%s'", filename);
                return NULL;
        }
        pt = C_malloc(sizeof (*pt));
        if (!root)
                root = pt;
        if (prev) {
                pt->prev = prev;
                pt->prev->next = pt;
        }
        if (next) {
                pt->next = next;
                pt->next->prev = pt;
        }
        pt->surface = surface;
        pt->refs = 1;
        C_debug("Loaded '%s'", filename);
        return pt;
}

/******************************************************************************\
 Frees memory associated with a texture. If there are still hanging references,
 just lowers the reference count.
\******************************************************************************/
void R_texture_free(r_texture_t *pt)
{
        if (!pt)
                return;
        pt->refs--;
        if (pt->refs > 0) {
                C_debug("Dereferenced '%s' (%d refs)", pt->filename, pt->refs);
                return;
        }
        if (root == pt)
                root = pt->next;
        if (pt->prev)
                pt->prev->next = pt->next;
        if (pt->next)
                pt->next->prev = pt->prev;
        SDL_FreeSurface(pt->surface);
        C_debug("Free'd '%s'", pt->filename);
        C_free(pt);
}

/******************************************************************************\
 Loads render assets.
\******************************************************************************/
void R_load_assets(void)
{
        C_status("Loading render assets");

        /* Load the test assets */
        if (r_test_globe.value.n)
                return;
        if (*r_test_model.value.s)
                r_test_model_data = R_model_load(r_test_model.value.s);
        else if (*r_test_mesh.value.s)
                r_test_mesh_data = R_static_mesh_load(r_test_mesh.value.s);
}

/******************************************************************************\
 Cleans up the asset resources.
\******************************************************************************/
void R_free_assets(void)
{
        /* Render testing */
        R_static_mesh_free(r_test_mesh_data);
        R_model_free(r_test_model_data);
}

