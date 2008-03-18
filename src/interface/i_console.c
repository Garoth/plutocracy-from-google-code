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
static i_window_t window;
static i_entry_t entry;
static i_scrollback_t scrollback;
static int cols_max;

/******************************************************************************\
 Align the scrollback child widgets.
\******************************************************************************/
static void scrollback_moved(i_scrollback_t *sb)
{
        i_widget_t *child;
        c_vec2_t origin;
        float height;

        /* Find the total height */
        child = sb->widget.child;
        height = 0.f;
        while (child) {
                height += child->size.y;
                child = child->next;
        }

        /* Re-align the widgets */
        child = sb->widget.child;
        origin = sb->widget.origin;
        origin.y -= height + sb->scroll;
        while (child) {
                I_widget_move(child, origin);
                origin.y += child->size.y;
                child = child->next;
        }
}

/******************************************************************************\
 Scrollback widget event function.
\******************************************************************************/
int I_scrollback_event(i_scrollback_t *sb, i_event_t event)
{
        switch (event) {
        case I_EV_CONFIGURE:
                R_window_cleanup(&sb->window);
                R_window_init(&sb->window, i_work_area.value.s);
                sb->window.sprite.size = sb->widget.size;
                sb->window.sprite.origin = sb->widget.origin;
                I_widget_pack(&sb->widget, I_PACK_V, I_COLLAPSE_NONE);
                scrollback_moved(sb);
                return FALSE;
        case I_EV_MOVED:
                sb->window.sprite.origin = sb->widget.origin;
                scrollback_moved(sb);
                break;
        case I_EV_CLEANUP:
                R_window_cleanup(&sb->window);
                break;
        case I_EV_RENDER:
                R_window_render(&sb->window);
                R_clip_rect(sb->widget.origin, sb->widget.size);
                R_clip_push();
                I_widget_propagate(&sb->widget, event);
                R_clip_pop();
                return FALSE;
        default:
                break;
        }
        return TRUE;
}

/******************************************************************************\
 Scrollback widget initialization.
\******************************************************************************/
i_widget_t *I_scrollback_init(i_scrollback_t *sb)
{
        C_zero(sb);
        I_widget_set_name(&sb->widget, "Scrollback");
        sb->widget.event_func = (i_event_f)I_scrollback_event;
        sb->widget.state = I_WS_READY;
        I_widget_inited(&sb->widget);
        return (i_widget_t *)sb;
}

/******************************************************************************\
 Callback for pressing enter on the console entry field.
\******************************************************************************/
static void on_enter(i_entry_t *entry)
{
        I_entry_configure(entry, NULL);
}

/******************************************************************************\
 Catches the scrollback's configure events so we know where to wrap incoming
 text.
\******************************************************************************/
static int scrollback_event(i_scrollback_t *sb, i_event_t event)
{
        if (event == I_EV_CONFIGURE)
                cols_max = sb->widget.size.x / R_font_width(R_FONT_CONSOLE);
        return I_scrollback_event(sb, event);
}

/******************************************************************************\
 Called whenever something is printed to the log.
\******************************************************************************/
static void log_handler(c_log_level_t level, const char *string)
{
}

/******************************************************************************\
 Initializes and returns a pointer to the console window.
\******************************************************************************/
i_widget_t *I_console_init(void)
{
        /* Set log handler */
        c_log_func = log_handler;

        /* Window container */
        I_window_init(&window);
        window.widget.size = C_vec2(320.f, 240.f);
        window.widget.origin = C_vec2(i_border.value.n, r_height_2d -
                                      window.widget.size.y - 64.f -
                                      i_border.value.n * 2);
        window.collapse = I_COLLAPSE_INVERT;

        /* Label */
        I_label_init(&label, "Console:");
        I_widget_add(&window.widget, &label.widget);

        /* Scrollback area */
        I_scrollback_init(&scrollback);
        scrollback.widget.event_func = (i_event_f)scrollback_event;
        scrollback.widget.padding = i_border.value.n / 2;
        I_widget_add(&window.widget, &scrollback.widget);

        /* Entry */
        I_entry_init(&entry, "");
        entry.on_enter = (i_callback_f)on_enter;
        I_widget_add(&window.widget, &entry.widget);

        return &window.widget;
}

