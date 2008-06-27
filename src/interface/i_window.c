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

static r_texture_t *decoration, *hanger;

/******************************************************************************\
 Initialize themeable window assets.
\******************************************************************************/
void I_theme_windows(void)
{
        I_theme_texture(&decoration, "window");
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
                        R_window_init(&window->window, decoration);
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

