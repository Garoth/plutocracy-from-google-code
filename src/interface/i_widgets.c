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

i_widget_t *I_console_init(void);

i_widget_t *i_key_focus;
int i_key, i_key_shift, i_key_unicode, i_mouse_x, i_mouse_y, i_mouse;

static i_widget_t root, *mouse_focus;
static i_window_t left_toolbar;
static i_button_t console_button;
static int widgets;

/******************************************************************************\
 Initializes a widget name field with a unique name that includes the
 widget's class.
\******************************************************************************/
void I_widget_set_name(i_widget_t *widget, const char *class_name)
{
        C_assert(widget);
        snprintf(widget->name, sizeof (widget->name),
                 "widget #%u (%s)", widgets++, class_name);
}

/******************************************************************************\
 Prints messages helpful for memory debugging. Call within a widget
 initialization function at the very end.
\******************************************************************************/
void I_widget_inited(const i_widget_t *widget)
{
        if (!widget->name[0])
                C_error("Forgot to set name for widget");
        if (!c_mem_check.value.n)
                return;
        C_debug("Initialized %s", widget->name);
}

/******************************************************************************\
 Root window event function.
 TODO: Stuff like rotating the globe and other interactions will happen here
       because this is the catch all for mouse/keyboard events that don't get
       handled.
\******************************************************************************/
static int root_event(i_widget_t *root, i_event_t event)
{
        return TRUE;
}

