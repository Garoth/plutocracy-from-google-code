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

unsigned int c_time_msec, c_frame_msec, c_frame;
float c_frame_sec;

/******************************************************************************\
 Initializes counters and timers.
\******************************************************************************/
void C_time_init(void)
{
        c_frame = 1;
        c_time_msec = SDL_GetTicks();
}

/******************************************************************************\
 Updates the current time. This needs to be called exactly once per frame.
\******************************************************************************/
void C_time_update(void)
{
        static unsigned int last_msec;

        c_time_msec = SDL_GetTicks();
        c_frame_msec = c_time_msec - last_msec;
        c_frame_sec = c_frame_msec / 1000.f;
        last_msec = c_time_msec;
        c_frame++;
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

/******************************************************************************\
 Initializes a counter structure.
\******************************************************************************/
void C_count_reset(c_count_t *counter)
{
        counter->last_time = c_time_msec;
        counter->start_frame = c_frame;
        counter->start_time = c_time_msec;
        counter->value = 0.f;
}

/******************************************************************************\
 Polls a counter to see if [interval] msec have elapsed since the last poll.
 Returns TRUE if the counter is ready to be polled and sets the poll time.
\******************************************************************************/
int C_count_poll(c_count_t *counter, int interval)
{
        if (c_time_msec - counter->last_time < interval)
                return FALSE;
        counter->last_time = c_time_msec;
        return TRUE;
}

/******************************************************************************\
 Returns the per-frame count of a counter.
\******************************************************************************/
float C_count_per_frame(const c_count_t *counter)
{
        int frames;

        frames = c_frame - counter->start_frame;
        if (frames < 1)
                return 0.f;
        return counter->value / frames;
}

/******************************************************************************\
 Returns the per-second count of a counter.
\******************************************************************************/
float C_count_per_sec(const c_count_t *counter)
{
        float seconds;

        seconds = (c_time_msec - counter->start_time) / 1000.f;
        if (seconds <= 0.f)
                return 0.f;
        return counter->value / seconds;
}

/******************************************************************************\
 Returns the average frames-per-second while counter was running.
\******************************************************************************/
float C_count_fps(const c_count_t *counter)
{
        float seconds;

        seconds = (c_time_msec - counter->start_time) / 1000.f;
        if (seconds <= 0.f)
                return 0.f;
        return (c_frame - counter->start_frame) / seconds;
}

