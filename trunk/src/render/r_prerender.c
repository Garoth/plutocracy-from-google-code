/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Contains routines for pre-rendering textures to the back buffer and reading
   them back in before the main rendering loop begins. Because of the special
   mode set for these operations, normal rendering calls should not be used. */

#include "r_common.h"

/* Proportion of the tile height that the isoceles triangle face takes up */
#define ISO_PROP 0.859375f

r_texture_t *r_tile_tex[4];

static r_texture_t *blend_mask;

/******************************************************************************\
 Finishes a buffer of pre-rendered textures. If testing is enabled, flips
 buffer to display the pre-rendered buffer and pauses in a fake event loop
 until Escape is pressed.
\******************************************************************************/
static void finish_buffer(void)
{
        SDL_Event ev;

        if (r_test_prerender.value.n) {
                SDL_GL_SwapBuffers();
                for (;;) {
                        while (SDL_PollEvent(&ev)) {
                                switch(ev.type) {
                                case SDL_QUIT:
                                        C_error("Interrupted during "
                                                "prerendering");
                                case SDL_KEYDOWN:
                                        if (ev.key.keysym.sym == SDLK_ESCAPE)
                                                goto done;
                                default:
                                        break;
                                }
                        }
                        C_throttle_fps();
                }
        }
done:   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        R_check_errors();
}

/******************************************************************************\
 Locks the back buffer, allocates a new texture, and blits to it a rectangle
 from the upper left-hand corner of the back buffer. You must manually upload
 the texture after calling this function.
\******************************************************************************/
static r_texture_t *save_buffer(int w, int h)
{
        SDL_Surface *video;
        r_texture_t *tex;

        video = SDL_GetVideoSurface();
        tex = R_texture_alloc(w, h, TRUE);
        if (SDL_LockSurface(tex->surface) < 0)
                C_error("Failed to lock texture: %s", SDL_GetError());
        glReadPixels(0, r_height.value.n - h, w, h, GL_RGBA, GL_UNSIGNED_BYTE,
                     tex->surface->pixels);
        R_surface_flip_v(tex->surface);
        R_check_errors();
        SDL_UnlockSurface(tex->surface);
        return tex;
}

/******************************************************************************\
 Set the tile vertex uv coordinates. If [tx] or [ty] is less than zero, no
 tile shift and scaling is performed.
\******************************************************************************/
static void set_tile_verts_uv(r_vertex2_t *verts, int iteration, int tx, int ty)
{
        c_vec2_t size, shift;
        float pixel_v, pixel_h;
        int i;

        /* Thanks to OpenGL coordinate weirdness, depending on the orientation
           of the triangle we need to be off-by-one pixel sometimes */
        pixel_h = (float)R_TILE_SHEET_W / r_tile_tex[0]->surface->w;
        pixel_v = (float)R_TILE_SHEET_H / r_tile_tex[0]->surface->h;

        switch (iteration) {
        case 1: /* 60 degree rotation */
                verts[0].uv = C_vec2(pixel_h, ISO_PROP);
                verts[1].uv = C_vec2(0.5f, 1.f - pixel_v);
                verts[2].uv = C_vec2(1.f - pixel_h, ISO_PROP);
                verts[3].uv = C_vec2(0.5f, pixel_v);
                verts[4].uv = C_vec2(pixel_h, pixel_v);
                verts[5].uv = C_vec2(1.f - pixel_h, ISO_PROP / 2.f);
                verts[6].uv = C_vec2(0.75f, pixel_v);
                break;

        default:
        case 2: /* Original rotation */
                verts[0].uv = C_vec2(0.5f, 0.f);
                verts[1].uv = C_vec2(0.f, 0.f);
                verts[2].uv = C_vec2(0.f, ISO_PROP);
                verts[3].uv = C_vec2(1.f, ISO_PROP);
                verts[4].uv = C_vec2(1.f, 0.f);
                verts[5].uv = C_vec2(0.f, 1.f);
                verts[6].uv = C_vec2(1.f, 1.f);
                break;

        case 3: /* 120 degree rotation */
                verts[0].uv = C_vec2(1.f - pixel_h, ISO_PROP);
                verts[1].uv = C_vec2(1.f - pixel_h, pixel_v);
                verts[2].uv = C_vec2(0.5f, pixel_v);
                verts[3].uv = C_vec2(pixel_h, ISO_PROP);
                verts[4].uv = C_vec2(0.5f, 1.f - pixel_v);
                verts[5].uv = C_vec2(0.25f, pixel_v);
                verts[6].uv = C_vec2(pixel_h, ISO_PROP / 2.f);
                break;
        }

        /* If this is a member of a tile sheet, shift and scale the uv
           coordinates to cover the tile */
        if (tx < 0 || ty < 0)
                return;
        size = C_vec2(R_TILE_SHEET_W, R_TILE_SHEET_H);
        shift = C_vec2((float)tx / R_TILE_SHEET_W, (float)ty / R_TILE_SHEET_H);
        for (i = 0; i < 7; i++) {
                verts[i].uv = C_vec2_div(verts[i].uv, size);
                verts[i].uv = C_vec2_add(verts[i].uv, shift);
        }
}

