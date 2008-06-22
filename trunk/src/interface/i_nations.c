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

static i_label_t title;

/******************************************************************************\
 Initializes nations window widgets on the given window.
\******************************************************************************/
void I_nations_init(i_window_t *window)
{
        I_window_init(window);
        window->widget.size = C_vec2(200.f, 0.f);
        window->fit = I_FIT_TOP;

        /* Title label */
        I_label_init(&title, C_str("i-nations", "Nations:"));
        title.font = R_FONT_TITLE;
        I_widget_add(&window->widget, &title.widget);
}

