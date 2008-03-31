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

        /* Ensure scroll is in range */
        if (sb->scroll > height - sb->widget.size.y)
                sb->scroll = height - sb->widget.size.y;
        if (sb->scroll < 0.f)
                sb->scroll = 0.f;

        /* Re-align the widgets */
        child = sb->widget.child;
        origin = sb->widget.origin;
        origin.y -= height - sb->scroll - sb->widget.size.y;
        while (child) {
                I_widget_move(child, origin);
                origin.y += child->size.y;
                child = child->next;
        }
}

/******************************************************************************\
 Scroll scrollback widget.
\******************************************************************************/
void I_scrollback_scroll(i_scrollback_t *sb, int up)
{
        sb->scroll += up ? I_WHEEL_SCROLL : -I_WHEEL_SCROLL;
        scrollback_moved(sb);
}

/******************************************************************************\
 Scrollback widget event function.
\******************************************************************************/
int I_scrollback_event(i_scrollback_t *sb, i_event_t event)
{
        switch (event) {
        case I_EV_ADD_CHILD:
                if (sb->children >= sb->limit)
                        I_widget_remove(sb->widget.child);
                else
                        sb->children++;
                i_child->size = sb->widget.size;
                I_widget_event(i_child, I_EV_CONFIGURE);
                scrollback_moved(sb);
                return FALSE;
        case I_EV_CONFIGURE:
                R_window_cleanup(&sb->window);
                R_window_init(&sb->window, i_work_area.value.s);
                if (!sb->widget.size.y)
                        sb->widget.size.y = (float)
                                            R_font_line_skip(R_FONT_CONSOLE);
                sb->window.sprite.size = sb->widget.size;
                sb->window.sprite.origin = sb->widget.origin;
                I_widget_pack(&sb->widget, I_PACK_V, I_FIT_NONE);
                scrollback_moved(sb);
                return FALSE;
        case I_EV_MOUSE_DOWN:
                if (i_mouse == SDL_BUTTON_WHEELUP)
                        I_scrollback_scroll(sb, TRUE);
                else if (i_mouse == SDL_BUTTON_WHEELDOWN)
                        I_scrollback_scroll(sb, FALSE);
                break;
        case I_EV_MOVED:
                sb->window.sprite.origin = sb->widget.origin;
                scrollback_moved(sb);
                break;
        case I_EV_CLEANUP:
                R_window_cleanup(&sb->window);
                break;
        case I_EV_RENDER:
                sb->window.sprite.modulate.a = sb->widget.fade;
                R_window_render(&sb->window);
                R_clip_rect(sb->widget.origin, sb->widget.size);
                R_clip_push();
                I_widget_propagate(&sb->widget, event);
                R_clip_pop();
                R_clip_disable();
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
        sb->widget.expand = TRUE;
        sb->limit = 100;
        sb->scroll = 0.f;
        I_widget_inited(&sb->widget);
        return (i_widget_t *)sb;
}

/******************************************************************************\
 Print text to the console. [string] does not need to persist after the
 function call.
\******************************************************************************/
static void I_console_print(i_color_t color, const char *string)
{
        static int locked;
        i_label_t *label;

        if (locked)
                return;
        locked = TRUE;
        label = I_label_new(string);
        label->font = R_FONT_CONSOLE;
        label->color = color;
        I_widget_add(&scrollback.widget, &label->widget);
        I_widget_show(&label->widget, TRUE);
        locked = FALSE;
}

/******************************************************************************\
 Callback for pressing enter on the console entry field.
\******************************************************************************/
static void on_enter(i_entry_t *entry)
{
        if (!entry->buffer[0])
                return;
        I_console_print(I_COLOR_ALT, entry->buffer);
        C_parse_config(entry->buffer);
        I_entry_configure(entry, NULL);
}

/******************************************************************************\
 Catches the scrollback's configure events so we know where to wrap incoming
 text.
\******************************************************************************/
static int scrollback_event(i_scrollback_t *sb, i_event_t event)
{
        if (event == I_EV_CONFIGURE)
                cols_max = (int)(sb->widget.size.x /
                                 R_font_width(R_FONT_CONSOLE) *
                                 r_pixel_scale.value.f);
        return I_scrollback_event(sb, event);
}

/******************************************************************************\
 Catches the entry widget's events to enable PgUp/PgDn scrolling the
 scrollback.
\******************************************************************************/
static int entry_event(i_entry_t *entry, i_event_t event)
{
        if (scrollback.widget.configured && event == I_EV_KEY_DOWN) {
                if (i_key == SDLK_PAGEUP)
                        I_scrollback_scroll(&scrollback, TRUE);
                else if (i_key == SDLK_PAGEDOWN)
                        I_scrollback_scroll(&scrollback, FALSE);
        }
        return I_entry_event(entry, event);
}

/******************************************************************************\
 Called whenever something is printed to the log.
\******************************************************************************/
static void log_handler(c_log_level_t level, int margin, const char *string)
{
        char *wrapped;
        int len;

        if (level >= C_LOG_TRACE ||
            !scrollback.widget.configured)
                return;

        /* Wrap the text but take off the trailing newline */
        wrapped = C_wrap_log(string, margin, cols_max, &len);
        if (wrapped[len - 1] == '\n')
                wrapped[len - 1] = NUL;
        I_console_print(I_COLOR, wrapped);
}

/******************************************************************************\
 Initializes console window widgets on the given window.
\******************************************************************************/
void I_console_init(i_window_t *window)
{
        /* Set log handler */
        c_log_func = log_handler;

        /* Label */
        I_label_init(&label, "Console:");
        I_widget_add(&window->widget, &label.widget);

        /* Scrollback area */
        I_scrollback_init(&scrollback);
        scrollback.widget.event_func = (i_event_f)scrollback_event;
        scrollback.widget.padding = 0.5f;
        I_widget_add(&window->widget, &scrollback.widget);

        /* Entry */
        I_entry_init(&entry, "");
        entry.widget.event_func = (i_event_f)entry_event;
        entry.on_enter = (i_callback_f)on_enter;
        I_widget_add(&window->widget, &entry.widget);
}

