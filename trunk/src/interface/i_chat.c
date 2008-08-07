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
static i_box_t chat_box, input_box;
static i_entry_t input;
static i_window_t input_window;
static i_button_t scrollback_button;
static i_scrollback_t scrollback;

/******************************************************************************\
 Chat widget event function.
\******************************************************************************/
static int chat_event(chat_t *chat, i_event_t event)
{
        switch (event) {
        case I_EV_CONFIGURE:
                I_widget_pack(&chat->widget, I_PACK_H, I_FIT_NONE);
                chat->widget.size = I_widget_child_bounds(&chat->widget);
                return FALSE;
        case I_EV_RENDER:
                if (chat->time >= 0 && c_time_msec - chat->time > CHAT_DURATION)
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
 Allocate a new chat widget. Allocated chats don't vanish.
\******************************************************************************/
static chat_t *chat_alloc(const char *name, i_color_t color, const char *text)
{
        chat_t *chat;

        chat = C_malloc(sizeof (*chat));
        chat_init(chat, name, color, text);
        chat->time = -1;
        chat->widget.heap = TRUE;
        return chat;
}

/******************************************************************************\
 Position the chat windows after a theme change.
\******************************************************************************/
void I_position_chat(void)
{
        float width;

        /* Position window */
        I_widget_move(&input_window.widget,
                      C_vec2((float)i_border.value.n,
                             i_right_toolbar.widget.origin.y -
                             input_window.widget.size.y - i_border.value.n));

        /* Position chat box */
        I_widget_move(&chat_box.widget,
                      C_vec2((float)i_border.value.n,
                             input_window.widget.origin.y -
                             chat_box.widget.size.y));

        /* Position scrollback */
        I_widget_move(&scrollback.widget,
                      C_vec2(input.widget.origin.x,
                             input_window.widget.origin.y -
                             scrollback.widget.size.y -
                             i_border.value.n * chat_box.widget.padding));

        /* Make the chat input window look nicely aligned to scrollback */
        width = scrollback.widget.size.x + input_window.widget.padding *
                                           i_border.value.n * 2.f;
        if (width != input_window.widget.size.x) {
                input_window.widget.size.x = width;
                I_widget_event(&input_window.widget, I_EV_CONFIGURE);
        }
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
 Show or hide the chat scrollback.
\******************************************************************************/
static void show_scrollback(bool show)
{
        I_widget_event(&scrollback.widget, show ? I_EV_SHOW : I_EV_HIDE);
        I_widget_event(&chat_box.widget, show ? I_EV_HIDE : I_EV_SHOW);
}

/******************************************************************************\
 Scrollback button was clicked.
\******************************************************************************/
static void scrollback_button_click(void)
{
        show_scrollback(!scrollback.widget.shown);
}


/******************************************************************************\
 A key event was delivered to the root window that may be relevant to chat.
\******************************************************************************/
void I_chat_event(i_event_t event)
{
        if (event != I_EV_KEY_DOWN || i_key_focus != &i_root)
                return;
        if (i_key == SDLK_ESCAPE)
                show_scrollback(FALSE);
        else if (i_key == SDLK_PAGEUP) {
                show_scrollback(TRUE);
                I_scrollback_scroll(&scrollback, TRUE);
        } else if (i_key == SDLK_PAGEDOWN) {
                show_scrollback(TRUE);
                I_scrollback_scroll(&scrollback, FALSE);
        } else if (i_key == SDLK_RETURN)
                I_show_chat();
}

/******************************************************************************\
 Catch events to the entry widget.
\******************************************************************************/
static int entry_event(i_entry_t *entry, i_event_t event)
{
        if (i_key == SDLK_PAGEUP) {
                show_scrollback(TRUE);
                I_scrollback_scroll(&scrollback, TRUE);
        } else if (i_key == SDLK_PAGEDOWN) {
                show_scrollback(TRUE);
                I_scrollback_scroll(&scrollback, FALSE);
        }
        return I_entry_event(entry, event);
}

/******************************************************************************\
 Catch scrollback event to close it when the user wants it gone.
\******************************************************************************/
static int scrollback_event(i_scrollback_t *scrollback, i_event_t event)
{
        if ((event == I_EV_MOUSE_DOWN && i_mouse == SDL_BUTTON_RIGHT) ||
            (event == I_EV_KEY_DOWN && i_key == SDLK_ESCAPE))
                show_scrollback(FALSE);
        return I_scrollback_event(scrollback, event);
}

/******************************************************************************\
 Initialize chat windows.
\******************************************************************************/
void I_init_chat(void)
{
        /* Chat box */
        I_box_init(&chat_box, I_PACK_V, 0.f);
        chat_box.widget.state = I_WS_NO_FOCUS;
        chat_box.widget.padding = 1.f;
        chat_box.widget.size = C_vec2(512.f, 0.f);
        chat_box.fit = I_FIT_BOTTOM;
        I_widget_add(&i_root, &chat_box.widget);

        /* Chat scrollback */
        I_scrollback_init(&scrollback);
        scrollback.widget.event_func = (i_event_f)scrollback_event;
        scrollback.widget.steal_keys = TRUE;
        scrollback.widget.size = C_vec2(512.f, 256.f);
        scrollback.widget.shown = FALSE;
        I_widget_add(&i_root, &scrollback.widget);

        /* Input window */
        I_window_init(&input_window);
        input_window.widget.size = C_vec2(512.f, 0.f);
        input_window.fit = I_FIT_BOTTOM;
        input_window.popup = TRUE;
        input_window.auto_hide = TRUE;
        I_widget_add(&i_root, &input_window.widget);

        /* Input window box */
        I_box_init(&input_box, I_PACK_H, 0.f);
        I_widget_add(&input_window.widget, &input_box.widget);

        /* Input entry widget */
        I_entry_init(&input, "");
        input.widget.event_func = (i_event_f)entry_event;
        input.widget.expand = TRUE;
        input.on_enter = (i_callback_f)input_enter;
        I_widget_add(&input_box.widget, &input.widget);

        /* Scrollback button */
        I_button_init(&scrollback_button, "gui/icons/scrollback.png",
                      NULL, I_BT_ROUND);
        scrollback_button.widget.margin_front = 0.5f;
        scrollback_button.on_click = (i_callback_f)scrollback_button_click;
        I_widget_add(&input_box.widget, &scrollback_button.widget);
}

/******************************************************************************\
 Focuses the chat input entry widget if it is open.
\******************************************************************************/
void I_focus_chat(void)
{
        if (input_window.widget.shown)
                I_widget_focus(&input.widget, TRUE, FALSE);
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
\******************************************************************************/
void I_print_chat(const char *name, i_color_t color, const char *message)
{
        chat_t *chat;
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
        I_widget_event(&chat_box.widget, I_EV_CONFIGURE);
        I_position_chat();

        /* Add a copy to the scrollback */
        chat = chat_alloc(name, color, message);
        I_widget_add(&scrollback.widget, &chat->widget);

        /* Debug log the chat */
        if (!message || !message[0])
                C_debug("%s", name);
        else
                C_debug("%s: %s", name, message);
}

