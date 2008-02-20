/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "i_common.h"
#include "../render/r_shared.h"

extern c_var_t i_window_bg;

static r_window_t test_window;

/******************************************************************************\
 Loads interface assets and initializes windows.
\******************************************************************************/
void I_init(void)
{
        R_window_init(&test_window, i_window_bg.value.s);
        test_window.sprite.origin = C_vec2(r_width_2d / 4, r_height_2d / 4);
        test_window.sprite.size = C_vec2(200.f, 300.f);
}

/******************************************************************************\
 Free interface assets.
\******************************************************************************/
void I_cleanup(void)
{
        R_window_cleanup(&test_window);
}

/******************************************************************************\
 Renders all of the currently open windows and widgets.
\******************************************************************************/
void I_render_windows(void)
{
        R_window_render(&test_window);
}

