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

/* SDL/OpenGL variables */
static SDL_PixelFormat sdl_pixel_format;
int gl_pixel_format;

/******************************************************************************\
 Loads a texture using SDL_image. If the texture has already been loaded,
 just ups the reference count and returns the loaded surface.
   TODO: Support loading from sources other than files.
\******************************************************************************/
r_texture_t *R_texture_load(const char *filename)
{
        r_texture_t *prev, *next, *pt;
        SDL_Surface *surface, *surface_rgba;

        /* Find a place for the texture */
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

        /* Load the image file */
        surface = IMG_Load(filename);
        if (!surface) {
                C_warning("Failed to load texture '%s'", filename);
                return NULL;
        }

        /* Convert to our texture format */
        surface_rgba = SDL_ConvertSurface(surface, &sdl_pixel_format,
                                          SDL_SRCALPHA);
        if (!surface_rgba) {
                C_warning("Failed to convert texture '%s'", filename);
                return NULL;
        }
        SDL_FreeSurface(surface);

        /* Allocate a new texture object */
        pt = C_malloc(sizeof (*pt));
        if (!root)
                root = pt;
        pt->prev = prev;
        if (prev)
                pt->prev->next = pt;
        pt->next = next;
        if (next)
                pt->next->prev = pt;
        pt->surface = surface_rgba;
        pt->format = gl_pixel_format;
        pt->refs = 1;
        C_strncpy(pt->filename, filename, sizeof (pt->filename));

        /* Load the texture into OpenGL */
        glGenTextures(1, &pt->gl_name);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA,
                          surface_rgba->w, surface_rgba->h,
                          GL_RGBA, gl_pixel_format, surface_rgba->pixels);

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
        glDeleteTextures(1, &pt->gl_name);
        C_debug("Free'd '%s'", pt->filename);
        C_free(pt);
}

/******************************************************************************\
 Raises a texture's reference count by one. Use this when you have a copy of a
 texture pointer and you expect your pointer will be free'd later.
\******************************************************************************/
void R_texture_ref(r_texture_t *texture)
{
        if (!texture)
                return;
        texture->refs++;
        C_debug("Referenced texture '%s' (%d refs)",
                texture->filename, texture->refs);
}

/******************************************************************************\
 Loads render assets.
\******************************************************************************/
void R_load_assets(void)
{
        C_status("Loading render assets");

        /* Setup the texture pixel format, RGBA in 32 or 16 bits */
        memset(&sdl_pixel_format, 0 , sizeof (sdl_pixel_format));
        if (r_colordepth.value.n > 16) {
                sdl_pixel_format.BitsPerPixel = 32;
                sdl_pixel_format.BytesPerPixel = 4;
                sdl_pixel_format.Rmask = 0xff000000;
                sdl_pixel_format.Gmask = 0x00ff0000;
                sdl_pixel_format.Bmask = 0x0000ff00;
                sdl_pixel_format.Amask = 0x000000ff;
                sdl_pixel_format.Rshift = 24;
                sdl_pixel_format.Gshift = 16;
                sdl_pixel_format.Bshift = 8;
                gl_pixel_format = GL_UNSIGNED_INT_8_8_8_8;
        } else {
                sdl_pixel_format.BitsPerPixel = 16;
                sdl_pixel_format.BytesPerPixel = 2;
                sdl_pixel_format.Rmask = 0xf000;
                sdl_pixel_format.Gmask = 0x0f00;
                sdl_pixel_format.Bmask = 0x00f0;
                sdl_pixel_format.Amask = 0x000f;
                sdl_pixel_format.Rshift = 12;
                sdl_pixel_format.Gshift = 8;
                sdl_pixel_format.Bshift = 4;
                sdl_pixel_format.Rloss = 4;
                sdl_pixel_format.Gloss = 4;
                sdl_pixel_format.Bloss = 4;
                sdl_pixel_format.Aloss = 4;
                gl_pixel_format = GL_UNSIGNED_SHORT_4_4_4_4;
        }

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

