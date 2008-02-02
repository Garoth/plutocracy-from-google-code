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

/******************************************************************************\
 Cleans up the SDL window resources.
\******************************************************************************/
static void R_close_window(void)
{
        C_debug("");
        SDL_Quit();
}

/******************************************************************************\
 Initializes some OpenGL stuff
\******************************************************************************/
static void R_init_gl_state(void)
{
        glViewport(0, 0, 800, 600);

        /* Make sure our matrices start sane. */

        /* The range of z values here is arbitrary and shoud be
           modified to fit ranges that we typically have. */
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(90.0, 800.0 / 600.0, 1.0, 10000.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        /* Backface culling. Only rasterize polygons that are facing you. */
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        /* Clear to black. */
        glClearColor(0.0, 0.0, 0.0, 1.0);
}

/******************************************************************************\
 Creates the client window. Returns TRUE on success.
\******************************************************************************/
int R_create_window(void)
{
        SDL_Surface *screen;

        C_debug("Hello World 2!");

        /* Just video for now. */
        if(SDL_Init(SDL_INIT_VIDEO) < 0) {
                return FALSE;
        }

        /* TODO: have configurable stuffs */

        /* Set GL options: just something for now I guess
         *  - 32bit colour
         *  - 4-bit stencil
         *  - double-buffered
         *  - 16b depth
         */
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

        /* Open context. 800x600 for now. */
        screen = SDL_SetVideoMode(800, 600, 0, SDL_OPENGL);
        if(!screen) {
                SDL_Quit();
                return FALSE;
        }

        /* Make sure it's cleaned up. */
        atexit(R_close_window);

        R_init_gl_state();

        return TRUE;
}

