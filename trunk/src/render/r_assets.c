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
static SDL_PixelFormat sdl_rgb_fmt, sdl_rgba_fmt;
int gl_rgb_type, gl_rgba_type;

/******************************************************************************\
 Frees memory associated with a texture.
\******************************************************************************/
static void texture_cleanup(r_texture_t *pt)
{
        SDL_FreeSurface(pt->surface);
        glDeleteTextures(1, &pt->gl_name);
}

/******************************************************************************\
 Gets a pixel from a locked SDL surface.
\******************************************************************************/
static Uint32 get_surface_pixel(const SDL_Surface *surf, int x, int y)
{
        Uint8 *p;
        int bpp;

        bpp = surf->format->BytesPerPixel;
        p = (Uint8 *)surf->pixels + y * surf->pitch + x * bpp;
        switch(bpp) {
        case 1: return *p;
        case 2: return *(Uint16 *)p;
        case 3: if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
                        return p[0] << 16 | p[1] << 8 | p[2];
                else
                        return p[0] | p[1] << 8 | p[2] << 16;
        case 4: return *(Uint32 *)p;
        default:
                return 0;
        }
}

/******************************************************************************\
 Iterates over an SDL surface and checks if any pixels actually use alpha
 blending. Returns TRUE if at least one pixel has alpha less than full.
\******************************************************************************/
static int surface_uses_alpha(SDL_Surface *surf)
{
        int x, y, bpp;

        if (SDL_LockSurface(surf) < 0) {
                C_warning("Failed to lock surface");
                return TRUE;
        }
        bpp = surf->format->BytesPerPixel;
        for (y = 0; y < surf->h; y++)
                for (x = 0; x < surf->w; x++) {
                        Uint32 pixel;
                        Uint8 r, g, b, a;

                        pixel = get_surface_pixel(surf, x, y);
                        SDL_GetRGBA(pixel, surf->format, &r, &g, &b, &a);
                        if (a < 255) {
                                SDL_UnlockSurface(surf);
                                return TRUE;
                        }
                }
        SDL_UnlockSurface(surf);
        return FALSE;
}

/******************************************************************************\
 Loads a texture using SDL_image. If the texture has already been loaded,
 just ups the reference count and returns the loaded surface.
   TODO: Support loading from sources other than files.
\******************************************************************************/
r_texture_t *R_texture_load(const char *filename)
{
        SDL_Surface *surface, *surface_new;
        SDL_PixelFormat *sdl_format;
        r_texture_t *pt;
        int found, flags, gl_internal, gl_format, gl_type;

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
        flags = SDL_HWSURFACE;
        if (surface_uses_alpha(surface)) {
                gl_type = gl_rgba_type;
                gl_format = GL_RGBA;
                gl_internal = GL_RGBA;
                sdl_format = &sdl_rgba_fmt;
                flags |= SDL_SRCALPHA;
                pt->alpha = TRUE;
        } else {
                gl_format = r_color_bits.value.n > 16 ? GL_RGBA : GL_RGB;
                gl_type = gl_rgb_type;
                gl_internal = GL_RGB;
                sdl_format = &sdl_rgb_fmt;
                pt->alpha = FALSE;
        }
        surface_new = SDL_ConvertSurface(surface, sdl_format, flags);
        SDL_FreeSurface(surface);
        if (!surface_new) {
                C_warning("Failed to convert texture '%s'", filename);
                return NULL;
        }
        pt->surface = surface_new;

        /* Load the texture into OpenGL */
        glGenTextures(1, &pt->gl_name);
        glBindTexture(GL_TEXTURE_2D, pt->gl_name);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        gluBuild2DMipmaps(GL_TEXTURE_2D, gl_internal,
                          surface_new->w, surface_new->h,
                          gl_format, gl_type, surface_new->pixels);
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
        memset(&sdl_rgb_fmt, 0 , sizeof (sdl_rgb_fmt));
        if (r_color_bits.value.n > 16) {

                /* RGBA */
                sdl_rgba_fmt.BitsPerPixel = 32;
                sdl_rgba_fmt.BytesPerPixel = 4;
                sdl_rgba_fmt.Rmask = 0xff000000;
                sdl_rgba_fmt.Gmask = 0x00ff0000;
                sdl_rgba_fmt.Bmask = 0x0000ff00;
                sdl_rgba_fmt.Amask = 0x000000ff;
                sdl_rgba_fmt.Rshift = 24;
                sdl_rgba_fmt.Gshift = 16;
                sdl_rgba_fmt.Bshift = 8;
                gl_rgba_type = GL_UNSIGNED_INT_8_8_8_8;

                /* FIXME: Just using RGBA format, is there even a plain RGB? */
                sdl_rgb_fmt = sdl_rgba_fmt;
                gl_rgb_type = gl_rgba_type;

                C_debug("Using 32-bit textures");
        } else {

                /* RGB */
                sdl_rgb_fmt.BitsPerPixel = 16;
                sdl_rgb_fmt.BytesPerPixel = 2;
                sdl_rgb_fmt.Rmask = 0xf800;
                sdl_rgb_fmt.Gmask = 0x07e0;
                sdl_rgb_fmt.Bmask = 0x001f;
                sdl_rgb_fmt.Rshift = 11;
                sdl_rgb_fmt.Gshift = 5;
                sdl_rgb_fmt.Rloss = 3;
                sdl_rgb_fmt.Gloss = 2;
                sdl_rgb_fmt.Bloss = 3;
                gl_rgb_type = GL_UNSIGNED_SHORT_5_6_5;

                /* RGBA */
                sdl_rgba_fmt.BitsPerPixel = 16;
                sdl_rgba_fmt.BytesPerPixel = 2;
                sdl_rgba_fmt.Rmask = 0xf000;
                sdl_rgba_fmt.Gmask = 0x0f00;
                sdl_rgba_fmt.Bmask = 0x00f0;
                sdl_rgba_fmt.Amask = 0x000f;
                sdl_rgba_fmt.Rshift = 12;
                sdl_rgba_fmt.Gshift = 8;
                sdl_rgba_fmt.Bshift = 4;
                sdl_rgba_fmt.Rloss = 4;
                sdl_rgba_fmt.Gloss = 4;
                sdl_rgba_fmt.Bloss = 4;
                sdl_rgba_fmt.Aloss = 4;
                gl_rgba_type = GL_UNSIGNED_SHORT_4_4_4_4;

                C_debug("Using 16-bit textures");
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

