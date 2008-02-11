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
extern c_var_t r_test_mesh_path, r_test_model_path, r_test_globe;
r_static_mesh_t *r_test_mesh;
r_model_t r_test_model;

/* Texture linked list */
static c_ref_t *root;

/* SDL/OpenGL variables */
static SDL_PixelFormat sdl_pixel_format;
int gl_pixel_format;

/******************************************************************************\
 Frees memory associated with a texture.
\******************************************************************************/
static void texture_cleanup(r_texture_t *pt)
{
        SDL_FreeSurface(pt->surface);
        glDeleteTextures(1, &pt->gl_name);
}

/******************************************************************************\
 Loads a texture using SDL_image. If the texture has already been loaded,
 just ups the reference count and returns the loaded surface.
   TODO: Support loading from sources other than files.
\******************************************************************************/
r_texture_t *R_texture_load(const char *filename)
{
        SDL_Surface *surface, *surface_rgba;
        r_texture_t *pt;
        int found;

        if (!filename || !filename[0])
                return NULL;

        /* Find a place for the texture */
        pt = C_ref_alloc(sizeof (*pt), &root, (c_ref_cleanup_f)texture_cleanup,
                         filename, &found);
        if (found)
                return pt;

        /* Load the image file */
        surface = IMG_Load(filename);
        if (!surface) {
                C_warning("Failed to load texture '%s'", filename);
                return NULL;
        }

        /* Convert to our texture format */
        surface_rgba = SDL_ConvertSurface(surface, &sdl_pixel_format,
                                          SDL_SRCALPHA);
        SDL_FreeSurface(surface);
        if (!surface_rgba) {
                C_warning("Failed to convert texture '%s'", filename);
                return NULL;
        }
        pt->surface = surface_rgba;
        pt->format = gl_pixel_format;

        /* Load the texture into OpenGL */
        glGenTextures(1, &pt->gl_name);
        glBindTexture(GL_TEXTURE_2D, pt->gl_name);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGBA,
                          surface_rgba->w, surface_rgba->h,
                          GL_RGBA, gl_pixel_format, surface_rgba->pixels);
        R_check_errors();

        return pt;
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
        if (*r_test_model_path.value.s)
                R_model_init(&r_test_model, r_test_model_path.value.s);
        else if (*r_test_mesh_path.value.s)
                r_test_mesh = R_static_mesh_load(r_test_mesh_path.value.s);
}

/******************************************************************************\
 Cleans up the asset resources.
\******************************************************************************/
void R_free_assets(void)
{
        /* Render testing */
        R_static_mesh_free(r_test_mesh);
        R_model_cleanup(&r_test_model);
}

