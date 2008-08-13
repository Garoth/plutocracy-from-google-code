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
static i_entry_t ip_entry;
static i_scrollback_t server_list;
static i_button_t host_button, join_button, leave_button, quit_button;
static i_box_t outer_box, left_box, right_box;

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
        G_leave_game();
        I_widget_event(I_widget_top_level(&button->widget), I_EV_HIDE);
}

/******************************************************************************\
 Host a new game via interface.
\******************************************************************************/
static void host_button_clicked(i_button_t *button)
{
        C_debug("Host button clicked");
        G_host_game();
        I_widget_event(I_widget_top_level(&button->widget), I_EV_HIDE);
}

/******************************************************************************\
 Join a game via interface.
\******************************************************************************/
static void join_button_clicked(i_button_t *button)
{
        C_debug("Join button clicked");
        G_join_game(ip_entry.buffer);
        I_widget_event(I_widget_top_level(&button->widget), I_EV_HIDE);
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
 Callback for when the ip value changes.
\******************************************************************************/
static void ip_entry_changed(void)
{
        C_strncpy_buf(i_ip.value.s, ip_entry.buffer);
        if (ip_entry.buffer[0] && join_button.widget.state == I_WS_DISABLED)
                join_button.widget.state = I_WS_READY;
        else if (!ip_entry.buffer[0])
                join_button.widget.state = I_WS_DISABLED;
}

/******************************************************************************\
 Initializes game window widgets on the given window.
\******************************************************************************/
void I_init_game(i_window_t *window)
{
        I_window_init(window);
        window->widget.size = C_vec2(400.f, 200.f);
        window->fit = I_FIT_TOP;

        /* Label */
        I_label_init(&label, C_str("i-menu", "Game Menu"));
        label.font = R_FONT_TITLE;
        I_widget_add(&window->widget, &label.widget);

        /* Initialize outer box */
        I_box_init(&outer_box, I_PACK_H,
                   window->widget.size.y - i_border.value.n * 2);
        I_widget_add(&window->widget, &outer_box.widget);

        /* Left-side vertical box */
        I_box_init(&left_box, I_PACK_V, window->widget.size.x / 4.f);
        left_box.widget.margin_rear = 1.f;
        I_widget_add(&outer_box.widget, &left_box.widget);

        /* Right-side vertical box */
        I_box_init(&right_box, I_PACK_V, 0.f);
        right_box.widget.expand = TRUE;
        I_widget_add(&outer_box.widget, &right_box.widget);

        /* Join button */
        I_button_init(&join_button, NULL, C_str("i-join", "Join"),
                      I_BT_DECORATED);
        join_button.on_click = (i_callback_f)join_button_clicked;
        I_widget_add(&left_box.widget, &join_button.widget);

        /* Leave button */
        I_button_init(&leave_button, NULL, C_str("i-leave", "Leave"),
                      I_BT_DECORATED);
        leave_button.widget.event_func = (i_event_f)leave_button_event;
        leave_button.on_click = (i_callback_f)leave_button_clicked;
        leave_button.widget.state = I_WS_DISABLED;
        I_widget_add(&left_box.widget, &leave_button.widget);

        /* Host button */
        I_button_init(&host_button, NULL, C_str("i-host", "Host"),
                      I_BT_DECORATED);
        host_button.on_click = (i_callback_f)host_button_clicked;
        host_button.widget.margin_rear = 1.f;
        I_widget_add(&left_box.widget, &host_button.widget);

        /* Quit button */
        I_button_init(&quit_button, NULL, C_str("i-quit", "Quit"),
                      I_BT_DECORATED);
        quit_button.on_click = (i_callback_f)quit_button_clicked;
        I_widget_add(&left_box.widget, &quit_button.widget);

        /* IP entry */
        C_var_unlatch(&i_ip);
        I_entry_init(&ip_entry, i_ip.value.s);
        ip_entry.widget.margin_rear = 0.5f;
        ip_entry.on_change = (i_callback_f)ip_entry_changed;
        I_widget_add(&right_box.widget, &ip_entry.widget);

        /* Server list scrollback */
        I_scrollback_init(&server_list);
        I_widget_add(&right_box.widget, &server_list.widget);
}

