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
static i_button_t nations[4];

/******************************************************************************\
 A nation button was clicked.
\******************************************************************************/
static void nation_clicked(i_button_t *button)
{
        I_widget_show(button->widget.parent, FALSE);
}

/******************************************************************************\
 Initializes nations window widgets on the given window.
\******************************************************************************/
void I_nations_init(i_window_t *window)
{
        I_window_init(window);
        window->natural_size = C_vec2(200.f, 0.f);
        window->fit = I_FIT_TOP;

        /* Title label */
        I_label_init(&title, C_str("i-nations", "Affiliation:"));
        title.font = R_FONT_TITLE;
        I_widget_add(&window->widget, &title.widget);

        /* Ruby nation */
        I_button_init(nations, "gui/flags/red.png",
                      C_str("c-team-red", "Ruby"), I_BT_DECORATED);
        nations[0].on_click = (i_callback_f)nation_clicked;
        I_widget_add(&window->widget, &nations[0].widget);

        /* Emerald nation */
        I_button_init(nations + 1, "gui/flags/green.png",
                      C_str("c-team-green", "Emerald"), I_BT_DECORATED);
        nations[1].on_click = (i_callback_f)nation_clicked;
        I_widget_add(&window->widget, &nations[1].widget);

        /* Sapphire nation */
        I_button_init(nations + 2, "gui/flags/blue.png",
                      C_str("c-team-blue", "Sapphire"), I_BT_DECORATED);
        nations[2].on_click = (i_callback_f)nation_clicked;
        I_widget_add(&window->widget, &nations[2].widget);

        /* Pirates */
        I_button_init(nations + 3, "gui/flags/pirate.png",
                      C_str("c-team-pirate", "Pirate"), I_BT_DECORATED);
        nations[3].widget.margin_front = 1.f;
        nations[3].on_click = (i_callback_f)nation_clicked;
        I_widget_add(&window->widget, &nations[3].widget);
}