/******************************************************************************\
 Loads interface assets and initializes windows.
\******************************************************************************/
void I_init(void)
{
        C_status("Initializing interface");

        /* Enable key repeats so held keys generate key-down events */
        SDL_EnableKeyRepeat(300, 30);

        /* Enable Unicode so we get Unicode key strokes AND capitals */
        SDL_EnableUNICODE(TRUE);

        /* Root window */
        C_zero(&root);
        I_widget_set_name(&root, "Root Window");
        root.event_func = (i_event_f)root_event;
        root.origin = C_vec2(0.f, 0.f);
        root.size = C_vec2(r_width_2d, r_height_2d);
        root.configured = 1;
        root.entry = TRUE;

        /* Left toolbar */
        I_window_init(&left_toolbar);
        left_toolbar.pack_children = I_PACK_H;
        left_toolbar.widget.size = C_vec2(128.f, 48.f);
        left_toolbar.widget.origin = C_vec2(i_border.value.n, r_height_2d -
                                            left_toolbar.widget.size.y -
                                            i_border.value.n);
        I_widget_add(&root, &left_toolbar.widget);

        /* Console button */
        I_button_init(&console_button, NULL, "Test Button", TRUE);
        I_widget_add(&left_toolbar.widget, &console_button.widget);

        /* Console window */
        I_widget_add(&root, I_console_init());

        /* Configure all widgets */
        I_widget_event(&root, I_EV_CONFIGURE);
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
 Window widget event function.
\******************************************************************************/
static int window_event(i_window_t *window, i_event_t event)
{
        switch (event) {
        case I_EV_CONFIGURE:
                R_window_cleanup(&window->window);
                I_widget_pack(&window->widget, window->pack_children,
                              window->collapse);
                if (window->decorated) {
                        R_window_init(&window->window, i_window.value.s);
                        window->window.sprite.origin = window->widget.origin;
                        window->window.sprite.size = window->widget.size;
                }
                return FALSE;
        case I_EV_MOVED:
                window->window.sprite.origin = window->widget.origin;
                break;
        case I_EV_CLEANUP:
                R_window_cleanup(&window->window);
                break;
        case I_EV_RENDER:
                if (window->decorated) {
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
        window->pack_children = I_PACK_V;
        window->decorated = TRUE;
        window->collapse = I_COLLAPSE_NONE;
        I_widget_inited(&window->widget);
}

/******************************************************************************\
 Label widget event function.
\******************************************************************************/
static int label_event(i_label_t *label, i_event_t event)
{
        switch (event) {
        case I_EV_CONFIGURE:
                R_sprite_cleanup(&label->text);
                R_sprite_init_text(&label->text, R_FONT_GUI,
                                   label->widget.size.x, i_shadow.value.f,
                                   FALSE, label->buffer);
                label->widget.size = label->text.size;
                label->text.modulate = C_color_string(i_color.value.s);
        case I_EV_MOVED:
                label->text.origin = C_vec2_clamp(label->widget.origin,
                                                  r_pixel_scale.value.f);
                break;
        case I_EV_CLEANUP:
                R_sprite_cleanup(&label->text);
                break;
        case I_EV_RENDER:
                label->text.modulate.a = label->widget.fade;
                R_sprite_render(&label->text);
                break;
        default:
                break;
        }
        return TRUE;
}

/******************************************************************************\
 Initializes a label widget.
\******************************************************************************/
void I_label_init(i_label_t *label, const char *text)
{
        if (!label)
                return;
        C_zero(label);
        I_widget_set_name(&label->widget, "Label");
        label->widget.event_func = (i_event_f)label_event;
        label->widget.state = I_WS_READY;
        C_strncpy_buf(label->buffer, text);
        I_widget_inited(&label->widget);
}

/******************************************************************************\
 Attaches a child widget to another widget.
\******************************************************************************/
void I_widget_add(i_widget_t *parent, i_widget_t *child)
{
        i_widget_t *sibling;

        if (!child)
                return;
        if (!parent->name[0])
                C_error("Parent widget uninitialized");
        if (!child->name[0])
                C_error("Child widget uninitialized");
        if (!parent)
                C_error("Tried to add %s to a null pointer", child->name);
        if (child->parent)
                C_error("Cannot add %s to %s, already has a parent",
                        child->name, parent->name);
        sibling = parent->child;
        if (sibling) {
                while (sibling->next)
                        sibling = sibling->next;
                sibling->next = child;
        } else
                parent->child = child;
        child->next = NULL;
        child->parent = parent;
}

/******************************************************************************\
 Move a widget and all of its children.
\******************************************************************************/
void I_widget_move(i_widget_t *widget, c_vec2_t origin)
{
        i_widget_t *child;
        c_vec2_t diff;

        diff = C_vec2_sub(origin, widget->origin);
        widget->origin = origin;
        if (widget->event_func)
                widget->event_func(widget, I_EV_MOVED);
        child = widget->child;
        while (child) {
                I_widget_move(child, C_vec2_add(child->origin, diff));
                child = child->next;
        }
}

/******************************************************************************\
 Packs a widget's children.
\******************************************************************************/
void I_widget_pack(i_widget_t *widget, i_pack_t pack, i_collapse_t collapse)
{
        i_widget_t *child;
        c_vec2_t origin, size;

        if (pack < I_PACK_NONE || pack >= I_PACK_TYPES)
                C_error("Invalid pack type");
        origin = C_vec2_addf(widget->origin, i_border.value.n);
        size = C_vec2_subf(widget->size, i_border.value.n * 2);
        child = widget->child;
        while (child) {
                child->origin = origin;
                child->size = size;
                child->pack = pack;
                I_widget_event(child, I_EV_CONFIGURE);
                if (pack == I_PACK_H) {
                        origin.x += child->size.x + child->padding;
                        size.x -= child->size.x + child->padding;
                } else if (pack == I_PACK_V) {
                        origin.y += child->size.y + child->padding;
                        size.y -= child->size.y + child->padding;
                }
                child = child->next;
        }
        if (pack == I_PACK_H &&
            (size.x < 0.f || collapse != I_COLLAPSE_NONE)) {
                widget->size.x -= size.x;
                if (collapse == I_COLLAPSE_INVERT) {
                        origin = widget->origin;
                        origin.x += size.x;
                        I_widget_move(widget, origin);
                }
        } else if (pack == I_PACK_V &&
                   (size.y < 0.f || collapse != I_COLLAPSE_NONE)) {
                widget->size.y -= size.y;
                if (collapse == I_COLLAPSE_INVERT) {
                        origin = widget->origin;
                        origin.y += size.y;
                        I_widget_move(widget, origin);
                }
        }
}

/******************************************************************************\
 Propagates an event to all of the widget's children in order. This must be
 called from your event function if you want to propagate events to child
 widgets.
\******************************************************************************/
void I_widget_propagate(i_widget_t *widget, i_event_t event)
{
        i_widget_t *child, *child_next;

        child = widget->child;
        while (child) {
                child_next = child->next;
                I_widget_event(child, event);
                child = child_next;
        }
}

/******************************************************************************\
 Returns a statically allocated string containing the name of an event token.
\******************************************************************************/
const char *I_event_string(i_event_t event)
{
        switch (event) {
        case I_EV_NONE:
                return "I_EV_NONE";
        case I_EV_RENDER:
                return "I_EV_RENDER";
        case I_EV_CLEANUP:
                return "I_EV_CLEANUP";
        case I_EV_CONFIGURE:
                return "I_EV_CONFIGURE";
        case I_EV_KEY_DOWN:
                return "I_EV_KEY_DOWN";
        case I_EV_KEY_UP:
                return "I_EV_KEY_UP";
        case I_EV_MOUSE_IN:
                return "I_EV_MOUSE_IN";
        case I_EV_MOUSE_OUT:
                return "I_EV_MOUSE_OUT";
        case I_EV_MOUSE_MOVE:
                return "I_EV_MOUSE_MOVE";
        case I_EV_MOUSE_DOWN:
                return "I_EV_MOUSE_DOWN";
        case I_EV_MOUSE_UP:
                return "I_EV_MOUSE_UP";
        default:
                return "I_EV_INVALID";
        }
}

/******************************************************************************\
 Call on a widget in hover or active state to check if the widget has lost
 mouse focus this frame. Will generate I_EV_MOUSE_OUT when applicable.
\******************************************************************************/
static int check_mouse_out(i_widget_t *widget)
{
        if (!widget)
                return FALSE;
        if (i_mouse_x >= widget->origin.x &&
            i_mouse_y >= widget->origin.y &&
            i_mouse_x < widget->origin.x + widget->size.x &&
            i_mouse_y < widget->origin.y + widget->size.y)
                return FALSE;
        if (widget->state != I_WS_HOVER &&
            widget->state != I_WS_ACTIVE)
                return TRUE;
        if (i_debug.value.n >= 2)
                C_debug("I_EV_MOUSE_OUT --> %s",
                        widget->name);
        widget->event_func(widget, I_EV_MOUSE_OUT);
        widget->state = I_WS_READY;
        return TRUE;
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
        int propagate;

        if (!widget->name[0])
                C_error("Propagated event to uninitialized widget");

        /* The only event an unconfigured widget can handle is I_EV_CONFIGURE */
        if (widget->configured < 1 && event != I_EV_CONFIGURE)
                C_error("Propagated non-configure event to unconfigured %s",
                        widget->name);

        /* Print out the event in debug mode */
        if (i_debug.value.n >= 2)
                switch (event) {
                default:
                        C_debug("%s --> %s",
                                I_event_string(event), widget->name);
                case I_EV_RENDER:
                case I_EV_MOUSE_IN:
                case I_EV_MOUSE_OUT:
                        break;
                }

        /* Before handling and propagating event to children */
        switch (event) {
        case I_EV_CLEANUP:
                if (c_mem_check.value.n)
                        C_debug("Freeing %s", widget->name);
                break;
        case I_EV_CONFIGURE:
                if (c_mem_check.value.n)
                        C_debug("Configuring %s", widget->name);
                break;
        case I_EV_MOUSE_IN:
        case I_EV_MOUSE_OUT:
                /* These events are generated and handled directly from mouse
                   motion events and should never be propagated. */
                return;
        case I_EV_MOUSE_DOWN:
                if (widget->state == I_WS_READY || widget->state == I_WS_HOVER)
                        widget->state = I_WS_ACTIVE;
        case I_EV_MOUSE_UP:
        case I_EV_MOUSE_MOVE:

                /* Mouse left the widget area */
                if (check_mouse_out(widget))
                        return;

                /* Mouse entered the widget area */
                if (widget->state == I_WS_READY) {
                        if (i_debug.value.n >= 2)
                                C_debug("I_EV_MOUSE_IN --> %s", widget->name);
                        widget->event_func(widget, I_EV_MOUSE_IN);
                        if (widget->state == I_WS_READY)
                                widget->state = I_WS_HOVER;
                }

                mouse_focus = widget;
                break;
        case I_EV_MOVED:
                C_error("I_EV_MOVED should only be generated by "
                        "I_widget_move()");
        case I_EV_RENDER:
                if (widget->state == I_WS_HIDDEN) {
                        widget->fade -= i_fade.value.f * c_frame_sec;
                        if (widget->fade < 0.f) {
                                widget->fade = 0.f;
                                return;
                        }
                } else {
                        widget->fade += i_fade.value.f * c_frame_sec;
                        if (widget->fade > 1.f)
                                widget->fade = 1.f;
                }
                break;
        default:
                break;
        }

        /* Call widget-specific event handler and propagate event down */
        propagate = TRUE;
        if (widget->event_func)
                propagate = widget->event_func(widget, event);
        if (propagate)
                I_widget_propagate(widget, event);

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
        case I_EV_MOUSE_UP:
                if (widget->state == I_WS_ACTIVE)
                        widget->state = I_WS_HOVER;
                break;
        case I_EV_MOUSE_MOVE:
                if (widget->entry && mouse_focus == widget)
                        i_key_focus = widget;
                break;
        default:
                break;
        }
}

/******************************************************************************\
 Returns the string representation of a keycode. The string it stored in a
 C_va() buffer and should be considered temporary.
 TODO: Some common keycodes should get names.
\******************************************************************************/
const char *I_key_string(int sym)
{
        if (sym >= ' ' && sym < 0x7f)
                return C_va("%c", sym);
        return C_va("0x%x", sym);
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
                event = I_EV_KEY_DOWN;
                i_key = ev->key.keysym.sym;
                i_key_shift = ev->key.keysym.mod & KMOD_SHIFT;
                i_key_unicode = ev->key.keysym.unicode;
                if (i_debug.value.n > 0)
                        C_debug("SDL_KEYDOWN (%s%s)",
                                (i_key_shift ? "shift + " : ""),
                                I_key_string(i_key));
                break;
        case SDL_KEYUP:
                event = I_EV_KEY_UP;
                i_key = ev->key.keysym.sym;
                i_key_shift = ev->key.keysym.mod & KMOD_SHIFT;
                i_key_unicode = ev->key.keysym.unicode;
                if (i_debug.value.n > 0)
                        C_debug("SDL_KEYUP");
                break;
        case SDL_MOUSEMOTION:
                event = I_EV_MOUSE_MOVE;
                i_mouse_x = ev->motion.x / r_pixel_scale.value.f + 0.5f;
                i_mouse_y = ev->motion.y / r_pixel_scale.value.f + 0.5f;
                if (i_debug.value.n > 0)
                        C_debug("SDL_MOUSEMOTION (%dx%d)",
                                i_mouse_x, i_mouse_y);
                check_mouse_out(mouse_focus);
                break;
        case SDL_MOUSEBUTTONDOWN:
                event = I_EV_MOUSE_DOWN;
                i_mouse = ev->button.button;
                if (i_debug.value.n > 0)
                        C_debug("SDL_MOUSEBUTTONDOWN (%d)", i_mouse);
                check_mouse_out(mouse_focus);
                break;
        case SDL_MOUSEBUTTONUP:
                event = I_EV_MOUSE_UP;
                i_mouse = ev->button.button;
                if (i_debug.value.n > 0)
                        C_debug("SDL_MOUSEBUTTONUP");
                check_mouse_out(mouse_focus);
                break;
        default:
                return;
        }

        /* Normally dispatch up from the root window, but for key events send
           straight to the focused widget */
        if (event == I_EV_KEY_DOWN || event == I_EV_KEY_UP) {
                if (i_key_focus)
                        I_widget_event(i_key_focus, event);
        } else
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

