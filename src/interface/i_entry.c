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
 Set the entry widget cursor position.
\******************************************************************************/
static void entry_set_pos(i_entry_t *entry, int pos)
{
        size_t len;
        float diff;
        char buf[256];

        /* Make sure position stays in range */
        if (pos < 0)
                pos = 0;
        len = strlen(entry->buffer);
        if (len > sizeof (buf))
                len = sizeof (buf);
        if (pos > len)
                pos = len;

        /* Render the inverted cursor character */
        buf[0] = entry->buffer[pos];
        if (buf[0] < ' ')
                buf[0] = ' ';
        buf[1] = NUL;
        R_sprite_cleanup(&entry->cursor);
        R_sprite_init_text(&entry->cursor, R_FONT_CONSOLE, 0,
                           i_shadow.value.f, TRUE, buf);
        entry->cursor.modulate = i_colors[I_COLOR];
        entry->cursor.origin = entry->text.origin;
        if (pos > 0) {
                c_vec2_t size;

                memcpy(buf, entry->buffer, pos);
                buf[pos] = NUL;
                size = C_vec2_invscalef(R_font_size(R_FONT_CONSOLE, buf),
                                        r_pixel_scale.value.f);
                entry->cursor.origin.x += size.x;
        }

        /* Check scrolling */
        diff = entry->cursor.origin.x + entry->cursor.size.x -
               entry->widget.origin.x - entry->widget.size.x;
        if (diff > 0) {
                entry->scroll += diff;
                entry->cursor.origin.x -= diff;
                entry->text.origin.x -= diff;
        }
        diff = entry->widget.origin.x - entry->cursor.origin.x;
        if (diff > 0) {
                entry->scroll -= diff;
                entry->cursor.origin.x += diff;
                entry->text.origin.x += diff;
        }

        entry->pos = pos;
}

/******************************************************************************\
 Setup an entry widget text and cursor positions after a move.
\******************************************************************************/
static void entry_moved(i_entry_t *entry)
{
        c_vec2_t origin;

        origin = entry->widget.origin;
        entry->window.sprite.origin = origin;
        origin.x -= entry->scroll;
        origin = C_vec2_clamp(origin, r_pixel_scale.value.f);
        entry->text.origin = origin;
        entry_set_pos(entry, entry->pos);
}

/******************************************************************************\
 Delete a character from an entry widget buffer.
\******************************************************************************/
static void entry_delete(i_entry_t *entry, int pos)
{
        size_t len;

        if (pos < 0)
                return;
        len = strlen(entry->buffer);
        if (pos >= len)
                return;
        memmove(entry->buffer + pos, entry->buffer + pos + 1, len - pos - 1);
        entry->buffer[len - 1] = NUL;
        entry->pos = pos;
        R_sprite_cleanup(&entry->text);
        R_sprite_init_text(&entry->text, R_FONT_CONSOLE, 0,
                           i_shadow.value.f, FALSE, entry->buffer);
        entry->text.modulate = i_colors[I_COLOR];
        entry_moved(entry);
}

/******************************************************************************\
 Insert a character into an entry widget buffer.
\******************************************************************************/
static void entry_insert(i_entry_t *entry, char ch)
{
        size_t len;

        len = strlen(entry->buffer);
        if (len >= sizeof (entry->buffer))
                return;
        if (entry->pos < len)
                memmove(entry->buffer + entry->pos + 1,
                        entry->buffer + entry->pos, len - entry->pos);
        entry->buffer[entry->pos++] = ch;
        if (entry->pos > len)
                entry->buffer[entry->pos] = NUL;
        R_sprite_cleanup(&entry->text);
        R_sprite_init_text(&entry->text, R_FONT_CONSOLE, 0,
                           i_shadow.value.f, FALSE, entry->buffer);
        entry->text.modulate = i_colors[I_COLOR];
        entry_moved(entry);
}

/******************************************************************************\
 Sets the entry field text and resets the cursor position. [string] does
 not need to persist after this function is called.
\******************************************************************************/
void I_entry_configure(i_entry_t *entry, const char *string)
{
        C_strncpy_buf(entry->buffer, string);
        entry->pos = C_strlen(string);
        entry->scroll = 0.f;
        I_widget_event(&entry->widget, I_EV_CONFIGURE);
}

/******************************************************************************\
 Cycle through the entry widget's input history.
\******************************************************************************/
static void entry_history_go(i_entry_t *entry, int change)
{
        int new_pos;

        new_pos = entry->history_pos + change;
        if (new_pos < 0)
                new_pos = 0;
        if (new_pos >= entry->history_size)
                new_pos = entry->history_size;
        if (entry->history_pos == new_pos)
                return;
        if (entry->history_pos == 0)
                C_strncpy_buf(entry->history[0], entry->buffer);
        entry->history_pos = new_pos;
        I_entry_configure(entry, entry->history[new_pos]);
}

