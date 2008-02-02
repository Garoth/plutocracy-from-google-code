/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* This file forms the starting point for the client program. The client may
   also function as a server. */

#include "common/c_shared.h"
#include "render/r_shared.h"
#include "SDL.h"

/******************************************************************************\
 Start up the client program from here.
\******************************************************************************/
int main(int argc, char *argv[])
{
        C_debug("Hello World!");
        if(!R_create_window()) {
                C_debug("Window creation failed\n");
                return 1;
        }

        /* Main loop */
        SDL_Event ev;
        int running = TRUE;

        while(running) {
                while(SDL_PollEvent(&ev)) {
                        switch(ev.type) {
                        case SDL_QUIT:
                                running = FALSE;
                                break;

                        default:
                                /* Ignore pretty much all events */
                                break;
                        }
                }

                R_render();
        }

        return 0;
}
