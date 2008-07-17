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

/* Colors in this array are kept up to date when the theme changes */
c_color_t i_colors[I_COLORS];

/* Points to the widget that currently holds keyboard focus */
i_widget_t *i_key_focus;

/* This is the child widget for an I_EV_ADD_CHILD event */
i_widget_t *i_child;

/* The most deeply nested widget that the mouse is hovering over */
i_widget_t *i_mouse_focus;

/* Event parameters */
int i_key, i_key_shift, i_key_unicode, i_mouse_x, i_mouse_y, i_mouse;

static int widgets;

/******************************************************************************\
 Initializes a widget name field with a unique name that includes the
 widget's class.
\******************************************************************************/
void I_widget_init(i_widget_t *widget, const char *class_name)
{
        C_assert(widget);
        C_zero(widget);
        snprintf(widget->name, sizeof (widget->name),
                 "widget #%u (%s)", widgets++, class_name);
        if (c_mem_check.value.n && i_debug.value.n)
                C_trace("Initialized %s", widget->name);
        widget->shown = TRUE;
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
 Remove all of the widget's children.
\******************************************************************************/
void I_widget_remove_children(i_widget_t *widget, int cleanup)
{
        i_widget_t *child, *child_next;

        if (!widget)
                return;
        for (child = widget->child; child; child = child_next) {
                child_next = child->next;
                child->parent = NULL;
                child->next = NULL;
                if (cleanup)
                        I_widget_event(child, I_EV_CLEANUP);
        }
        widget->child = NULL;
}

/******************************************************************************\
 Remove a widget from its parent container and free its memory.
\******************************************************************************/
void I_widget_remove(i_widget_t *widget, int cleanup)
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
        if (cleanup)
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
        child = widget->child;
        while (child) {
                I_widget_move(child, C_vec2_add(child->origin, diff));
                child = child->next;
        }
        if (widget->event_func)
                widget->event_func(widget, I_EV_MOVED);
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

        size = C_vec2_divf(size, (float)expanders);
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
        size = C_vec2_subf(widget->size,
                           widget->padding * i_border.value.n * 2.f);
        origin = C_vec2_addf(widget->origin,
                             widget->padding * i_border.value.n);
        expanders = 0;
        child = widget->child;
        while (child) {
                float margin_front, margin_rear;

                margin_front = child->margin_front * i_border.value.n;
                margin_rear = child->margin_rear * i_border.value.n;
                if (pack == I_PACK_H) {
                        origin.x += margin_front;
                        child->origin = origin;
                        child->size = C_vec2(0.f, size.y);
                        I_widget_event(child, I_EV_CONFIGURE);
                        origin.x += child->size.x + margin_front + margin_rear;
                        size.x -= child->size.x + margin_front + margin_rear;
                } else if (pack == I_PACK_V) {
                        origin.y += child->margin_front * i_border.value.n;
                        child->origin = origin;
                        child->size = C_vec2(size.x, 0.f);
                        I_widget_event(child, I_EV_CONFIGURE);
                        origin.y += child->size.y + margin_front + margin_rear;
                        size.y -= child->size.y + margin_front + margin_rear;
                }
                if (child->expand)
                        expanders++;
                child = child->next;
        }

        /* If there is left over space and we are not collapsing, assign it
           to any expandable widgets */
        if (fit == I_FIT_NONE) {
                if (pack == I_PACK_H && expanders) {
                        size.y = 0.f;
                        expand_children(widget, size, expanders);
                } else if (pack == I_PACK_V && expanders) {
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
 Returns the width and height of a rectangle that would encompass the widget
 including margins.
\******************************************************************************/
c_vec2_t I_widget_bounds(const i_widget_t *widget, i_pack_t pack)
{
        c_vec2_t size;
        float margin_front, margin_rear;

        size = widget->size;
        margin_front = widget->margin_front * i_border.value.n;
        margin_rear = widget->margin_rear * i_border.value.n;
        if (pack == I_PACK_H)
                size.x += margin_front + margin_rear;
        else if (pack == I_PACK_V)
                size.y += margin_front + margin_rear;
        return size;
}

/******************************************************************************\
 Returns the width and height of a rectangle from the widget's origin that
 would encompass it's child widgets.
\******************************************************************************/
c_vec2_t I_widget_child_bounds(const i_widget_t *widget)
{
        c_vec2_t bounds, corner;
        i_widget_t *child;

        child = widget->child;
        bounds = C_vec2(0.f, 0.f);
        while (child) {
                corner = C_vec2_add(C_vec2_sub(child->origin, widget->origin),
                                    child->size);
                if (bounds.x < corner.x)
                        bounds.x = corner.x;
                if (bounds.y < corner.y)
                        bounds.y = corner.y;
                child = child->next;
        }
        return bounds;
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
 Call on a widget in hover or active state to check if the widget has mouse
 focus this frame. Will generate I_EV_MOUSE_OUT when applicable. Returns
 TRUE if the widget has mouse focus.
\******************************************************************************/
static int check_mouse_focus(i_widget_t *widget)
{
        if (!widget)
                return FALSE;
        if (i_mouse_x >= widget->origin.x && i_mouse_y >= widget->origin.y &&
            i_mouse_x < widget->origin.x + widget->size.x &&
            i_mouse_y < widget->origin.y + widget->size.y &&
            widget->shown && widget->clickable &&
            widget->state != I_WS_DISABLED) {
                i_mouse_focus = widget;
                return TRUE;
        }
        if (widget->state == I_WS_HOVER || widget->state == I_WS_ACTIVE)
                I_widget_event(widget, I_EV_MOUSE_OUT);
        return FALSE;
}

/******************************************************************************\
 When a widget is hidden, propagates up to the widget's parent and then to its
 parent and so on until mouse focus is claimed. Does nothing if the mouse is
 not within widget bounds.
\******************************************************************************/
static void mouse_focus_parent(i_widget_t *widget)
{
        i_widget_t *p;

        /* See if the focus is this widget or within this widget */
        if (!i_mouse_focus)
                return;
        p = i_mouse_focus;
        while (p != widget) {
                if (!p->parent)
                        return;
                p = p->parent;
        }

        /* Propagate mouse out events */
        p = i_mouse_focus;
        while (p != widget) {
                I_widget_event(p, I_EV_MOUSE_OUT);
                p = p->parent;
        }
        I_widget_event(p, I_EV_MOUSE_OUT);

        /* Propagate focus up */
        while (widget->parent) {
                widget = widget->parent;
                if (check_mouse_focus(widget))
                        return;
        }
        i_mouse_focus = &i_root;
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
        if (!widget->name[0] || !widget->event_func)
                C_error("Propagated event to uninitialized widget");

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
                case I_EV_MOUSE_MOVE:
                case I_EV_GRAB_FOCUS:
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
                mouse_focus_parent(widget);
                widget->event_func(widget, event);
                return;
        case I_EV_MOUSE_IN:
                if (widget->state == I_WS_READY && widget->clickable)
                        widget->state = I_WS_HOVER;
                widget->event_func(widget, event);
                return;
        case I_EV_MOUSE_OUT:
                if (widget->state == I_WS_HOVER || widget->state == I_WS_ACTIVE)
                        widget->state = I_WS_READY;
                widget->event_func(widget, event);
                return;
        case I_EV_MOUSE_MOVE:

                /* See if mouse left the widget area */
                if (!check_mouse_focus(widget))
                        return;

                /* Mouse entered the widget area */
                if (widget->state == I_WS_READY)
                        I_widget_event(widget, I_EV_MOUSE_IN);

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
                if (i_fade.value.f > 0.f) {
                        float target, rate;

                        target = 0.f;
                        if (widget->shown) {
                                target = 1.f;
                                if (widget->parent)
                                        target = widget->parent->fade;
                        }
                        rate = i_fade.value.f * c_frame_sec;
                        if (widget->state == I_WS_DISABLED) {
                                target /= 4.f;
                                if (widget->fade <= 0.25f)
                                        rate /= 4.f;
                        }
                        if (widget->fade < target) {
                                widget->fade += rate;
                                if (widget->fade > target)
                                        widget->fade = target;
                        } else if (widget->fade > target) {
                                widget->fade -= rate;
                                if (widget->fade < target)
                                        widget->fade = target;
                        }
                        if (widget->fade <= 0.f)
                                return;
                } else if (!widget->shown)
                        return;
                break;
        case I_EV_SHOW:
                widget->shown = TRUE;
                check_mouse_focus(widget);
                widget->event_func(widget, event);
                return;
        default:
                break;
        }

        /* Call widget-specific event handler and propagate event down */
        if (widget->event_func(widget, event))
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
        case I_EV_MOUSE_MOVE:
                if (i_mouse_focus == widget && widget->entry)
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
                                I_key_string(i_key_unicode));
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
                        C_trace("SDL_KEYUP (%s%s)",
                                (i_key_shift ? "shift + " : ""),
                                I_key_string(i_key_unicode));
                break;
        case SDL_MOUSEMOTION:
                event = I_EV_MOUSE_MOVE;
                i_mouse_x = (int)(ev->motion.x / r_pixel_scale.value.f + 0.5f);
                i_mouse_y = (int)(ev->motion.y / r_pixel_scale.value.f + 0.5f);
                check_mouse_focus(i_mouse_focus);
                break;
        case SDL_MOUSEBUTTONDOWN:
                event = I_EV_MOUSE_DOWN;
                i_mouse = ev->button.button;
                if (i_debug.value.n > 0)
                        C_trace("SDL_MOUSEBUTTONDOWN (%d)", i_mouse);
                check_mouse_focus(i_mouse_focus);
                break;
        case SDL_MOUSEBUTTONUP:
                event = I_EV_MOUSE_UP;
                i_mouse = ev->button.button;
                if (i_debug.value.n > 0)
                        C_trace("SDL_MOUSEBUTTONUP (%d)", i_mouse);
                check_mouse_focus(i_mouse_focus);
                break;
        default:
                return;
        }

        /* Normally dispatch up from the root window, but for key and mouse
           down events send straight to the focused widget */
        if (event == I_EV_KEY_DOWN) {
                if (i_key_focus && i_key_focus->event_func &&
                    i_key_focus->shown && i_key_focus->state != I_WS_DISABLED)
                        i_key_focus->event_func(i_key_focus, event);
        } else if (event == I_EV_MOUSE_DOWN) {
                if (i_mouse_focus && i_mouse_focus->event_func &&
                    i_mouse_focus->shown &&
                    i_mouse_focus->state != I_WS_DISABLED)
                        i_mouse_focus->event_func(i_mouse_focus, event);
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