/******************************************************************************\
 Renders the tile sheet back to the frame buffer.
\******************************************************************************/
static void test_tile_sheet(int i)
{
        r_vertex2_t verts[4];
        unsigned short indices[] = {0, 1, 2, 3};

        if (!r_test_prerender.value.n)
                return;
        R_texture_select(r_tile_tex[i]);
        verts[0].co = C_vec3((float)r_tile_tex[i]->surface->w, 0.f, 0.f);
        verts[0].uv = C_vec2(0.f, 0.f);
        verts[1].co = C_vec3((float)r_tile_tex[i]->surface->w,
                             (float)r_tile_tex[i]->surface->h, 0.f);
        verts[1].uv = C_vec2(0.f, 1.f);
        verts[2].co = C_vec3(r_tile_tex[i]->surface->w * 2.f,
                             (float)r_tile_tex[i]->surface->h, 0.f);
        verts[2].uv = C_vec2(1.f, 1.f);
        verts[3].co = C_vec3(r_tile_tex[i]->surface->w * 2.f, 0.f, 0.f);
        verts[3].uv = C_vec2(1.f, 0.f);
        glPushMatrix();
        glLoadIdentity();
        glInterleavedArrays(R_VERTEX2_FORMAT, 0, verts);
        glDrawElements(GL_QUADS, 4, GL_UNSIGNED_SHORT, indices);
        glPopMatrix();
}

/******************************************************************************\
 Pre-renders the extra rotations of the tile mask and the terrain tile sheets.
 The tiles are rendered on this vertex arrangement:

   1--0--4
   | / \ |
   |/   \|
   2-----3
   |  \  |
   5-----6

 This function will generate tile textures 1-3.
\******************************************************************************/
static void prerender_tiles(void)
{
        r_vertex2_t verts[7];
        r_texture_t *rotated_mask;
        float width, height;
        int i, x, y;
        unsigned short indices[] = {
                0, 1, 2,
                0, 2, 3,
                0, 3, 4,
                2, 5, 6,
                2, 6, 3,
        };

        /* Vertices are aligned in the tile so that the top vertex of the
           triangle touches the top center of the tile */
        width = r_tile_tex[0]->surface->w / R_TILE_SHEET_W;
        height = r_tile_tex[0]->surface->h / R_TILE_SHEET_H;
        verts[0].co = C_vec3(width / 2.f, 0.f, 0.f);
        verts[1].co = C_vec3(0.f, 0.f, 0.f);
        verts[2].co = C_vec3(0.f, height * ISO_PROP, 0.f);
        verts[3].co = C_vec3(width, height * ISO_PROP, 0.f);
        verts[4].co = C_vec3(width, 0.f, 0.f);
        verts[5].co = C_vec3(0.f, height, 0.f);
        verts[6].co = C_vec3(width, height, 0.f);

        rotated_mask = NULL;
        glPushMatrix();
        for (i = 1; i < 4; i++) {
                glLoadIdentity();

                /* Render the blend mask at tile size and orientation */
                set_tile_verts_uv(verts, i, -1, -1);
                R_texture_select(blend_mask);
                glInterleavedArrays(R_VERTEX2_FORMAT, 0, verts);
                glDrawElements(GL_TRIANGLES, 15, GL_UNSIGNED_SHORT, indices);
                rotated_mask = save_buffer(width, height);
                R_texture_upload(rotated_mask, TRUE);
                finish_buffer();

                /* Render the tiles at this orientation and blend them with
                   the mask */
                R_texture_select(r_tile_tex[0]);
                for (y = 0; y < R_TILE_SHEET_H; y++)
                        for (x = 0; x < R_TILE_SHEET_W; x++) {
                                set_tile_verts_uv(verts, 0, x, y);
                                glLoadIdentity();
                                glTranslatef(x * width, y * height, 0.f);
                                glInterleavedArrays(R_VERTEX2_FORMAT, 0, verts);
                                glDrawElements(GL_TRIANGLES, 15,
                                               GL_UNSIGNED_SHORT, indices);
                        }
                r_tile_tex[i] = save_buffer(width * R_TILE_SHEET_W,
                                            height * R_TILE_SHEET_H);
                R_surface_mask(r_tile_tex[i]->surface, rotated_mask->surface);
                R_texture_free(rotated_mask);
                R_texture_upload(r_tile_tex[i], TRUE);
                test_tile_sheet(i);
                finish_buffer();
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glPopMatrix();
}

/******************************************************************************\
 Renders the pre-render textures to the back buffer and reads them back in for
 later use.
\******************************************************************************/
void R_init_prerender(void)
{
        C_status("Pre-rendering textures");
        C_var_unlatch(&r_test_prerender);

        /* Initialize with a custom 2D mode */
        R_set_mode(R_MODE_NONE);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.f, r_width.value.n, r_height.value.n, 0.f, -1.f, 1.f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        /* Clear to black */
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* Load textures */
        blend_mask = R_texture_load("models/globe/mask.png", TRUE);
        r_tile_tex[0] = R_texture_load("models/globe/terrain.png", TRUE);
        if (!blend_mask || !r_tile_tex[0])
                C_error("Failed to load essential prerendering assets");

        prerender_tiles();
        R_texture_free(blend_mask);
}

/******************************************************************************\
 Cleans up textures loaded and created during initialization.
\******************************************************************************/
void R_cleanup_prerender(void)
{
        int i;

        for (i = 0; i < 4; i++)
                R_texture_free(r_tile_tex[i]);
}

