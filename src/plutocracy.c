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

extern c_var_t c_max_fps;

/******************************************************************************\
 This is the client's graphical main loop.
\******************************************************************************/
static void main_loop(void)
{
        SDL_Event ev;
        c_count_t fps, throttled;
        int desired_msec, wait_msec;

        C_status("Main loop");
        C_time_init();
        C_count_init(&fps);
        C_count_init(&throttled);

        /* Calculate the desired frame msec.
             FIXME: I don't know why, but we need to wait twice as long as
                    the mathematically correct rate...? */
        if (c_max_fps.value.n < 1)
                c_max_fps.value.n = 1;
        desired_msec = 2000 / c_max_fps.value.n;
        wait_msec = 0;

        for (;;) {
                while (SDL_PollEvent(&ev)) {
                        switch(ev.type) {
                        case SDL_KEYDOWN:
                                if (ev.key.keysym.sym == SDLK_ESCAPE) {
                                        C_debug("Escape key pressed");
                                        return;
                                }
                                break;
                        case SDL_QUIT:
                                return;
                        default:
                                break;
                        }
                }
                R_render();
                C_time_update();

                /* Throttle framerate if vsync is broken. SDL_Delay() will
                   only work in 10ms increments on some systems so it is
                   necessary to break down our delays into bite-sized chunks. */
                if (c_frame_msec < desired_msec) {
                        int msec, delay;

                        msec = desired_msec - c_frame_msec;
                        C_count_add(&throttled, msec);
                        wait_msec += msec;
                        delay = wait_msec / 10;
                        if (delay > 0) {
                                SDL_Delay(delay);
                                wait_msec -= delay;
                        }
                }

                /* Print FPS to console periodically */
                C_count_add(&fps, 1);
                if (C_count_poll(&fps, 20000))
                        C_debug("Frame %d, %.1f FPS (%.1f%% throttled), "
                                "%.1f faces/frame",
                                c_frame, C_count_per_sec(&fps),
                                100.f * C_count_per_frame(&throttled) /
                                        desired_msec,
                                C_count_per_frame(&r_count_faces));
        }
        C_debug("Exited main loop");
}

/******************************************************************************\
 Concatenates an argument array and runs it through the config parser.
\******************************************************************************/
static void parse_config_args(int argc, char *argv[])
{
        int i;
        char buffer[4096], *pos;

        if (argc < 2)
                return;
        C_status("Parsing command line");
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
 Called when the program quits normally or is killed by a signal and should
 perform an orderly cleanup.
\******************************************************************************/
static void cleanup(void)
{
        static int ran_once;

        /* Don't cleanup twice */
        if (ran_once) {
                C_warning("Cleanup already called");
                return;
        }
        ran_once = TRUE;

        C_status("Cleaning up");
        R_render_cleanup();
        SDL_Quit();
        C_check_leaks();
        C_debug("Done");
}

/******************************************************************************\
 Start up the client program from here.
\******************************************************************************/
int main(int argc, char *argv[])
{
        /* Use the cleanup function instead of lots of atexit() calls to
           control the order of cleanup */
        atexit(cleanup);

        /* Each namespace must register its configurable variables */
        C_register_variables();
        R_register_variables();

        /* Parse configuration scripts and open the log file */
        C_parse_config_file("config/default.cfg");
        parse_config_args(argc, argv);
        C_open_log_file();

        /* Run tests if they are enabled */
        C_test_mem_check();

        /* Initialize */
        C_status("Initializing " PACKAGE_STRING " client");
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
                C_error("Failed to initialize SDL: %s", SDL_GetError());
        if (!R_render_init())
                C_error("Window creation failed");

        /* Run the main loop */
        main_loop();

        return 0;
}

