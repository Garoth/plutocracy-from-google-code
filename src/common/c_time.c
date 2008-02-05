/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Contains code that uses SDL timers for various purposes */

#include "c_shared.h"

unsigned int c_time_msec;
float c_frame_sec;

/******************************************************************************\
 Updates the current time.
\******************************************************************************/
void C_time_update(void)
{
        static unsigned int last_msec;

        c_time_msec = SDL_GetTicks();
        c_frame_sec = (c_time_msec - last_msec) / 1000.f;
        last_msec = c_time_msec;
}

/******************************************************************************\
 Returns the time since the last call to C_timer(). Useful for measuring the
 efficiency of sections of code.
\******************************************************************************/
unsigned int C_timer(void)
{
        static unsigned int last_msec;
        unsigned int elapsed;

        elapsed = SDL_GetTicks() - last_msec;
        last_msec += elapsed;
        return elapsed;
}

