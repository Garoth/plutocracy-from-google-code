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

static r_vertex2_t verts[7];
static c_vec2_t tile, sheet;
static unsigned short indices[] = { 0, 1, 2,  0, 2, 3,  0, 3, 4,
                                    2, 5, 6,  2, 6, 3 };

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
                                case SDL_KEYDOWN:
                                        if (ev.key.keysym.sym != SDLK_ESCAPE)
                                                goto done;
                                case SDL_QUIT:
                                        C_error("Interrupted during "
                                                "prerendering");
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
 Sets up the vertex uv coordinates for rendering the blending mask.
\******************************************************************************/
static void set_mask_verts_uv(int iteration)
{
        switch (iteration) {
        default:
                C_error("Invalid iteration");

        case -1:
                /* Original rotation, vertical flip */
                verts[0].uv = C_vec2(0.5f, R_ISO_PROP);
                verts[1].uv = C_vec2(0.f, R_ISO_PROP);
                verts[2].uv = C_vec2(0.f, 0.f);
                verts[3].uv = C_vec2(1.f, 0.f);
                verts[4].uv = C_vec2(1.f, R_ISO_PROP);
                verts[5].uv = C_vec2(0.f, 0.f);
                verts[6].uv = C_vec2(1.f, 0.f);
                break;

        case 0: /* Original rotation */
                verts[0].uv = C_vec2(0.5f, 0.f);
                verts[1].uv = C_vec2(0.f, 0.f);
                verts[2].uv = C_vec2(0.f, R_ISO_PROP);
                verts[3].uv = C_vec2(1.f, R_ISO_PROP);
                verts[4].uv = C_vec2(1.f, 0.f);
                verts[5].uv = C_vec2(0.f, 1.f);
                verts[6].uv = C_vec2(1.f, 1.f);
                break;

        case 1: /* 120 degree rotation */
                verts[0].uv = C_vec2(1.f, R_ISO_PROP);
                verts[1].uv = C_vec2(1.f, 0.f);
                verts[2].uv = C_vec2(0.5f, 0.f);
                verts[3].uv = C_vec2(0.f, R_ISO_PROP);
                verts[4].uv = C_vec2(0.5f, 1.f);
                verts[5].uv = C_vec2(0.25f, 0.f);
                verts[6].uv = C_vec2(0.f, R_ISO_PROP - 0.25f);
                break;

        case 2: /* 60 degree rotation */
                verts[0].uv = C_vec2(0.f, R_ISO_PROP);
                verts[1].uv = C_vec2(0.5f, 1.f);
                verts[2].uv = C_vec2(1.f, R_ISO_PROP);
                verts[3].uv = C_vec2(0.5f, 0.f);
                verts[4].uv = C_vec2(0.f, 0.f);
                verts[5].uv = C_vec2(1.f, R_ISO_PROP - 0.25f);
                verts[6].uv = C_vec2(0.75f, 0.f);
                break;
        }
}

/******************************************************************************\
 Sets up the vertex uv coordinates for rendering a tile. The indices here
 have to match with those in set_mask_vertices(), don't change one without
 the other or the tiles will have seams.
\******************************************************************************/
static void set_tile_verts_uv(int iteration, int tx, int ty)
{
        c_vec2_t size, shift;
        int i;

        switch (iteration) {
        default:
                C_error("Invalid iteration");

        case -1:
                /* Original orientation */
                set_mask_verts_uv(0);
                break;

        case 0: /* Flip over top vertex */
                verts[0].uv = C_vec2(0.5f, 0.f);
                verts[1].uv = C_vec2(1.f, 0.f);
                verts[2].uv = C_vec2(1.f, R_ISO_PROP);
                verts[3].uv = C_vec2(0.f, R_ISO_PROP);
                verts[4].uv = C_vec2(0.f, 0.f);
                verts[5].uv = C_vec2(1.f, 1.f);
                verts[6].uv = C_vec2(0.f, 1.f);
                break;

        case 1: /* Flip over bottom-left vertex */
                verts[0].uv = C_vec2(1.f, R_ISO_PROP);
                verts[1].uv = C_vec2(0.75f, 0.f);
                verts[2].uv = C_vec2(0.f, R_ISO_PROP);
                verts[3].uv = C_vec2(0.5f, 0.f);
                verts[4].uv = C_vec2(1.f, 0.f);
                verts[5].uv = C_vec2(0.f, R_ISO_PROP - 0.25f);
                verts[6].uv = C_vec2(0.25f, 0.f);
                break;

        case 2: /* Flip over bottom-right vertex */
                verts[0].uv = C_vec2(0.f, R_ISO_PROP);
                verts[1].uv = C_vec2(0.f, 0.f);
                verts[2].uv = C_vec2(0.5f, 0.f);
                verts[3].uv = C_vec2(1.f, R_ISO_PROP);
                verts[4].uv = C_vec2(0.25f, 1.f);
                verts[5].uv = C_vec2(0.25f, 0.f);
                verts[6].uv = C_vec2(1.f, R_ISO_PROP - 0.25f);
                break;
        }

        /* Shift and scale the uv coordinates to cover the tile */
        size = C_vec2_div(sheet, tile);
        shift = C_vec2((float)tx / R_TILE_SHEET_W, (float)ty / R_TILE_SHEET_H);
        for (i = 0; i < 7; i++) {
                verts[i].uv = C_vec2_div(verts[i].uv, size);
                verts[i].uv = C_vec2_add(verts[i].uv, shift);
        }
}

