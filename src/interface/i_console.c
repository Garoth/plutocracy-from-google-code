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

static i_label_t label;
static i_window_t window;
static i_entry_t entry;

/******************************************************************************\
 Initializes and returns a pointer to the console window.
\******************************************************************************/
i_widget_t *I_console_init(void)
{
        /* Window container */
        I_window_init(&window);
        window.widget.size = C_vec2(320.f, 240.f);
        window.widget.origin = C_vec2(i_border.value.n, r_height_2d -
                                      window.widget.size.y - 64.f -
                                      i_border.value.n * 2);
        window.collapse = I_COLLAPSE_INVERT;

        /* Label */
        I_label_init(&label, "Console:");
        I_widget_add(&window.widget, &label.widget);

        /* Entry */
        I_entry_init(&entry, "test");
        I_widget_add(&window.widget, &entry.widget);

        return &window.widget;
}

