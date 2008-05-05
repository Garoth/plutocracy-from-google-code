/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "../common/c_shared.h"
#include "../render/r_shared.h"
#include "i_shared.h"

/* Pixels to scroll for mouse wheel scrolling */
#define I_WHEEL_SCROLL 32.f

/* Size of the history queue kept by the text entry widget */
#define I_ENTRY_HISTORY 32

/* Possible events. The arguments (mouse position etc) for the events are
   stored in global variables, so the interface system only processes one
   event at a time. */
typedef enum {
        I_EV_NONE,              /* Do nothing */
        I_EV_ADD_CHILD,         /* A child widget was added */
        I_EV_CLEANUP,           /* Should free resources */
        I_EV_CONFIGURE,         /* Initialized or resized */
        I_EV_GRAB_FOCUS,        /* Entry widgets should take key focus */
        I_EV_HIDE,              /* Widget is being hidden */
        I_EV_KEY_DOWN,          /* A key is pressed down or repeated */
        I_EV_KEY_UP,            /* A key is released */
        I_EV_MOUSE_IN,          /* Mouse just entered widget area */
        I_EV_MOUSE_OUT,         /* Mouse just left widget area */
        I_EV_MOUSE_MOVE,        /* Mouse is moved over widget */
        I_EV_MOUSE_DOWN,        /* A mouse button is pressed on the widget */
        I_EV_MOUSE_UP,          /* A mouse button is released on the widget */
        I_EV_MOVED,             /* Widget was moved */
        I_EV_RENDER,            /* Should render */
        I_EV_SHOW,              /* Widget is being shown */
        I_EVENTS
} i_event_t;

/* Widgets can only be packed as lists horizontally or vertically */
typedef enum {
        I_PACK_NONE,
        I_PACK_H,
        I_PACK_V,
        I_PACK_TYPES
} i_pack_t;

/* Widget input states */
typedef enum {
        I_WS_DISABLED,
        I_WS_READY,
        I_WS_HOVER,
        I_WS_ACTIVE,
        I_WIDGET_STATES
} i_widget_state_t;

/* Determines how a widget will use any extra space it has been assigned */
typedef enum {
        I_FIT_NONE,
        I_FIT_TOP,
        I_FIT_BOTTOM,
} i_fit_t;

/* The interface uses a limited number of colors */
typedef enum {
        I_COLOR,
        I_COLOR_ALT,
        I_COLORS
} i_color_t;

/* Function to handle mouse, keyboard, or other events. Return FALSE to
   prevent the automatic propagation of an event to the widget's children. */
typedef int (*i_event_f)(void *widget, i_event_t);

/* Simple callback for handling widget-specific events */
typedef void (*i_callback_f)(void *widget);

/* Entry-widget auto-complete feed function */
typedef const char *(*i_auto_complete_f)(const char *root);

/* The basic properties all interface widgets need to have. The widget struct
   should be the first member of any derived widget struct.

   When a widget receives the I_EV_CONFIGURE event it, its parent has set its
   origin and size to the maximum available space. The widget must then change
   its size to its actual size.

   The reason for keeping [name] as a buffer rather than a pointer to a
   statically-allocated string is to assist in memory leak detection. The
   leak detector thinks the memory is a string and prints the widget name. */
typedef struct i_widget {
        char name[32];
        struct i_widget *parent, *next, *child;
        c_vec2_t origin, size;
        i_event_f event_func;
        i_widget_state_t state;
        float fade, margin_front, margin_rear, padding;
        int configured, entry, clickable, expand, shown, heap;
} i_widget_t;

/* Windows are decorated containers */
typedef struct i_window {
        i_widget_t widget;
        r_window_t window;
        i_pack_t pack_children;
        i_fit_t fit;
        r_sprite_t hanger;
        i_widget_t *key_focus;
        float hanger_x;
        int hanger_shown, decorated;
} i_window_t;

/* Buttons can have an icon and/or text */
typedef struct i_button {
        i_widget_t widget;
        r_window_t normal, hover, active;
        r_sprite_t icon, text, light, prelight;
        i_callback_f on_click;
        void *data;
        int decorated;
        char buffer[64];
} i_button_t;

/* Labels only have text */
typedef struct i_label {
        i_widget_t widget;
        r_sprite_t text;
        r_font_t font;
        i_color_t color;
        float width;
        char buffer[256];
} i_label_t;

/* A one-line text field */
typedef struct i_entry {
        i_widget_t widget;
        r_sprite_t text, cursor;
        r_window_t window;
        i_callback_f on_enter;
        i_auto_complete_f auto_complete;
        float scroll;
        int pos, history_pos, history_size;
        char buffer[256], history[I_ENTRY_HISTORY][256];
} i_entry_t;

/* A fixed-size, work-area widget that can dynamically add new components
   vertically and scroll up and down. Widgets will begin to be removed after
   a specified limit is reached. */
typedef struct i_scrollback {
        i_widget_t widget;
        r_window_t window;
        float scroll;
        int children, limit;
} i_scrollback_t;

/* Displays a label and a selection widget which can select text items from
   a list to the right of it */
typedef struct i_select {
        i_widget_t widget;
        i_label_t label, item;
        i_button_t left, right;
        i_callback_f on_change;
        void *data;
        const char **list;
        int list_len, index;
} i_select_t;

/* i_button.c */
void I_button_init(i_button_t *, const char *icon, const char *text, int bg);

/* i_console.c */
void I_console_init(i_window_t *);

/* i_entry.c */
int I_entry_event(i_entry_t *, i_event_t);
void I_entry_init(i_entry_t *, const char *);
void I_entry_configure(i_entry_t *, const char *);

/* i_game.c */
void I_game_init(i_window_t *);

/* i_label.c */
void I_label_init(i_label_t *, const char *);
void I_label_configure(i_label_t *, const char *);
i_label_t *I_label_new(const char *);

/* i_select.c */
void I_select_init(i_select_t *, const char *label, const char **list,
                   int initial);

/* i_variables.c */
extern c_var_t i_border, i_button, i_button_active, i_button_hover,
               i_button_light, i_button_prelight, i_color, i_color2,
               i_debug, i_fade, i_hanger, i_scroll_speed, i_shadow, i_theme,
               i_window, i_work_area, i_zoom_speed;

/* i_video.c */
void I_video_init(i_window_t *);

/* i_widgets.c */
const char *I_event_string(i_event_t);
void I_widget_add(i_widget_t *parent, i_widget_t *child);
c_vec2_t I_widget_bounds(const i_widget_t *, i_pack_t);
c_vec2_t I_widget_child_bounds(const i_widget_t *);
#define I_widget_cleanup(w) I_widget_event(w, I_EV_CLEANUP)
void I_widget_event(i_widget_t *, i_event_t);
void I_widget_inited(const i_widget_t *);
void I_widget_move(i_widget_t *, c_vec2_t new_origin);
void I_widget_pack(i_widget_t *, i_pack_t, i_fit_t);
void I_widget_propagate(i_widget_t *, i_event_t);
void I_widget_remove(i_widget_t *);
void I_widget_set_name(i_widget_t *, const char *class_name);
void I_widget_show(i_widget_t *, int show);

extern c_color_t i_colors[I_COLORS];
extern i_widget_t *i_key_focus, *i_child;
extern int i_key, i_key_shift, i_key_unicode, i_mouse_x, i_mouse_y, i_mouse;

/* i_window.c */
void I_window_init(i_window_t *);
void I_window_hanger(i_window_t *, i_widget_t *align, int shown);