/******************************************************************************\
 Renders the tile vertices.
\******************************************************************************/
static void prerender_tile(void)
{
        glInterleavedArrays(R_VERTEX2_FORMAT, 0, verts);
        glDrawElements(GL_TRIANGLES, 15, GL_UNSIGNED_SHORT, indices);
}

/******************************************************************************\
 Pre-renders the extra rotations of the tile mask and the terrain tile sheets.
 This function will generate tile textures 1-3.
\******************************************************************************/
static void prerender_tiles(void)
{
        r_texture_t *blend_mask, *rotated_mask, *masked_terrain;
        int i, x, y;

        blend_mask = R_texture_load("models/globe/blend_mask.png", FALSE);
        if (!blend_mask || !r_terrain_tex)
                C_error("Failed to load essential prerendering assets");

        rotated_mask = NULL;
        glPushMatrix();
        for (i = 0; i < 3; i++) {
                glLoadIdentity();

                /* Render the blend mask at tile size and orientation */
                set_mask_verts_uv(i);
                R_texture_select(blend_mask);
                prerender_tile();
                rotated_mask = save_buffer(tile.x, tile.y);
                R_texture_upload(rotated_mask, TRUE);
                finish_buffer();

                /* Render the tiles at this orientation */
                R_texture_select(r_terrain_tex);
                for (y = 0; y < R_TILE_SHEET_H; y++)
                        for (x = 0; x < R_TILE_SHEET_W; x++) {
                                set_tile_verts_uv(i, x, y);
                                glLoadIdentity();
                                glTranslatef(x * tile.x, y * tile.y, 0.f);
                                prerender_tile();
                        }

                /* Read the terrain back in and mask it */
                masked_terrain = save_buffer(sheet.x, sheet.y);
                R_surface_mask(masked_terrain->surface, rotated_mask->surface);
                R_texture_free(rotated_mask);

                /* Render the masked terrain on top of the terrain texture
                   and read it back in to replace the old terrain texture */
                R_texture_render(r_terrain_tex, 0, 0);
                R_texture_upload(masked_terrain, TRUE);
                R_texture_render(masked_terrain, 0, 0);
                if (r_test_prerender.value.n)
                        R_texture_render(masked_terrain, sheet.x, 0);
                R_texture_free(masked_terrain);
                R_texture_free(r_terrain_tex);
                r_terrain_tex = save_buffer(sheet.x, sheet.y);
                R_texture_upload(r_terrain_tex, TRUE);
                finish_buffer();
        }
        R_texture_free(blend_mask);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glPopMatrix();
}

