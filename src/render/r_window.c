/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin, Devin Papineau

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "r_common.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include "SDL.h"
#include <stdlib.h>

/* Mesh render testing */
extern c_var_t r_test_mesh;
r_static_mesh_t *r_test_mesh_data = NULL;

/******************************************************************************\
 Loads render assets.
\******************************************************************************/
void R_load_assets(void)
{
        C_status("Loading render assets");

        /* Load the test mesh */
        if (*r_test_mesh.value.s)
                r_test_mesh_data = R_static_mesh_load(r_test_mesh.value.s);
}

/******************************************************************************\
 Cleans up the asset resources.
\******************************************************************************/
void R_free_assets(void)
{
        R_static_mesh_free(r_test_mesh_data);
}

/******************************************************************************\
 Initializes OpenGL settings such view matrices, culling, and depth testing.
\******************************************************************************/
static void R_init_gl_state(void)
{
        glViewport(0, 0, r_width.value.n, r_height.value.n);

        /* The range of z values here is arbitrary and shoud be
           modified to fit ranges that we typically have. */
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(90.0, (float)r_width.value.n / r_height.value.n,
                       1.0, 10000.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        /* Backface culling. Only rasterize polygons that are facing you. */
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CW);

        /* Clear to black. */
        glClearColor(0.0, 0.0, 0.0, 1.0);

        /* Depth testing */
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
}

/******************************************************************************\
 Creates the client window. Returns TRUE on success.
\******************************************************************************/
int R_create_window(void)
{
        SDL_Surface *screen;
        int flags;

        C_status("Opening window");

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, r_depth.value.n);
        SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, r_vsync.value.n);
        if (r_colordepth.value.n == 16) {
                SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
                SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
                SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
        } else {
                SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
                SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
                SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
                if (r_colordepth.value.n != 24)
                        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        }

        /* Open context. */
        flags = SDL_OPENGL;
        if (!r_windowed.value.n)
                flags |= SDL_FULLSCREEN;
        screen = SDL_SetVideoMode(r_width.value.n, r_height.value.n, 0, flags);
        if (!screen) {
                C_warning("Failed to get context: %s", SDL_GetError());
                return FALSE;
        }
        R_init_gl_state();

        return TRUE;
}

