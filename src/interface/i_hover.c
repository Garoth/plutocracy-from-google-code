/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Window to provide information about the currently hovered-over tile */

#include "i_common.h"

static i_label_t title_label;
static i_window_t hover_window;

/******************************************************************************\
 Event function for hover window.
\******************************************************************************/
static bool hover_window_event(i_window_t *window, i_event_t event)
{
        /* Position and size the window */
        if (event == I_EV_CONFIGURE) {
                window->widget.origin = C_vec2((float)i_border.value.n,
                                               (float)i_border.value.n);
                window->widget.size = C_vec2(180.f, 0.f);
        }

        return I_window_event(window, event);
}

/******************************************************************************\
 Show the hover window and set its title.
\******************************************************************************/
void I_hover_show(const char *title)
{
        /* Get rid of the old window */
        I_widget_event(&hover_window.widget, I_EV_CLEANUP);

        /* Add the window to the root widget */
        I_window_init(&hover_window);
        hover_window.widget.auto_configure = TRUE;
        hover_window.widget.event_func = (i_event_f)hover_window_event;
        hover_window.fit = I_FIT_BOTTOM;
        hover_window.popup = TRUE;
        I_widget_add(&i_root, &hover_window.widget);

        /* Add the title */
        I_label_init(&title_label, title);
        title_label.font = R_FONT_TITLE;
        I_widget_add(&hover_window.widget, &title_label.widget);
}

/******************************************************************************\
 Close the hover window. The window will need to be initialized again the next
 time it is shown.
\******************************************************************************/
void I_hover_close(void)
{
        if (!hover_window.widget.configured)
                return;
        I_widget_event(&hover_window.widget, I_EV_HIDE);
}

/******************************************************************************\
 Add an information item to the hover window.
\******************************************************************************/
void I_hover_add(const char *label, const char *value)
{
        i_info_t *info;

        info = I_info_alloc(label, value);
        I_widget_add(&hover_window.widget, &info->widget);
}

/******************************************************************************\
 Add a colored information item to the hover window.
\******************************************************************************/
void I_hover_add_color(const char *label, const char *value, i_color_t color)
{
        i_info_t *info;

        info = I_info_alloc(label, value);
        info->right.color = color;
        I_widget_add(&hover_window.widget, &info->widget);
}

