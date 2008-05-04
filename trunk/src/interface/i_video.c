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
static i_button_t apply_button;
static i_select_t windowed;
static const char *list_bool[] = {"No", "Yes", NULL};

/******************************************************************************\
 Apply modified settings.
\******************************************************************************/
static void apply_button_clicked(i_button_t *button)
{
}

/******************************************************************************\
 Initializes game window widgets on the given window.
\******************************************************************************/
void I_video_init(i_window_t *window)
{
        I_window_init(window);
        window->fit = I_FIT_TOP;

        /* Label */
        I_label_init(&label, C_str("i-video", "Video Settings:"));
        label.font = R_FONT_TITLE;
        I_widget_add(&window->widget, &label.widget);

        /* Select windowed */
        I_select_init(&windowed, C_str("i-video-windowed", "Windowed:"),
                      list_bool, 0);
        I_widget_add(&window->widget, &windowed.widget);

        /* Apply button */
        I_button_init(&apply_button, NULL,
                      C_str("i-video-apply", "Apply"), TRUE);
        apply_button.on_click = (i_callback_f)apply_button_clicked;
        apply_button.widget.margin_front = 1.f;
        I_widget_add(&window->widget, &apply_button.widget);
}

