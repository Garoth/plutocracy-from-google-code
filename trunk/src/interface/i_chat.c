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

/* Duration that chat stays on the screen */
#define CHAT_DURATION 15000

/* Maximum number of chat lines */
#define CHAT_LINES 10

/* Chat line widget */
typedef struct chat {
        i_widget_t widget;
        i_label_t name, text;
        int time;
} chat_t;

static chat_t chat_lines[CHAT_LINES];
static i_box_t chat_box;
static i_entry_t input;
static i_window_t input_window;

/******************************************************************************\
 Chat widget event function.
\******************************************************************************/
static int chat_event(chat_t *chat, i_event_t event)
{
        switch (event) {
        case I_EV_CONFIGURE:
                chat->widget.size.x = 512.f;
                I_widget_pack(&chat->widget, I_PACK_H, I_FIT_NONE);
                chat->widget.size = I_widget_child_bounds(&chat->widget);
                return FALSE;
        case I_EV_RENDER:
                if (c_time_msec - chat->time > CHAT_DURATION)
                        I_widget_event(&chat->widget, I_EV_HIDE);
                break;
        default:
                break;
        }
        return TRUE;
}

/******************************************************************************\
 Initialize a chat widget.
\******************************************************************************/
static void chat_init(chat_t *chat, const char *name, i_color_t color,
                      const char *text)
{
        I_widget_init(&chat->widget, "Chat Line");
        chat->widget.state = I_WS_READY;
        chat->widget.event_func = (i_event_f)chat_event;
        chat->time = c_time_msec;

        /* No text */
        if (!text || !text[0]) {
                I_label_init(&chat->name, name);
                chat->name.color = color;
                I_widget_add(&chat->widget, &chat->name.widget);
                return;
        }

        /* /me action */
        if ((text[0] == '/' || text[0] == '\\') &&
            !strncasecmp(text + 1, "me ", 3)) {
                I_label_init(&chat->name, C_va("*** %s", name));
                text += 4;
        }

        /* Normal colored name */
        else
                I_label_init(&chat->name, C_va("%s:", name));

        chat->name.color = color;
        I_widget_add(&chat->widget, &chat->name.widget);

        /* Message text */
        I_label_init(&chat->text, text);
        chat->text.widget.expand = TRUE;
        I_widget_add(&chat->widget, &chat->text.widget);
}

/******************************************************************************\
 Position the chat windows after a theme change.
\******************************************************************************/
void I_position_chat(void)
{
        /* Position window */
        input_window.widget.origin = C_vec2((float)i_border.value.n,
                                            r_height_2d -
                                            i_border.value.n * 4 - 32.f);

        /* Position and size box */
        chat_box.widget.size = C_vec2(0.f, 0.f);
        chat_box.widget.origin = C_vec2((float)i_border.value.n,
                                        input_window.widget.origin.y -
                                        input_window.widget.size.y -
                                        i_border.value.n);
}

/******************************************************************************\
 Hide the chat input window.
\******************************************************************************/
void I_hide_chat(void)
{
        I_widget_event(&input_window.widget, I_EV_HIDE);
}

/******************************************************************************\
 User pressed enter on the input box.
\******************************************************************************/
static void input_enter(void)
{
        G_input_chat(input.buffer);
        I_hide_chat();
        I_entry_configure(&input, "");
}

/******************************************************************************\
 Initialize chat windows.
\******************************************************************************/
void I_init_chat(void)
{
        /* Chat box */
        I_box_init(&chat_box, I_PACK_V, 0.f);
        chat_box.widget.state = I_WS_NO_FOCUS;
        chat_box.fit = I_FIT_TOP;
        I_widget_add(&i_root, &chat_box.widget);

        /* Input window */
        I_window_init(&input_window);
        input_window.natural_size = C_vec2(400.f, 0.f);
        input_window.fit = I_FIT_TOP;
        input_window.popup = TRUE;
        input_window.auto_hide = TRUE;
        I_widget_add(&i_root, &input_window.widget);

        /* Input entry widget */
        I_entry_init(&input, "");
        input.on_enter = (i_callback_f)input_enter;
        I_widget_add(&input_window.widget, &input.widget);

        I_position_chat();
}

/******************************************************************************\
 Focuses the chat input entry widget if it is open.
\******************************************************************************/
void I_focus_chat(void)
{
        if (input_window.widget.shown)
                i_key_focus = &input.widget;
}

/******************************************************************************\
 Show the chat input window.
\******************************************************************************/
void I_show_chat(void)
{
        if (i_limbo)
                return;
        I_entry_configure(&input, "");
        I_widget_event(&input_window.widget, I_EV_SHOW);
        I_focus_chat();
}

/******************************************************************************\
 Print something in chat.
 FIXME: Do not reconfigure chat lines that didn't change
\******************************************************************************/
void I_print_chat(const char *name, i_color_t color, const char *message)
{
        int i, oldest;

        /* Remove hidden chat lines */
        for (i = 0; i < CHAT_LINES; i++)
                if (chat_lines[i].widget.parent &&
                    !chat_lines[i].widget.shown &&
                    chat_lines[i].widget.fade <= 0.f)
                        I_widget_remove(&chat_lines[i].widget, TRUE);

        /* Find a free chat line */
        for (oldest = i = 0; chat_lines[i].widget.parent; i++) {
                if (chat_lines[i].time < chat_lines[oldest].time)
                        oldest = i;
                if (i >= CHAT_LINES - 1) {
                        i = oldest;
                        I_widget_remove(&chat_lines[i].widget, TRUE);
                        break;
                }
        }

        /* Initialize and add it back */
        chat_init(chat_lines + i, name, color, message);
        I_widget_add(&chat_box.widget, &chat_lines[i].widget);

        /* Reconfigure the chat box */
        I_position_chat();
        I_widget_event(&chat_box.widget, I_EV_CONFIGURE);
}

