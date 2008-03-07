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

SDLKey i_key;
int i_key_shift, i_mouse_x, i_mouse_y, i_mouse;

static i_widget_t root, *key_focus;
static i_toolbar_t left_toolbar;

/******************************************************************************\
 Loads interface assets and initializes windows.
\******************************************************************************/
void I_init(void)
{
        I_widget_init(&root);
        root.class_name = "Root Window";
        I_toolbar_init(&left_toolbar);
        left_toolbar.widget.origin = C_vec2(i_border.value.n,
                                            r_height_2d - 32.f -
                                            i_border.value.n);
        left_toolbar.widget.size = C_vec2(128.f, 32.f);
        I_widget_add(&root, &left_toolbar.widget);
}

/******************************************************************************\
 Free interface assets.
\******************************************************************************/
void I_cleanup(void)
{
        I_widget_event(&root, I_EV_CLEANUP);
}

/******************************************************************************\
 Render all visible windows.
\******************************************************************************/
void I_render(void)
{
        I_widget_event(&root, I_EV_RENDER);
}

/******************************************************************************\
 Toolbar event function.
\******************************************************************************/
static void toolbar_event(i_toolbar_t *toolbar, i_event_t event)
{
        switch (event) {
        case I_EV_CONFIGURE:
                if (toolbar->widget.configured)
                        R_window_cleanup(&toolbar->window);
                R_window_init(&toolbar->window, i_window_bg.value.s);
                toolbar->window.sprite.origin = toolbar->widget.origin;
                toolbar->window.sprite.size = toolbar->widget.size;
                break;
        case I_EV_CLEANUP:
                if (!toolbar->widget.configured)
                        break;
                R_window_cleanup(&toolbar->window);
                break;
        case I_EV_RENDER:
                if (!toolbar->widget.configured)
                        C_error("Toolbar used without being configured");
                R_window_render(&toolbar->window);
                break;
        default:
                break;
        }
}

/******************************************************************************\
 Initializes a toolbar.
\******************************************************************************/
void I_toolbar_init(i_toolbar_t *toolbar)
{
        if (!toolbar)
                return;
        I_widget_init(&toolbar->widget);
        toolbar->widget.class_name = "Toolbar";
        toolbar->widget.event_func = (i_event_f)toolbar_event;
}

/******************************************************************************\
 Attaches a child widget to another widget. This generates a I_EV_CONFIGURE
 event so the child widget must be fully initialized before calling this.
\******************************************************************************/
void I_widget_add(i_widget_t *parent, i_widget_t *child)
{
        i_widget_t *sibling;

        if (!child)
                return;
        if (!parent->class_name)
                C_error("Parent widget uninitialized");
        if (!child->class_name)
                C_error("Child widget uninitialized");
        if (!parent)
                C_error("Tried to add %s to a null pointer", child->class_name);
        if (child->parent)
                C_error("Cannot add %s to %s, already has a parent",
                        child->class_name, parent->class_name);
        sibling = parent->child;
        if (sibling) {
                while (sibling->next)
                        sibling = sibling->next;
                sibling->next = child;
        } else
                parent->child = child;
        child->next = NULL;
        child->parent = parent;
        I_widget_event(child, I_EV_CONFIGURE);
}

/******************************************************************************\
 Dispatches basic widget events and does some checks to see if the event
 applies to this widget. Cleans up resources on I_EV_CLEANUP and propagates
 the event to all child widgets.

 Key focus follows the mouse. Only one widget can have keyboard focus at any
 time and that is the last widget that the mouse cursor passed over that
 could take keyboard input.

 Events filter down through containers to child widgets. The ordering will call
 all of a widget's parent containers before calling the widget and will call
 all of the children of a widget before calling a sibling.
\******************************************************************************/
void I_widget_event(i_widget_t *widget, i_event_t event)
{
        i_widget_t *child;

        child = widget->child;

        /* Before handling and propagating event to children */
        switch (event) {
        case I_EV_KEY_DOWN:
        case I_EV_KEY_UP:
                if (key_focus != widget)
                        return;
                break;
        case I_EV_MOUSE_IN:
                widget->hover = TRUE;
                if (widget->focusable)
                        key_focus = widget;
                break;
        case I_EV_MOUSE_OUT:
                if (!widget->hover)
                        return;
                widget->hover = FALSE;
                break;
        case I_EV_MOUSE_MOVE:
        case I_EV_MOUSE_DOWN:
        case I_EV_MOUSE_UP:
                if (i_mouse_x < widget->origin.x ||
                    i_mouse_y < widget->origin.y ||
                    i_mouse_x >= widget->origin.x + widget->size.x ||
                    i_mouse_y >= widget->origin.y + widget->size.y) {
                        if (widget->hover)
                                I_widget_event(widget, I_EV_MOUSE_OUT);
                        return;
                }
                if (!widget->hover)
                        widget->event_func(widget, I_EV_MOUSE_IN);
                break;
        default:
                break;
        }

        /* Call widget-specific event handler and propagate event down */
        if (widget->event_func)
                widget->event_func(widget, event);
        while (child) {
                I_widget_event(child, event);
                child = child->next;
        }

        /* After handling and propagation to children */
        switch (event) {
        case I_EV_CONFIGURE:
                widget->configured++;
                break;
        case I_EV_CLEANUP:
                if (widget->heap)
                        C_free(widget);
                else
                        C_zero(widget);
                break;
        default:
                break;
        }
}

/******************************************************************************\
 Handle an SDL event. Does not allow multiple keys or mouse buttons to be
 pressed at the same time.
\******************************************************************************/
void I_dispatch(const SDL_Event *ev)
{
        i_event_t event;

        /* Before dispatch */
        switch (ev->type) {
        case SDL_KEYDOWN:
                if (ev->key.keysym.sym == SDLK_RSHIFT ||
                    ev->key.keysym.sym == SDLK_LSHIFT || i_key)
                        return;
                event = I_EV_KEY_DOWN;
                i_key = ev->key.keysym.sym;
                i_key_shift = ev->key.keysym.mod & KMOD_SHIFT;
                break;
        case SDL_KEYUP:
                if (ev->key.keysym.sym == SDLK_RSHIFT ||
                    ev->key.keysym.sym == SDLK_LSHIFT ||
                    i_key != ev->key.keysym.sym)
                        return;
                event = I_EV_KEY_DOWN;
                i_key_shift = ev->key.keysym.mod & KMOD_SHIFT;
                break;
        case SDL_MOUSEMOTION:
                event = I_EV_MOUSE_MOVE;
                i_mouse_x = ev->motion.x;
                i_mouse_y = ev->motion.y;
                key_focus = NULL;
                break;
        case SDL_MOUSEBUTTONDOWN:
                if (i_mouse)
                        return;
                event = I_EV_MOUSE_DOWN;
                i_mouse = ev->button.button;
                i_mouse_x = ev->button.x;
                i_mouse_y = ev->button.y;
                key_focus = NULL;
                break;
        case SDL_MOUSEBUTTONUP:
                if (!i_mouse || i_mouse != ev->button.button)
                        return;
                event = I_EV_MOUSE_UP;
                i_mouse_x = ev->button.x;
                i_mouse_y = ev->button.y;
                key_focus = NULL;
                break;
        default:
                return;
        }

        I_widget_event(&root, event);

        /* After dispatch */
        switch (ev->type) {
        case SDL_MOUSEBUTTONUP:
                i_mouse = 0;
                break;
        case SDL_KEYUP:
                i_key = 0;
                break;
        default:
                break;
        }
}

