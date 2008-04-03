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

extern i_widget_t i_root;

c_color_t i_colors[I_COLORS];
i_widget_t *i_key_focus, *i_child;
int i_key, i_key_shift, i_key_unicode, i_mouse_x, i_mouse_y, i_mouse;

static i_widget_t *mouse_focus;
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
        if (i_debug.value.n)
                C_trace("Initialized %s", widget->name);
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
                              window->fit);
                if (window->decorated) {
                        R_window_init(&window->window, i_window.value.s);
                        window->window.sprite.origin = window->widget.origin;
                        window->window.sprite.size = window->widget.size;
                        R_sprite_cleanup(&window->hanger);
                        R_sprite_init(&window->hanger, i_hanger.value.s);
                        window->hanger.origin.y = window->widget.origin.y +
                                                  window->widget.size.y;
                        window->hanger.size.y = i_border.value.n;
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
        window->pack_children = I_PACK_V;
        window->decorated = TRUE;
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
                R_sprite_init_text(&label->text, label->font,
                                   label->widget.size.x, i_shadow.value.f,
                                   FALSE, label->buffer);
                label->widget.size = label->text.size;
                label->text.modulate = i_colors[label->color];
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
        label->font = R_FONT_GUI;
        C_strncpy_buf(label->buffer, text);
        I_widget_inited(&label->widget);
}

/******************************************************************************\
 Dynamically allocates a label widget.
\******************************************************************************/
i_label_t *I_label_new(const char *text)
{
        i_label_t *label;

        label = C_malloc(sizeof (*label));
        I_label_init(label, text);
        label->widget.heap = TRUE;
        return label;
}

/******************************************************************************\
 Attaches a child widget to another widget to the end of the parent's children
 linked list. Sends the I_EV_ADD_CHILD event.
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
        i_child = child;
        if (parent->event_func)
                parent->event_func(parent, I_EV_ADD_CHILD);
}

/******************************************************************************\
 Remove a widget from its parent container.
\******************************************************************************/
void I_widget_remove(i_widget_t *widget)
{
        if (!widget)
                return;
        if (widget->parent) {
                i_widget_t *prev;

                prev = widget->parent->child;
                if (prev != widget) {
                        while (prev->next != widget) {
                                if (!prev->next)
                                        C_error("%s is not its parent's child",
                                                widget->name);
                                prev = prev->next;
                        }
                        prev->next = widget->next;
                } else
                        widget->parent->child = widget->next;
        }
        widget->parent = NULL;
        widget->next = NULL;
        I_widget_event(widget, I_EV_CLEANUP);
}

