/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Implements generic windows and toolbars */

#include "i_common.h"

/* Time in milliseconds that the popup widget will remain shown */
#define POPUP_TIME 3000

/* Maximum number of popup messages in the queue */
#define POPUP_MESSAGES_MAX 8

/* Type for queued popup messages */
typedef struct popup_message {
        i_popup_icon_t icon;
        c_vec3_t goto_pos;
        int has_goto_pos;
        char message[128];
} popup_message_t;

static popup_message_t popup_messages[POPUP_MESSAGES_MAX];
static i_label_t popup_label;
static i_widget_t popup_widget;
static r_window_t popup_window;
static r_texture_t *decor_window, *hanger, *decor_popup;
static int popup_mouse_time;

/******************************************************************************\
 Initialize themeable window assets.
\******************************************************************************/
void I_theme_windows(void)
{
        I_theme_texture(&decor_window, "window");
        I_theme_texture(&decor_popup, "popup");
        I_theme_texture(&hanger, "hanger");
}

/******************************************************************************\
 Window widget event function.
\******************************************************************************/
static int window_event(i_window_t *window, i_event_t event)
{
        switch (event) {
        case I_EV_CONFIGURE:

                /* Windows are optionally excepted from the normal packing
                   order. Since they are never in another container, they can
                   set their own size. */
                if (window->natural_size.x || window->natural_size.y)
                        window->widget.size = window->natural_size;

                R_window_cleanup(&window->window);
                I_widget_pack(&window->widget, window->pack_children,
                              window->fit);
                if (window->decorated) {
                        R_window_init(&window->window, decor_window);
                        window->window.sprite.origin = window->widget.origin;
                        window->window.sprite.size = window->widget.size;
                        R_sprite_cleanup(&window->hanger);
                        R_sprite_init(&window->hanger, hanger);
                        window->hanger.origin.y = window->widget.origin.y +
                                                  window->widget.size.y;
                        window->hanger.size.y = (float)i_border.value.n;
                }
                return FALSE;
        case I_EV_MOUSE_IN:
                i_key_focus = NULL;
                I_widget_propagate(&window->widget, I_EV_GRAB_FOCUS);
                break;
        case I_EV_MOVED:
                window->window.sprite.origin = window->widget.origin;
                window->hanger.origin.y = window->widget.origin.y +
                                          window->widget.size.y;
                break;
        case I_EV_CLEANUP:
                R_window_cleanup(&window->window);
                R_sprite_cleanup(&window->hanger);
                break;
        case I_EV_RENDER:
                if (!window->decorated)
                        break;
                window->window.sprite.modulate.a = window->widget.fade;
                R_window_render(&window->window);
                if (!window->hanger_align)
                        break;
                window->hanger.origin.x = window->hanger_align->origin.x +
                                          window->hanger_align->size.x / 2 -
                                          window->hanger.size.x / 2;
                window->hanger.modulate.a = window->widget.fade;
                R_sprite_render(&window->hanger);
                break;
        default:
                break;
        }
        return TRUE;
}

/******************************************************************************\
 Initializes an window widget.
\******************************************************************************/
void I_window_init(i_window_t *window)
{
        if (!window)
                return;
        C_zero(window);
        I_widget_init(&window->widget, "Window");
        window->widget.event_func = (i_event_f)window_event;
        window->widget.state = I_WS_READY;
        window->widget.clickable = TRUE;
        window->widget.padding = 1.f;
        window->pack_children = I_PACK_V;
        window->decorated = TRUE;
}

/******************************************************************************\
 Positions the windows this toolbar controls.
\******************************************************************************/
static void toolbar_position(i_toolbar_t *toolbar)
{
        i_widget_t *widget;
        c_vec2_t origin;
        int i;

        for (i = 0; i < toolbar->children; i++) {
                widget = &toolbar->windows[i].widget;
                origin = toolbar->widget.origin;
                origin.y -= i_border.value.n + widget->size.y;
                if (toolbar->right)
                        origin.x += toolbar->widget.size.x - widget->size.x;
                I_widget_move(widget, origin);
        }
}

/******************************************************************************\
 Toolbar event function.
\******************************************************************************/
int I_toolbar_event(i_toolbar_t *toolbar, i_event_t event)
{
        int i;

        switch (event) {
        case I_EV_CONFIGURE:

                /* The toolbar just has the child window widget so we basically
                   slave all the positioning and sizing to it */
                toolbar->window.widget.origin = toolbar->widget.origin;
                toolbar->window.widget.size = toolbar->widget.size;
                I_widget_event(&toolbar->window.widget, event);
                toolbar->widget.origin = toolbar->window.widget.origin;
                toolbar->widget.size = toolbar->window.widget.size;
                toolbar_position(toolbar);
                return FALSE;
        case I_EV_HIDE:
                for (i = 0; i < toolbar->children; i++)
                        I_widget_show(&toolbar->windows[i].widget, FALSE);
                break;
        default:
                break;
        }
        return TRUE;
}

/******************************************************************************\
 Initialize a toolbar widget.
\******************************************************************************/
void I_toolbar_init(i_toolbar_t *toolbar, int right)
{
        C_zero(toolbar);
        I_widget_init(&toolbar->widget, "Toolbar");
        toolbar->widget.event_func = (i_event_f)I_toolbar_event;
        toolbar->widget.state = I_WS_READY;
        toolbar->widget.clickable = TRUE;
        toolbar->right = right;
        I_window_init(&toolbar->window);
        toolbar->window.pack_children = I_PACK_H;
        toolbar->window.fit = right ? I_FIT_TOP : I_FIT_BOTTOM;
        I_widget_add(&toolbar->widget, &toolbar->window.widget);
}