/******************************************************************************\
 Save current input in a new history entry.
\******************************************************************************/
static void entry_history_save(i_entry_t *entry)
{
        if (entry->history_size) {
                if (entry->history_size > I_ENTRY_HISTORY - 2)
                        entry->history_size = I_ENTRY_HISTORY - 2;
                memmove(entry->history + 2, entry->history + 1,
                        sizeof (entry->history[0]) * entry->history_size);
        }
        C_strncpy_buf(entry->history[1], entry->buffer);
        entry->history_size++;
        entry->history_pos = 0;
}

/******************************************************************************\
 Entry widget event function.
 FIXME: Unicode input
 TODO: Horizontal packing
\******************************************************************************/
int I_entry_event(i_entry_t *entry, i_event_t event)
{
        switch (event) {
        case I_EV_CONFIGURE:

                /* Setup text */
                R_sprite_cleanup(&entry->text);
                R_sprite_init_text(&entry->text, R_FONT_CONSOLE, 0,
                                   i_shadow.value.f, FALSE, entry->buffer);
                entry->widget.size.y = R_font_line_skip(R_FONT_CONSOLE) /
                                       r_pixel_scale.value.f + 1;
                entry->text.modulate = i_colors[I_COLOR];

                /* Setup work area window */
                R_window_cleanup(&entry->window);
                R_window_init(&entry->window, i_work_area.value.s);
                entry->window.sprite.size = entry->widget.size;

        case I_EV_MOVED:
                entry_moved(entry);
                break;
        case I_EV_CLEANUP:
                R_sprite_cleanup(&entry->text);
                R_sprite_cleanup(&entry->cursor);
                R_window_cleanup(&entry->window);
                break;
        case I_EV_KEY_DOWN:
                if (i_key == SDLK_UP)
                        entry_history_go(entry, 1);
                if (i_key == SDLK_DOWN)
                        entry_history_go(entry, -1);
                if (i_key == SDLK_LEFT)
                        entry_set_pos(entry, entry->pos - 1);
                if (i_key == SDLK_RIGHT)
                        entry_set_pos(entry, entry->pos + 1);
                if (i_key == SDLK_HOME)
                        entry_set_pos(entry, 0);
                if (i_key == SDLK_END)
                        entry_set_pos(entry, sizeof (entry->buffer));
                if (i_key == SDLK_BACKSPACE)
                        entry_delete(entry, entry->pos - 1);
                if (i_key == SDLK_DELETE)
                        entry_delete(entry, entry->pos);
                if (i_key == SDLK_RETURN && entry->on_enter) {
                        entry_history_save(entry);
                        entry->on_enter(entry);
                }
                if (i_key >= ' ' && i_key < 0x7f)
                        entry_insert(entry, i_key_unicode);
                break;
        case I_EV_RENDER:
                entry->window.sprite.modulate.a = entry->widget.fade;
                entry->text.modulate.a = entry->widget.fade;
                entry->cursor.modulate.a = entry->widget.fade;
                R_window_render(&entry->window);
                if (i_key_focus == (i_widget_t *)entry) {
                        R_clip_left(entry->cursor.origin.x +
                                    entry->cursor.size.x - 2.f);
                        R_clip_right(entry->widget.origin.x +
                                     entry->widget.size.x);
                        R_sprite_render(&entry->text);
                        R_clip_disable();
                        if (entry->pos > 0) {
                                R_clip_left(entry->widget.origin.x);
                                R_clip_right(entry->cursor.origin.x);
                                R_sprite_render(&entry->text);
                                R_clip_disable();
                        }
                        R_sprite_render(&entry->cursor);
                } else {
                        R_clip_left(entry->widget.origin.x);
                        R_clip_right(entry->widget.origin.x +
                                     entry->widget.size.x);
                        R_sprite_render(&entry->text);
                        R_clip_disable();
                }
                break;
        default:
                break;
        }
        return TRUE;
}

/******************************************************************************\
 Initializes an entry widget.
\******************************************************************************/
void I_entry_init(i_entry_t *entry, const char *text)
{
        if (!entry)
                return;
        C_zero(entry);
        I_widget_set_name(&entry->widget, "Entry");
        entry->widget.event_func = (i_event_f)I_entry_event;
        entry->widget.state = I_WS_READY;
        entry->widget.entry = TRUE;
        C_strncpy_buf(entry->buffer, text);
        I_widget_inited(&entry->widget);
}