/******************************************************************************\
 Move a widget and all of its children. Will not generate move events if the
 origin did not change.
\******************************************************************************/
void I_widget_move(i_widget_t *widget, c_vec2_t origin)
{
        i_widget_t *child;
        c_vec2_t diff;

        diff = C_vec2_sub(origin, widget->origin);
        if (!diff.x && !diff.y)
                return;
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
 Show or hide a widget and all of its children. Will not generate events if
 the visibility status did not change.
\******************************************************************************/
void I_widget_show(i_widget_t *widget, int show)
{
        if (widget->shown == show)
                return;
        if (show)
                I_widget_event(widget, I_EV_SHOW);
        else
                I_widget_event(widget, I_EV_HIDE);
}

/******************************************************************************\
 Gives extra space to expanding child widgets.
\******************************************************************************/
static void expand_children(i_widget_t *widget, c_vec2_t size, int expanders)
{
        c_vec2_t offset;
        i_widget_t *child;

        size = C_vec2_divf(size, expanders);
        offset = C_vec2(0.f, 0.f);
        child = widget->child;
        while (child) {
                if (!child->expand) {
                        I_widget_move(child, C_vec2_add(child->origin, offset));
                        child = child->next;
                        continue;
                }
                child->size = C_vec2_add(child->size, size);
                child->origin = C_vec2_add(child->origin, offset);
                I_widget_event(child, I_EV_CONFIGURE);
                offset = C_vec2_add(offset, size);
                child = child->next;
        }
}

/******************************************************************************\
 Packs a widget's children.
\******************************************************************************/
void I_widget_pack(i_widget_t *widget, i_pack_t pack, i_fit_t fit)
{
        i_widget_t *child;
        c_vec2_t origin, size;
        int expanders;

        /* First, let every widget claim the minimum space it requires */
        size = C_vec2_subf(widget->size, i_border.value.n * 2);
        origin = C_vec2_addf(widget->origin, i_border.value.n);
        expanders = 0;
        child = widget->child;
        while (child) {
                float padding;

                child->origin = origin;
                padding = child->padding * i_border.value.n;
                if (pack == I_PACK_H) {
                        child->size = C_vec2(0.f, size.y);
                        I_widget_event(child, I_EV_CONFIGURE);
                        origin.x += child->size.x + padding;
                        size.x -= child->size.x + padding;
                } else if (pack == I_PACK_V) {
                        child->size = C_vec2(size.x, 0.f);
                        I_widget_event(child, I_EV_CONFIGURE);
                        origin.y += child->size.y + padding;
                        size.y -= child->size.y + padding;
                }
                if (child->expand)
                        expanders++;
                child = child->next;
        }

        /* If there is left over space and we are not collapsing, assign it
           to any expandable widgets */
        if (fit == I_FIT_NONE) {
                if (pack == I_PACK_H && expanders && size.x > 0.f) {
                        size.y = 0.f;
                        expand_children(widget, size, expanders);
                } else if (pack == I_PACK_V && expanders && size.y > 0.f) {
                        size.x = 0.f;
                        expand_children(widget, size, expanders);
                }
                return;
        }

        /* This is a fitting widget so fit the widget to its contents */
        if (pack == I_PACK_H && size.x < 0.f) {
                widget->size.x -= size.x;
                if (fit == I_FIT_TOP) {
                        origin = widget->origin;
                        origin.x += size.x;
                        I_widget_move(widget, origin);
                }
        } else if (pack == I_PACK_V && size.y < 0.f) {
                widget->size.y -= size.y;
                if (fit == I_FIT_TOP) {
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
        case I_EV_ADD_CHILD:
                return "I_EV_ADD_CHILD";
        case I_EV_CLEANUP:
                return "I_EV_CLEANUP";
        case I_EV_CONFIGURE:
                return "I_EV_CONFIGURE";
        case I_EV_GRAB_FOCUS:
                return "I_EV_GRAB_FOCUS";
        case I_EV_HIDE:
                return "I_EV_HIDE";
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
        case I_EV_MOVED:
                return "I_EV_MOVED";
        case I_EV_RENDER:
                return "I_EV_RENDER";
        case I_EV_SHOW:
                return "I_EV_SHOW";
        default:
                if (event >= 0 && event < I_EVENTS)
                        C_warning("Forgot I_event_string() entry for event %d",
                                  event);
                return "I_EV_INVALID";
        }
}

/******************************************************************************\
 Call on a widget in hover or active state to check if the widget has lost
 mouse focus this frame. Will generate I_EV_MOUSE_OUT when applicable. Returns
 TRUE if the widget has lost mouse focus.
\******************************************************************************/
static int check_mouse_out(i_widget_t *widget)
{
        if (!widget)
                return TRUE;
        if (i_mouse_x >= widget->origin.x &&
            i_mouse_y >= widget->origin.y &&
            i_mouse_x < widget->origin.x + widget->size.x &&
            i_mouse_y < widget->origin.y + widget->size.y &&
            widget->shown && widget->state != I_WS_DISABLED)
                return FALSE;
        if (widget->state != I_WS_HOVER && widget->state != I_WS_ACTIVE)
                return TRUE;
        if (i_debug.value.n >= 2)
                C_trace("I_EV_MOUSE_OUT --> %s", widget->name);
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

 Always call this function instead of a widget's class event function in order
 to ensure correct event filtering and propagation.
\******************************************************************************/
void I_widget_event(i_widget_t *widget, i_event_t event)
{
        int propagate, handle;

        if (!widget->name[0])
                C_error("Propagated event to uninitialized widget");
        propagate = TRUE;
        handle = TRUE;

        /* The only event an unconfigured widget can handle is I_EV_CONFIGURE */
        if (widget->configured < 1 && event != I_EV_CONFIGURE)
                C_error("Propagated %s to unconfigured %s",
                        I_event_string(event), widget->name);

        /* Print out the event in debug mode */
        if (i_debug.value.n >= 2)
                switch (event) {
                default:
                        C_trace("%s --> %s",
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
                        C_trace("Freeing %s", widget->name);
                break;
        case I_EV_CONFIGURE:
                if (c_mem_check.value.n)
                        C_trace("Configuring %s", widget->name);
                break;
        case I_EV_GRAB_FOCUS:
                if (widget->entry && widget->shown &&
                    widget->state != I_WS_DISABLED)
                        i_key_focus = widget;
                break;
        case I_EV_HIDE:
                widget->shown = FALSE;
                if (i_key_focus == widget)
                        i_key_focus = NULL;
                break;
        case I_EV_MOUSE_IN:
        case I_EV_MOUSE_OUT:
                /* These events are generated and handled directly from mouse
                   motion events and should never be propagated. */
                return;
        case I_EV_MOUSE_DOWN:
                if (widget->state == I_WS_READY || widget->state == I_WS_HOVER)
                        widget->state = I_WS_ACTIVE;
                handle = FALSE;
        case I_EV_MOUSE_MOVE:

                /* Mouse left the widget area */
                if (check_mouse_out(widget))
                        return;

                /* Mouse entered the widget area */
                if (widget->state == I_WS_READY) {
                        if (i_debug.value.n >= 2)
                                C_trace("I_EV_MOUSE_IN --> %s", widget->name);
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
                if (!i_debug.value.n && widget->parent &&
                    !C_rect_intersect(widget->origin, widget->size,
                                      widget->parent->origin,
                                      widget->parent->size))
                        return;
                if (widget->shown) {
                        widget->fade += i_fade.value.f * c_frame_sec;
                        if (widget->fade > 1.f)
                                widget->fade = 1.f;
                } else {
                        widget->fade -= i_fade.value.f * c_frame_sec;
                        if (widget->fade < 0.f) {
                                widget->fade = 0.f;
                                return;
                        }
                }
                break;
        case I_EV_SHOW:
                widget->shown = TRUE;
                break;
        default:
                break;
        }

        /* Call widget-specific event handler and propagate event down */
        if (widget->event_func && handle)
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
        case I_EV_MOUSE_DOWN:
                if (mouse_focus == widget && widget->event_func)
                        widget->event_func(widget, event);
                break;
        case I_EV_MOUSE_UP:
                if (widget->state == I_WS_ACTIVE)
                        widget->state = I_WS_HOVER;
                break;
        case I_EV_MOUSE_MOVE:
                if (mouse_focus == widget && widget->entry)
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
                        C_trace("SDL_KEYDOWN (%s%s)",
                                (i_key_shift ? "shift + " : ""),
                                I_key_string(i_key));
                if (!i_key) {
                        C_warning("SDL sent zero keysym");
                        return;
                }
                break;
        case SDL_KEYUP:
                event = I_EV_KEY_UP;
                i_key = ev->key.keysym.sym;
                i_key_shift = ev->key.keysym.mod & KMOD_SHIFT;
                i_key_unicode = ev->key.keysym.unicode;
                if (i_debug.value.n > 0)
                        C_trace("SDL_KEYUP");
                break;
        case SDL_MOUSEMOTION:
                event = I_EV_MOUSE_MOVE;
                i_mouse_x = ev->motion.x / r_pixel_scale.value.f + 0.5f;
                i_mouse_y = ev->motion.y / r_pixel_scale.value.f + 0.5f;
                if (i_debug.value.n > 0)
                        C_trace("SDL_MOUSEMOTION (%dx%d)",
                                i_mouse_x, i_mouse_y);
                check_mouse_out(mouse_focus);
                break;
        case SDL_MOUSEBUTTONDOWN:
                event = I_EV_MOUSE_DOWN;
                i_mouse = ev->button.button;
                if (i_debug.value.n > 0)
                        C_trace("SDL_MOUSEBUTTONDOWN (%d)", i_mouse);
                check_mouse_out(mouse_focus);
                break;
        case SDL_MOUSEBUTTONUP:
                event = I_EV_MOUSE_UP;
                i_mouse = ev->button.button;
                if (i_debug.value.n > 0)
                        C_trace("SDL_MOUSEBUTTONUP");
                check_mouse_out(mouse_focus);
                break;
        default:
                return;
        }

        /* Normally dispatch up from the root window, but for key events send
           straight to the focused widget */
        if (event == I_EV_KEY_DOWN) {
                if (i_key_focus && i_key_focus->event_func &&
                    i_key_focus->shown && i_key_focus->state != I_WS_DISABLED)
                        i_key_focus->event_func(i_key_focus, event);
        } else
                I_widget_event(&i_root, event);

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

