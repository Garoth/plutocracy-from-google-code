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

extern c_var_t i_button, i_button_active, i_button_hover,
               i_shadow, i_theme, i_window;

SDLKey i_key;
int i_key_shift, i_mouse_x, i_mouse_y, i_mouse;

static i_widget_t root, *key_focus, *mouse_focus;
static i_toolbar_t left_toolbar;
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
static void root_event(i_widget_t *root, i_event_t event)
{
}

/******************************************************************************\
 Loads interface assets and initializes windows.
\******************************************************************************/
void I_init(void)
{
        C_status("Initializing interface");

        /* Load theme */
        C_parse_config_file(i_theme.value.s);

        /* Root window */
        C_zero(&root);
        I_widget_set_name(&root, "Root Window");
        root.event_func = (i_event_f)root_event;
        root.origin = C_vec2(0.f, 0.f);
        root.size = C_vec2(r_width_2d, r_height_2d);
        root.configured = 1;

        /* Left Toolbar */
        I_toolbar_init(&left_toolbar);
        left_toolbar.widget.size = C_vec2(128.f, 48.f);
        left_toolbar.widget.origin = C_vec2(i_border.value.n, r_height_2d -
                                            left_toolbar.widget.size.y -
                                            i_border.value.n);
        I_widget_add(&root, &left_toolbar.widget);

        /* Console Button */
        I_button_init(&console_button, NULL, "Test Button", TRUE);
        /*console_button.widget.origin = C_vec2(100.f, 100.f);
        console_button.widget.size = C_vec2(128.f, 32.f);*/
        I_widget_add(&left_toolbar.widget, &console_button.widget);

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
 Toolbar event function.
\******************************************************************************/
static void toolbar_event(i_toolbar_t *toolbar, i_event_t event)
{
        switch (event) {
        case I_EV_CONFIGURE:
                if (toolbar->widget.configured)
                        R_window_cleanup(&toolbar->window);
                R_window_init(&toolbar->window, i_window.value.s);
                toolbar->window.sprite.origin = toolbar->widget.origin;
                toolbar->window.sprite.size = toolbar->widget.size;
                I_widget_pack(&toolbar->widget, I_PACK_H);
                return;
        case I_EV_CLEANUP:
                if (!toolbar->widget.configured)
                        break;
                R_window_cleanup(&toolbar->window);
                break;
        case I_EV_RENDER:
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
        C_zero(toolbar);
        I_widget_set_name(&toolbar->widget, "Toolbar");
        toolbar->widget.event_func = (i_event_f)toolbar_event;
        toolbar->widget.state = I_WS_READY;
        I_widget_inited(&toolbar->widget);
}

/******************************************************************************\
 Button event function.
\******************************************************************************/
static void button_event(i_button_t *button, i_event_t event)
{
        c_vec2_t origin, size;
        float width;

        switch (event) {
        case I_EV_CONFIGURE:
                if (button->widget.configured) {
                        R_window_cleanup(&button->normal);
                        R_window_cleanup(&button->active);
                        R_window_cleanup(&button->hover);
                }

                /* Setup decorations (for each state) to cover full area */
                origin = button->widget.origin;
                size = button->widget.size;
                if (button->decorated) {

                        /* No focus */
                        R_window_init(&button->normal, i_button.value.s);
                        button->normal.sprite.origin = origin;
                        button->normal.sprite.size = size;

                        /* Mouse is hovering over button */
                        R_window_init(&button->hover, i_button_hover.value.s);
                        button->hover.sprite.origin = origin;
                        button->hover.sprite.size = size;

                        /* Mouse is clicking on the button */
                        R_window_init(&button->active, i_button_active.value.s);
                        button->active.sprite.origin = origin;
                        button->active.sprite.size = size;
                }

                /* Pack the icon left, vertically centered */
                origin.x += i_border.value.n;
                origin.y += size.y / 2;
                size = C_vec2_subf(size, i_border.value.n * 2);
                if (button->icon.texture) {
                        button->icon.origin = C_vec2(origin.x, origin.y -
                                                     button->icon.size.y / 2);
                        width = button->icon.size.x + i_border.value.n;
                        origin.x += width;
                        size.x -= width;
                }

                /* Wrap the text with available space and pack */
                R_text_set_text(&button->text, R_FONT_GUI, size.x,
                                i_shadow.value.f, button->text.string);
                button->text.sprite.origin = origin;
                button->text.sprite.origin.y -= button->text.sprite.size.y / 2;

                /* If there is some room left, center the icon and label */
                width = (size.x - button->text.sprite.size.x) / 2.f;
                if (width > 0.f) {
                        button->icon.origin.x += width;
                        button->text.sprite.origin.x += width;
                }

                break;
        case I_EV_CLEANUP:
                R_window_cleanup(&button->normal);
                R_window_cleanup(&button->active);
                R_window_cleanup(&button->hover);
                R_sprite_cleanup(&button->icon);
                R_text_cleanup(&button->text);
                break;
        case I_EV_MOUSE_DOWN:
                if (i_mouse == SDL_BUTTON_LEFT)
                        button->widget.state = I_WS_ACTIVE;
                break;
        case I_EV_MOUSE_UP:
                if (button->widget.state == I_WS_ACTIVE && button->on_click)
                        button->on_click(button);
                break;
        case I_EV_RENDER:
                R_sprite_render(&button->icon);
                R_text_render(&button->text);
                if (button->widget.state == I_WS_HOVER)
                        R_window_render(&button->hover);
                else if (button->widget.state == I_WS_ACTIVE)
                        R_window_render(&button->active);
                else
                        R_window_render(&button->normal);
                break;
        default:
                break;
        }
}

/******************************************************************************\
 Initializes a button. The [icon] path and [text] strings can be NULL and do
 not need to persist after calling the function. Call with [bg] set to TRUE to
 decorate the button background.
\******************************************************************************/
void I_button_init(i_button_t *button, const char *icon, const char *text,
                   int bg)
{
        C_zero(button);
        I_widget_set_name(&button->widget, "Button");
        button->widget.event_func = (i_event_f)button_event;
        button->widget.state = I_WS_READY;
        button->on_click = NULL;
        button->decorated = bg;

        /* We can initialize the icon sprite now */
        R_sprite_init(&button->icon, icon);

        /* The text depends on our dimensions though, so just cache it in
           the text structure itself */
        R_text_init(&button->text);
        C_strncpy_buf(button->text.string, text);

        I_widget_inited(&button->widget);
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
 Packs a widget's children.
\******************************************************************************/
void I_widget_pack(i_widget_t *widget, i_pack_t pack)
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
                        origin.x += child->size.x;
                        size.x -= child->size.x;
                } else if (pack == I_PACK_V) {
                        origin.y += child->size.y;
                        size.y -= child->size.y;
                }
                child = child->next;
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
        case I_EV_KEY_DOWN:
        case I_EV_KEY_UP:
                if (key_focus != widget)
                        return;
                break;
        case I_EV_MOUSE_IN:
        case I_EV_MOUSE_OUT:
                /* These events are generated and handled directly from mouse
                   motion events and should never be propagated. */
                return;
        case I_EV_MOUSE_DOWN:
                if (widget->state == I_WS_READY ||
                    widget->state == I_WS_HOVER)
                        widget->state = I_WS_ACTIVE;
        case I_EV_MOUSE_UP:
        case I_EV_MOUSE_MOVE:

                /* Mouse left the widget area */
                if (i_mouse_x < widget->origin.x ||
                    i_mouse_y < widget->origin.y ||
                    i_mouse_x >= widget->origin.x + widget->size.x ||
                    i_mouse_y >= widget->origin.y + widget->size.y) {
                        if (widget->state == I_WS_HOVER ||
                            widget->state == I_WS_ACTIVE) {
                                if (i_debug.value.n >= 2)
                                        C_debug("I_EV_MOUSE_OUT --> %s",
                                                widget->name);
                                widget->event_func(widget, I_EV_MOUSE_OUT);
                                widget->state = I_WS_READY;
                        }
                        return;
                }

                /* Mouse entered the widget area */
                if (widget->state == I_WS_READY) {
                        if (i_debug.value.n >= 2)
                                C_debug("I_EV_MOUSE_IN --> %s", widget->name);
                        widget->event_func(widget, I_EV_MOUSE_IN);
                        if (widget->state == I_WS_READY)
                                widget->state = I_WS_HOVER;
                        if (widget->state == I_WS_ENTRY)
                                key_focus = widget;
                }

                mouse_focus = widget;
                break;
        default:
                break;
        }

        /* Call widget-specific event handler and propagate event down */
        if (widget->event_func)
                widget->event_func(widget, event);
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
                if (ev->key.keysym.sym == SDLK_RSHIFT ||
                    ev->key.keysym.sym == SDLK_LSHIFT || i_key)
                        return;
                event = I_EV_KEY_DOWN;
                i_key = ev->key.keysym.sym;
                i_key_shift = ev->key.keysym.mod & KMOD_SHIFT;
                if (i_debug.value.n > 0)
                        C_debug("SDL_KEYDOWN (%s%s)",
                                (i_key_shift ? "shift + " : ""),
                                I_key_string(i_key));
                break;
        case SDL_KEYUP:
                if (ev->key.keysym.sym == SDLK_RSHIFT ||
                    ev->key.keysym.sym == SDLK_LSHIFT ||
                    i_key != ev->key.keysym.sym)
                        return;
                event = I_EV_KEY_DOWN;
                i_key_shift = ev->key.keysym.mod & KMOD_SHIFT;
                if (i_debug.value.n > 0)
                        C_debug("SDL_KEYUP");
                break;
        case SDL_MOUSEMOTION:
                event = I_EV_MOUSE_MOVE;
                i_mouse_x = ev->motion.x;
                i_mouse_y = ev->motion.y;
                key_focus = NULL;
                if (i_debug.value.n > 0)
                        C_debug("SDL_MOUSEMOTION (%dx%d)",
                                i_mouse_x, i_mouse_y);
                break;
        case SDL_MOUSEBUTTONDOWN:
                if (i_mouse)
                        return;
                event = I_EV_MOUSE_DOWN;
                i_mouse = ev->button.button;
                i_mouse_x = ev->button.x;
                i_mouse_y = ev->button.y;
                key_focus = NULL;
                if (i_debug.value.n > 0)
                        C_debug("SDL_MOUSEBUTTONDOWN (%d)", i_mouse);
                break;
        case SDL_MOUSEBUTTONUP:
                if (!i_mouse || i_mouse != ev->button.button)
                        return;
                event = I_EV_MOUSE_UP;
                i_mouse_x = ev->button.x;
                i_mouse_y = ev->button.y;
                key_focus = NULL;
                if (i_debug.value.n > 0)
                        C_debug("SDL_MOUSEBUTTONUP");
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

