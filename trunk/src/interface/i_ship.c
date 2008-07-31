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
static i_info_t class_info, owner_info;

/******************************************************************************\
 Deselect a ship.
\******************************************************************************/
void I_deselect_ship(void)
{
        I_toolbar_enable(&i_right_toolbar, i_ship_button, FALSE);
}

/******************************************************************************\
 Selected a ship.
\******************************************************************************/
void I_select_ship(i_color_t color, const char *name,
                   const char *owner, const char *class_name)
{
        I_toolbar_enable(&i_right_toolbar, i_ship_button, TRUE);

        /* Title */
        I_label_configure(&title, name);

        /* Infos */
        owner_info.right.color = color;
        I_info_configure(&owner_info, owner);
        I_info_configure(&class_info, class_name);
}

/******************************************************************************\
 Initialize the ship window.
\******************************************************************************/
void I_init_ship(i_window_t *window)
{
        I_window_init(window);
        window->widget.size = C_vec2(200.f, 0.f);
        window->fit = I_FIT_TOP;

        /* Label */
        I_label_init(&title, "Unnamed");
        title.width = -1.f;
        title.font = R_FONT_TITLE;
        I_widget_add(&window->widget, &title.widget);

        /* Owner */
        I_info_init(&owner_info, C_str("i-ship-owner", "Owner:"), "None");
        I_widget_add(&window->widget, &owner_info.widget);

        /* Ship class */
        I_info_init(&class_info, C_str("i-ship-class", "Ship class:"), "None");
        I_widget_add(&window->widget, &class_info.widget);
}

