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
 Concatenates an argument array and runs it through the config parser.
\******************************************************************************/
static void parse_config_args(int argc, char *argv[])
{
        int i;
        char buffer[4096], *pos;

        if (argc < 2)
                return;
        buffer[0] = NUL;
        pos = buffer;
        for (i = 1; i < argc; i++) {
                size_t len;

                len = strlen(argv[i]);
                if (pos + len >= buffer + sizeof (buffer)) {
                        C_warning("Command-line config overflowed");
                        return;
                }
                memcpy(pos, argv[i], len);
                pos += len;
                *(pos++) = ' ';
        }
        *pos = NUL;
        C_parse_config(buffer);
}

/******************************************************************************\
 Start up the client program from here.
\******************************************************************************/
int main(int argc, char *argv[])
{
        SDL_Event ev;
        int running;

        C_debug(PACKAGE_STRING " client startup");

        /* Register configurable variables */
        R_register_variables();

        parse_config_args(argc, argv);

        /* Initialize */
        if(!R_create_window()) {
                C_debug("Window creation failed");
                return 1;
        }

        /* Main loop */
        running = TRUE;
        while(running) {
                while(SDL_PollEvent(&ev)) {
                        switch(ev.type) {
                        case SDL_QUIT:
                                running = FALSE;
                                break;
                        default:
                                break;
                        }
                }
                R_render();
        }

        return 0;
}

