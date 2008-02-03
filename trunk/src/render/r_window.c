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

/* Video parameters */
extern c_var_t r_width, r_height, r_colordepth, r_depth, r_windowed, r_vsync;

/* Model testing */
extern c_var_t r_mesh;
r_static_mesh_t* r_mesh_data = NULL;

/******************************************************************************\
 Cleans up the SDL window resources.
\******************************************************************************/
void R_close_window(void)
{
        SDL_Quit();
}

/******************************************************************************\
 Initializes some OpenGL stuff.
\******************************************************************************/
static void R_init_gl_state(void)
{
        glViewport(0, 0, r_width.value.n, r_height.value.n);

        /* Make sure our matrices start sane. */

        /* The range of z values here is arbitrary and shoud be
           modified to fit ranges that we typically have. */
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        float aspect = (float)r_width.value.n / r_height.value.n;
        gluPerspective(90.0, aspect, 1.0, 10000.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        /* Backface culling. Only rasterize polygons that are facing you. */
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CW);

        /* Clear to black. */
        glClearColor(0.0, 0.0, 0.0, 1.0);

        /* depth testing */
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
}

/******************************************************************************\
 Creates the client window. Returns TRUE on success.
\******************************************************************************/
int R_create_window(void)
{
        SDL_Surface *screen;

        C_status("Opening window");

        /* Just video for now. */
        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
                C_debug("Failed to initialize SDL: %s", SDL_GetError());
                return FALSE;
        }

        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, r_depth.value.n);
        SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, r_vsync.value.n);

        int cdepth = r_colordepth.value.n;
        if(cdepth <= 16) {
                C_warning("r_colordepth has invalid value %d, using 16",
                          cdepth);

                SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
                SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
                SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
        } else {
                SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
                SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
                SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);

                if(cdepth < 24) {
                        C_warning("r_colordepth has invalid value %d, "
                                  "using 24",
                                  cdepth);
                } else if(cdepth > 24) {
                        if(cdepth != 32) {
                                C_warning("r_colordepth has invalid value"
                                          " %d, using 32",
                                          cdepth);
                        }
                        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
                }
        }


        /* Open context. */
        int flags = SDL_OPENGL;
        if(!r_windowed.value.n) {
                flags |= SDL_FULLSCREEN;
        }

        screen = SDL_SetVideoMode(r_width.value.n,
                                  r_height.value.n,
                                  0,
                                  flags);
        if(!screen) {
                C_debug("Failed to get context: %s", SDL_GetError());
                SDL_Quit();
                return FALSE;
        }

        R_init_gl_state();

        if(*r_mesh.value.s) {
                /* Not empty, load the mesh. */
                r_mesh_data = R_load_static_mesh(r_mesh.value.s);
        }

        return TRUE;
}