/******************************************************************************\
 Toolbar button handlers.
\******************************************************************************/
static void toolbar_button_click(i_button_t *button)
{
        i_toolbar_t *toolbar;
        i_window_t *window;

        toolbar = (i_toolbar_t *)button->data;
        window = toolbar->windows + (int)(button - toolbar->buttons);
        if (toolbar->open_window && toolbar->open_window->widget.shown) {
                I_widget_show(&toolbar->open_window->widget, FALSE);
                if (toolbar->open_window == window) {
                        toolbar->open_window = NULL;
                        return;
                }
        }
        I_widget_show(&window->widget, TRUE);
        toolbar->open_window = window;
}

/******************************************************************************\
 Adds a button to a toolbar widget. The window opened by the icon button is
 initialized with [init_func]. This function should set the window's size as
 the root window does not pack child windows. The window's origin is controlled
 by the toolbar.
\******************************************************************************/
void I_toolbar_add_button(i_toolbar_t *toolbar, const char *icon,
                          i_callback_f init_func)
{
        i_button_t *button;
        i_window_t *window;

        C_assert(toolbar->children < I_TOOLBAR_BUTTONS);

        /* Pad the previous button */
        if (toolbar->children) {
                button = toolbar->buttons + toolbar->children - 1;
                button->widget.margin_rear = 0.5f;
        }

        /* Add a new button */
        button = toolbar->buttons + toolbar->children;
        I_button_init(button, icon, NULL, I_BT_SQUARE);
        button->on_click = (i_callback_f)toolbar_button_click;
        button->data = toolbar;
        I_widget_add(&toolbar->window.widget, &button->widget);

        /* Initialize the window */
        window = toolbar->windows + toolbar->children;
        init_func(window);
        window->widget.shown = FALSE;
        window->hanger_align = &button->widget;
        I_widget_add(&i_root, &window->widget);

        toolbar->children++;
}

/******************************************************************************\
 Configure the popup widget to display a message from the queue.
\******************************************************************************/
static void popup_configure()
{
        if (index < 0 || !popup_messages[0].message[0]) {
                I_widget_show(&popup_widget, FALSE);
                return;
        }
        I_label_configure(&popup_label, popup_messages[0].message);
        I_widget_event(&popup_widget, I_EV_CONFIGURE);
        I_widget_show(&popup_widget, TRUE);
        popup_mouse_time = c_time_msec;
}

/******************************************************************************\
 Popup widget event function.
\******************************************************************************/
static int popup_event(i_widget_t *widget, i_event_t event)
{
        c_vec2_t origin;

        switch (event) {
        case I_EV_CONFIGURE:
                widget->size = C_vec2(0.f, 32.f);
                I_widget_pack(widget, I_PACK_H, I_FIT_BOTTOM);
                origin = C_vec2(r_width_2d / 2.f - widget->size.x / 2.f,
                                r_height_2d - widget->size.y -
                                i_border.value.n);
                R_window_cleanup(&popup_window);
                R_window_init(&popup_window, decor_popup);
                I_widget_move(widget, origin);
        case I_EV_MOVED:
                popup_window.sprite.size = widget->size;
                popup_window.sprite.origin = widget->origin;
                return FALSE;
        case I_EV_MOUSE_MOVE:
                popup_mouse_time = c_time_msec;
                break;
        case I_EV_CLEANUP:
                R_window_cleanup(&popup_window);
                break;
        case I_EV_RENDER:
                if (c_time_msec - popup_mouse_time > POPUP_TIME)
                        I_widget_show(&popup_widget, FALSE);
                popup_window.sprite.modulate.a = widget->fade;
                R_window_render(&popup_window);
                break;
        default:
                break;
        }
        return TRUE;
}

/******************************************************************************\
 Initialize popup widget and add it to the root widget.
\******************************************************************************/
void I_init_popup(void)
{
        I_widget_init(&popup_widget, "Popup");
        popup_widget.event_func = (i_event_f)popup_event;
        popup_widget.state = I_WS_READY;
        popup_widget.shown = FALSE;
        popup_widget.clickable = TRUE;
        popup_widget.padding = 1.f;

        /* Label */
        I_label_init(&popup_label, NULL);
        I_widget_add(&popup_widget, &popup_label.widget);

        I_widget_add(&i_root, &popup_widget);
}

/******************************************************************************\
 Updates the popup window.
\******************************************************************************/
void I_update_popup(void)
{
        if (popup_widget.shown || popup_widget.fade > 0.f ||
            !popup_messages[0].message[0])
                return;
        memcpy(popup_messages, popup_messages + 1,
               sizeof (popup_messages) - sizeof (*popup_messages));
        popup_messages[POPUP_MESSAGES_MAX - 1].message[0] = NUL;
        popup_configure();
}

/******************************************************************************\
 Queues a new message to popup
\******************************************************************************/
void I_popup(i_popup_icon_t icon, const char *message, c_vec3_t *goto_pos)
{
        int i;

        /* Find an open slot, if there isn't one, pump the queue */
        for (i = 0; popup_messages[i].message[0]; i++)
                if (i >= POPUP_MESSAGES_MAX) {
                        memcpy(popup_messages, popup_messages + 1,
                               sizeof (popup_messages) -
                               sizeof (*popup_messages));
                        i--;
                        break;
                }

        /* Setup the queue entry */
        popup_messages[i].icon = icon;
        C_strncpy_buf(popup_messages[i].message, message);
        if (goto_pos) {
                popup_messages[i].has_goto_pos = TRUE;
                popup_messages[i].goto_pos = *goto_pos;
        } else
                popup_messages[i].has_goto_pos = FALSE;

        /* If the popup window is hidden, configure and open it */
        if (!popup_widget.shown || i >= POPUP_MESSAGES_MAX - 1)
                popup_configure();
}