/******************************************************************************\
 Pre-renders tiles for transition between base tile types.
\******************************************************************************/
static void prerender_transitions(void)
{
        r_texture_t *trans_mask, *inverted_mask, *large_mask, *masked_terrain;
        int y, x, tiles_a[] = {0, 1, 1, 2}, tiles_b[] = {1, 0, 2, 1};

        trans_mask = R_texture_load("models/globe/trans_mask.png", FALSE);
        if (!trans_mask || !r_terrain_tex)
                C_error("Failed to load essential prerendering assets");

        /* Render an inverted version of the mask */
        R_texture_select(trans_mask);
        set_mask_verts_uv(-1);
        prerender_tile();
        inverted_mask = save_buffer(tile.x, tile.y);
        R_surface_invert(inverted_mask->surface, TRUE, FALSE);
        R_texture_upload(inverted_mask, FALSE);
        finish_buffer();

        /* Render the transition mask tile sheet */
        glPushMatrix();
        for (y = 1; y < 5; y++) {
                if (y & 1)
                        R_texture_select(trans_mask);
                else
                        R_texture_select(inverted_mask);
                for (x = 0; x < 3; x++) {
                        glLoadIdentity();
                        glTranslatef(x * tile.x, y * tile.y, 0.f);
                        set_mask_verts_uv(x);
                        prerender_tile();
                }
        }
        glPopMatrix();
        large_mask = save_buffer(sheet.x, sheet.y);
        R_texture_free(trans_mask);
        R_texture_free(inverted_mask);
        finish_buffer();

        /* Render the masked layer of the terrain and read it back in */
        R_texture_select(r_terrain_tex);
        glPushMatrix();
        for (y = 1; y < 5; y++)
                for (x = 0; x < 3; x++) {
                        glLoadIdentity();
                        glTranslatef(x * tile.x, y * tile.y, 0.f);
                        set_tile_verts_uv(-1, tiles_b[y - 1], 0);
                        prerender_tile();
                }
        glPopMatrix();
        masked_terrain = save_buffer(sheet.x, sheet.y);
        R_surface_mask(masked_terrain->surface, large_mask->surface);
        R_texture_free(large_mask);
        R_texture_upload(masked_terrain, FALSE);
        if (r_test_prerender.value.n)
                R_texture_render(masked_terrain, sheet.x, 0);

        /* Render the base layer of the terrain */
        R_texture_render(r_terrain_tex, 0, 0);
        R_texture_select(r_terrain_tex);
        glPushMatrix();
        for (y = 1; y < 5; y++)
                for (x = 0; x < 3; x++) {
                        glLoadIdentity();
                        glTranslatef(x * tile.x, y * tile.y, 0.f);
                        set_tile_verts_uv(-1, tiles_a[y - 1], 0);
                        prerender_tile();
                }
        glPopMatrix();

        /* Render the masked terrain over the tile sheet and read it back in */
        R_texture_render(masked_terrain, 0, 0);
        R_texture_free(masked_terrain);
        R_texture_free(r_terrain_tex);
        r_terrain_tex = save_buffer(sheet.x, sheet.y);
        R_texture_upload(r_terrain_tex, TRUE);

        finish_buffer();
}

/******************************************************************************\
 Renders the pre-render textures to the back buffer and reads them back in for
 later use. The tiles are rendered on this vertex arrangement:

   1--0--4
   | / \ |
   |/   \|
   2-----3
   |  \  |
   5-----6

\******************************************************************************/
void R_prerender(void)
{
        C_status("Pre-rendering textures");
        C_var_unlatch(&r_test_prerender);

        /* Initialize with a custom 2D mode */
        r_mode_hold = TRUE;
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.f, r_width.value.n, r_height.value.n, 0.f, -1.f, 1.f);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        /* Clear to black */
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* Vertices are aligned in the tile so that the top vertex of the
           triangle touches the top center of the tile */
        sheet.x = r_terrain_tex->surface->w;
        sheet.y = r_terrain_tex->surface->h;
        tile.x = r_terrain_tex->surface->w / R_TILE_SHEET_W;
        tile.y = r_terrain_tex->surface->h / R_TILE_SHEET_H;
        verts[0].co = C_vec3(tile.x / 2.f, 0.f, 0.f);
        verts[1].co = C_vec3(0.f, 0.f, 0.f);
        verts[2].co = C_vec3(0.f, tile.y * R_ISO_PROP, 0.f);
        verts[3].co = C_vec3(tile.x, tile.y * R_ISO_PROP, 0.f);
        verts[4].co = C_vec3(tile.x, 0.f, 0.f);
        verts[5].co = C_vec3(0.f, tile.y, 0.f);
        verts[6].co = C_vec3(tile.x, tile.y, 0.f);

        prerender_tiles();
        prerender_transitions();
        r_mode_hold = FALSE;
}

