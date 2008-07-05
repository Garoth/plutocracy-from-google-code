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

/* Nation button limit */
#define NATIONS_LEN 8

static i_window_t *nations_window;
static i_label_t title;
static i_button_t nation_buttons[NATIONS_LEN];
static int nations_len, selected;

/******************************************************************************\
 A nation button was clicked.
\******************************************************************************/
static void nation_clicked(i_button_t *button)
{
        I_widget_show(button->widget.parent, FALSE);
        G_change_nation((int)(button - nation_buttons));
}

/******************************************************************************\
 Reset nation buttons.
\******************************************************************************/
void I_reset_nations(void)
{
        int i;

        for (i = 0; i < NATIONS_LEN; i++)
                I_widget_cleanup(&nation_buttons[i].widget);
        nations_len = 0;
}

/******************************************************************************\
 Add a nation button.
\******************************************************************************/
void I_add_nation(const char *short_name, const char *long_name, bool margin)
{
        C_assert(nations_len < NATIONS_LEN);
        I_button_init(nation_buttons + nations_len,
                      C_va("gui/flags/%s.png", short_name),
                      C_str(C_va("c-team-%s", short_name), long_name),
                      I_BT_DECORATED);
        nation_buttons[nations_len].on_click = (i_callback_f)nation_clicked;
        if (margin)
                nation_buttons[nations_len].widget.margin_front = 1.f;
        I_widget_add(&nations_window->widget,
                     &nation_buttons[nations_len++].widget);
}

/******************************************************************************\
 Configures nations window.
\******************************************************************************/
void I_configure_nations(void)
{
        /* Because the window is top-fitting, we need to move it back to the
           original position before configuration */
        nations_window->widget.origin.y += nations_window->widget.size.y;

        I_widget_event(&nations_window->widget, I_EV_CONFIGURE);
}

/******************************************************************************\
 Disables one of the nation buttons.
\******************************************************************************/
void I_select_nation(int nation)
{
        C_assert(nation >= 0 && nation < NATIONS_LEN);
        if (nation_buttons[selected].widget.state == I_WS_DISABLED)
                nation_buttons[selected].widget.state = I_WS_READY;
        nation_buttons[selected = nation].widget.state = I_WS_DISABLED;
}

/******************************************************************************\
 Initializes nations window widgets on the given window. Nation buttons can
 be added after this is called.
\******************************************************************************/
void I_nations_init(i_window_t *window)
{
        I_window_init(window);
        window->natural_size = C_vec2(200.f, 0.f);
        window->fit = I_FIT_TOP;
        nations_window = window;

        /* Title label */
        I_label_init(&title, C_str("i-nations", "Affiliation:"));
        title.font = R_FONT_TITLE;
        I_widget_add(&window->widget, &title.widget);
}

