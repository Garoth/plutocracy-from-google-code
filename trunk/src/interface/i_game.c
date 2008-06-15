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
static i_button_t host_button, join_button, leave_button, quit_button;

/******************************************************************************\
 Leave button event handler.
\******************************************************************************/
static int leave_button_event(i_button_t *button, i_event_t event)
{
        if (event == I_EV_RENDER) {
                if (button->widget.state == I_WS_DISABLED && !i_limbo)
                        button->widget.state = I_WS_READY;
                else if (button->widget.state != I_WS_DISABLED && i_limbo)
                        button->widget.state = I_WS_DISABLED;
        }
        return I_button_event(button, event);
}

/******************************************************************************\
 Leave the current game.
\******************************************************************************/
static void leave_button_clicked(i_button_t *button)
{
        C_debug("Leave button clicked");
        I_widget_show(button->widget.parent, FALSE);
        I_enter_limbo();
}

/******************************************************************************\
 Host a new game via interface.
\******************************************************************************/
static void host_button_clicked(i_button_t *button)
{
        C_debug("Host button clicked");
        G_host_game();
        I_widget_show(button->widget.parent, FALSE);
        I_leave_limbo();
}

/******************************************************************************\
 Quit the game via interface.
\******************************************************************************/
static void quit_button_clicked(i_button_t *button)
{
        C_debug("Quit button clicked");
        c_exit = TRUE;
}

/******************************************************************************\
 Initializes game window widgets on the given window.
\******************************************************************************/
void I_game_init(i_window_t *window)
{
        I_window_init(window);
        window->fit = I_FIT_TOP;

        /* Label */
        I_label_init(&label, C_str("i-menu", "Game Menu:"));
        label.font = R_FONT_TITLE;
        I_widget_add(&window->widget, &label.widget);

        /* Join button */
        I_button_init(&join_button, NULL, C_str("i-join", "Join"),
                      I_BT_DECORATED);
        join_button.widget.state = I_WS_DISABLED;
        I_widget_add(&window->widget, &join_button.widget);

        /* Leave button */
        I_button_init(&leave_button, NULL, C_str("i-leave", "Leave"),
                      I_BT_DECORATED);
        leave_button.widget.event_func = (i_event_f)leave_button_event;
        leave_button.on_click = (i_callback_f)leave_button_clicked;
        leave_button.widget.state = I_WS_DISABLED;
        I_widget_add(&window->widget, &leave_button.widget);

        /* Host button */
        I_button_init(&host_button, NULL, C_str("i-host", "Host"),
                      I_BT_DECORATED);
        host_button.on_click = (i_callback_f)host_button_clicked;
        host_button.widget.margin_rear = 1.f;
        I_widget_add(&window->widget, &host_button.widget);

        /* Quit button */
        I_button_init(&quit_button, NULL, C_str("i-quit", "Quit"),
                      I_BT_DECORATED);
        quit_button.on_click = (i_callback_f)quit_button_clicked;
        I_widget_add(&window->widget, &quit_button.widget);
}

