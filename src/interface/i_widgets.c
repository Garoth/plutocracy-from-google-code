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

void I_console_init(i_window_t *window);

c_color_t i_colors[I_COLORS];
i_widget_t *i_key_focus, *i_child;
int i_key, i_key_shift, i_key_unicode, i_mouse_x, i_mouse_y, i_mouse;

static i_widget_t root, *mouse_focus;
static i_window_t left_toolbar, console_window;
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
        if (i_debug.value.n)
                C_trace("Initialized %s", widget->name);
}

/******************************************************************************\
 Root window event function.
 TODO: Stuff like rotating the globe and other interactions will happen here
       because this is the catch all for mouse/keyboard events that don't get
       handled.
\******************************************************************************/
static int root_event(i_widget_t *root, i_event_t event)
{
        if (event == I_EV_CONFIGURE) {
                i_colors[I_COLOR] = C_color_string(i_color.value.s);
                i_colors[I_COLOR_ALT] = C_color_string(i_color2.value.s);
        }
        return TRUE;
}

/******************************************************************************\
 Parses the theme configuration file. This is called before the initialization
 functions in case there are font changes in the theme.
\******************************************************************************/
void I_parse_config(void)
{
        C_var_unlatch(&i_theme);
        C_parse_config_file(i_theme.value.s);
}

/******************************************************************************\
 Updates after a theme change.
\******************************************************************************/
static void theme_configure(void)
{
        C_var_unlatch(&i_border);
        C_var_unlatch(&i_button);
        C_var_unlatch(&i_button_active);
        C_var_unlatch(&i_button_hover);
        C_var_unlatch(&i_color);
        C_var_unlatch(&i_color2);
        C_var_unlatch(&i_shadow);
        C_var_unlatch(&i_window);
        C_var_unlatch(&i_work_area);

        /* Root window */
        root.size = C_vec2(r_width_2d, r_height_2d);

        /* Left toolbar */
        left_toolbar.widget.size = C_vec2(128.f, 48.f);
        left_toolbar.widget.origin = C_vec2(i_border.value.n, r_height_2d -
                                            left_toolbar.widget.size.y -
                                            i_border.value.n);

        /* Console window */
        console_window.widget.size = C_vec2(480.f, 240.f);
        console_window.widget.origin = C_vec2(i_border.value.n, r_height_2d -
                                              console_window.widget.size.y -
                                              left_toolbar.widget.size.y -
                                              i_border.value.n * 2);
}

/******************************************************************************\
 Update function for i_theme. Will also reload fonts.
\******************************************************************************/
static int theme_update(c_var_t *var, c_var_value_t value)
{
        if (!C_parse_config_file(value.s))
                return FALSE;
        theme_configure();
        R_free_fonts();
        R_load_fonts();
        I_widget_event(&root, I_EV_CONFIGURE);
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
        root.configured = 1;
        root.entry = TRUE;

        /* Left toolbar */
        I_window_init(&left_toolbar);
        left_toolbar.pack_children = I_PACK_H;
        I_widget_add(&root, &left_toolbar.widget);

        /* Console button */
        I_button_init(&console_button, NULL, "Test Button", TRUE);
        I_widget_add(&left_toolbar.widget, &console_button.widget);

        /* Console window */
        I_window_init(&console_window);
        I_console_init(&console_window);
        I_widget_add(&root, &console_window.widget);

        /* Theme can now be modified dynamically */
        theme_configure();
        I_widget_event(&root, I_EV_CONFIGURE);
        i_theme.update = (c_var_update_f)theme_update;
        i_theme.edit = C_VE_FUNCTION;
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
                              window->fit);
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
        if (widget == &root)
                C_error("Tried to remove root window");
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
 Gives extra space to expanding child widgets.
\******************************************************************************/
static void expand_children(i_widget_t *widget, c_vec2_t size, int expanders)
{
        c_vec2_t offset;
        i_widget_t *child;

        size = C_vec2_invscalef(size, expanders);
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
                child->origin = origin;
                if (pack == I_PACK_H) {
                        child->size = C_vec2(0.f, size.y);
                        I_widget_event(child, I_EV_CONFIGURE);
                        origin.x += child->size.x + child->padding;
                        size.x -= child->size.x + child->padding;
                } else if (pack == I_PACK_V) {
                        child->size = C_vec2(size.x, 0.f);
                        I_widget_event(child, I_EV_CONFIGURE);
                        origin.y += child->size.y + child->padding;
                        size.y -= child->size.y + child->padding;
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

        /* This is a collapsing widget so fit the widget to its contents */
        if (pack == I_PACK_H) {
                widget->size.x -= size.x;
                if (fit == I_FIT_TOP) {
                        origin = widget->origin;
                        origin.x += size.x;
                        I_widget_move(widget, origin);
                }
        } else if (pack == I_PACK_V) {
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
                        C_trace("SDL_KEYDOWN (%s%s)",
                                (i_key_shift ? "shift + " : ""),
                                I_key_string(i_key));
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
        if (event == I_EV_KEY_DOWN || event == I_EV_KEY_UP) {
                if (i_key_focus && i_key_focus->event_func)
                        i_key_focus->event_func(i_key_focus, event);
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

