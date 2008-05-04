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

/******************************************************************************\
 Window widget event function.
\******************************************************************************/
static int window_event(i_window_t *window, i_event_t event)
{
        switch (event) {
        case I_EV_CONFIGURE:
                R_window_cleanup(&window->window);
                I_widget_pack(&window->widget, window->pack_children,
                              window->fit);
                if (window->decorated) {
                        R_window_init(&window->window, i_window.value.s);
                        window->window.sprite.origin = window->widget.origin;
                        window->window.sprite.size = window->widget.size;
                        R_sprite_cleanup(&window->hanger);
                        R_sprite_init(&window->hanger, i_hanger.value.s);
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
                break;
        case I_EV_CLEANUP:
                R_window_cleanup(&window->window);
                R_sprite_cleanup(&window->hanger);
                break;
        case I_EV_RENDER:
                if (window->decorated) {
                        if (window->hanger.origin.x) {
                                window->hanger.modulate.a = window->widget.fade;
                                R_sprite_render(&window->hanger);
                        }
                        window->window.sprite.modulate.a = window->widget.fade;
                        R_window_render(&window->window);
                }
                break;
        default:
                break;
        }
        return TRUE;
}

/******************************************************************************\
 Configure a window widget's hanger sprite.
\******************************************************************************/
void I_window_hanger(i_window_t *window, i_widget_t *widget, int shown)
{
        if (!shown) {
                window->hanger.origin.x = 0.f;
                return;
        }
        window->hanger.origin.x = widget->origin.x + widget->size.x / 2 -
                                  window->hanger.size.x / 2;
}

/******************************************************************************\
 Initializes an window widget.
\******************************************************************************/
void I_window_init(i_window_t *window)
{
        if (!window)
                return;
        C_zero(window);
        I_widget_set_name(&window->widget, "Window");
        window->widget.event_func = (i_event_f)window_event;
        window->widget.state = I_WS_READY;
        window->widget.clickable = TRUE;
        window->widget.padding = 1.f;
        window->pack_children = I_PACK_V;
        window->decorated = TRUE;
        I_widget_inited(&window->widget);
}

