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
static i_button_t quit_button;

/******************************************************************************\
 Button callbacks.
\******************************************************************************/
static void quit_button_clicked(i_button_t *button)
{
        C_debug("Exit button clicked");
        c_exit = TRUE;
}

/******************************************************************************\
 Initializes game window widgets on the given window.
\******************************************************************************/
void I_game_init(i_window_t *window)
{
        /* Label */
        I_label_init(&label, _("Game Menu:"));
        I_widget_add(&window->widget, &label.widget);

        /* Quit button */
        I_button_init(&quit_button, NULL, _("Quit"), TRUE);
        quit_button.on_click = (i_callback_f)quit_button_clicked;
        I_widget_add(&window->widget, &quit_button.widget);
}

