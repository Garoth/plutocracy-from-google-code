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
   them back in before the main rendering loop begins */

#include "r_common.h"

r_texture_t *r_terrain_blend[3];

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
                                        return;
                                case SDL_KEYDOWN:
                                        if (ev.key.keysym.sym == SDLK_ESCAPE)
                                                return;
                                default:
                                        break;
                                }
                        }
                        C_throttle_fps();
                }
        }
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

/******************************************************************************\
 Pre-renders terrain blend tile sheets.
\******************************************************************************/
static void prerender_terrain(void)
{
        finish_buffer();
}

/******************************************************************************\
 Renders the pre-render textures to the back buffer and reads them back in for
 later use.
\******************************************************************************/
void R_prerender(void)
{
        C_status("Pre-rendering textures");
        C_var_unlatch(&r_test_prerender);

        /* Clear to black */
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* Initialize with 2D mode */
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.f, r_width.value.n, r_height.value.n, 0.f, -1.f, 1.f);
        glDisable(GL_LIGHTING);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        prerender_terrain();
}

