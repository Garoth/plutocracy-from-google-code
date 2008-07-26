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
#include "../game/g_shared.h"
#include "i_shared.h"

/* Pixels to scroll for mouse wheel scrolling */
#define I_WHEEL_SCROLL 32.f

/* Size of the history queue kept by the text entry widget */
#define I_ENTRY_HISTORY 32

/* Limit for toolbar buttons */
#define I_TOOLBAR_BUTTONS 6

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
        I_EV_KEY_FOCUS,         /* This widget is the new keyboard focus */
        I_EV_MOUSE_IN,          /* Mouse just entered widget area */
        I_EV_MOUSE_OUT,         /* Mouse just left widget area */
        I_EV_MOUSE_MOVE,        /* Mouse is moved over widget */
        I_EV_MOUSE_DOWN,        /* A mouse button is pressed on the widget */
        I_EV_MOUSE_UP,          /* A mouse button is released on the widget */
        I_EV_MOUSE_FOCUS,       /* Event used to find which widget has focus */
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
        I_WS_READY,
        I_WS_HOVER,
        I_WS_ACTIVE,
        I_WS_DISABLED,
        I_WS_NO_FOCUS,
        I_WIDGET_STATES
} i_widget_state_t;

/* Determines how a widget will use any extra space it has been assigned */
typedef enum {
        I_FIT_NONE,
        I_FIT_TOP,
        I_FIT_BOTTOM,
} i_fit_t;

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
        bool configured, entry, expand, shown, heap;
} i_widget_t;

/* Windows are decorated containers */
typedef struct i_window {
        i_widget_t widget;
        i_pack_t pack_children;
        i_fit_t fit;
        i_widget_t *key_focus, *hanger_align;
        r_window_t window;
        r_sprite_t hanger;
        c_vec2_t natural_size;
        float hanger_x;
        bool auto_hide, decorated, hanger_shown, popup;
} i_window_t;

/* Button type */
typedef enum {
        I_BT_DECORATED,
        I_BT_SQUARE,
        I_BT_ROUND,
} i_button_type_t;

/* Buttons can have an icon and/or text */
typedef struct i_button {
        i_widget_t widget;
        r_window_t normal, hover, active;
        r_sprite_t icon, text, icon_active, icon_hover;
        i_callback_f on_click;
        i_button_type_t type;
        void *data;
        char buffer[64];
        bool hover_activate;
} i_button_t;

/* Labels only have text */
typedef struct i_label {
        i_widget_t widget;
        r_sprite_t text;
        r_font_t font;
        i_color_t color;
        float width;
        char buffer[256];
        bool center;
} i_label_t;

/* A one-line text field */
typedef struct i_entry {
        i_widget_t widget;
        r_sprite_t text, cursor;
        r_window_t window;
        i_callback_f on_change, on_enter;
        i_auto_complete_f auto_complete;
        float scroll;
        int pos, history_pos, history_size;
        char buffer[256], history[I_ENTRY_HISTORY][256];
        bool just_tabbed;
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
        bool reverse;
} i_select_t;

/* Toolbar widget used for the left and right toolbars on the screen */
typedef struct i_toolbar {
        i_widget_t widget;
        i_window_t window, windows[I_TOOLBAR_BUTTONS], *open_window;
        i_button_t buttons[I_TOOLBAR_BUTTONS];
        bool right, children, was_open[I_TOOLBAR_BUTTONS];
} i_toolbar_t;

/* Box widget used for aligning other widgets */
typedef struct i_box {
        i_widget_t widget;
        i_fit_t fit;
        i_pack_t pack_children;
        float width;
} i_box_t;

/* A left-aligned and a right-aligned label packed into a horizontal box */
typedef struct i_info {
        i_widget_t widget;
        i_label_t left, right;
} i_info_t;

/* Images just display a sprite */
typedef struct i_image {
        i_widget_t widget;
        r_sprite_t sprite;
        r_texture_t **theme_texture;
        c_vec2_t original_size;
        bool resize;
} i_image_t;

/* i_button.c */
int I_button_event(i_button_t *, i_event_t);
void I_button_init(i_button_t *, const char *icon, const char *text,
                   i_button_type_t);
void I_button_configure(i_button_t *, const char *icon, const char *text,
                        i_button_type_t);
void I_theme_buttons(void);

/* i_chat.c */
void I_focus_chat(void);
void I_hide_chat(void);
void I_init_chat(void);
void I_position_chat(void);
void I_show_chat(void);

/* i_console.c */
void I_init_console(i_window_t *);
void I_scrollback_init(i_scrollback_t *);
void I_theme_scrollbacks(void);

/* i_entry.c */
int I_entry_event(i_entry_t *, i_event_t);
void I_entry_init(i_entry_t *, const char *);
void I_entry_configure(i_entry_t *, const char *);
void I_theme_entries(void);

/* i_game.c */
void I_init_game(i_window_t *);

/* i_globe.c */
void I_globe_event(i_event_t);

/* i_layout.c */
void I_theme_texture(r_texture_t **, const char *name);

extern i_toolbar_t i_right_toolbar;
extern i_widget_t i_root;
extern int i_ship_button;

/* i_nations.c */
void I_init_nations(i_window_t *);

/* i_ring.c */
void I_close_ring(void);
void I_init_ring(void);
int I_ring_shown(void);
void I_theme_ring(void);

/* i_select.c */
void I_select_init(i_select_t *, const char *label, const char **list,
                   int initial, bool reverse);

/* i_ship.c */
void I_init_ship(i_window_t *);

/* i_static.c */
void I_box_init(i_box_t *, i_pack_t, float width);
void I_info_init(i_info_t *, const char *label, const char *info);
void I_info_configure(i_info_t *, const char *);
void I_label_init(i_label_t *, const char *);
void I_label_configure(i_label_t *, const char *);
i_label_t *I_label_new(const char *);
void I_image_init(i_image_t *, const char *icon);
#define I_image_init_sep(i) I_image_init_themed(i, NULL)
void I_image_init_themed(i_image_t *, r_texture_t **);
void I_theme_statics(void);

/* i_variables.c */
extern c_var_t i_border, i_color, i_color_alt, i_debug, i_fade, i_scroll_speed,
               i_shadow, i_test_globe, i_theme, i_zoom_speed;

/* i_video.c */
void I_init_video(i_window_t *);

/* i_widgets.c */
const char *I_event_string(i_event_t);
void I_widget_add(i_widget_t *parent, i_widget_t *child);
c_vec2_t I_widget_bounds(const i_widget_t *, i_pack_t);
c_vec2_t I_widget_child_bounds(const i_widget_t *);
bool I_widget_child_of(const i_widget_t *parent, const i_widget_t *child);
void I_widget_event(i_widget_t *, i_event_t);
void I_widget_init(i_widget_t *, const char *class_name);
void I_widget_move(i_widget_t *, c_vec2_t new_origin);
void I_widget_pack(i_widget_t *, i_pack_t, i_fit_t);
void I_widget_propagate(i_widget_t *, i_event_t);
void I_widget_remove(i_widget_t *, int cleanup);
void I_widget_remove_children(i_widget_t *, int cleanup);
i_widget_t *I_widget_top_level(i_widget_t *);

extern c_color_t i_colors[I_COLORS];
extern i_widget_t *i_child, *i_key_focus, *i_mouse_focus;
extern int i_key, i_key_shift, i_key_unicode, i_mouse_x, i_mouse_y, i_mouse;

/* i_window.c */
void I_init_popup(void);
void I_update_popup(void);
void I_theme_windows(void);
int I_toolbar_add_button(i_toolbar_t *, const char *icon,
                         i_callback_f init_func);
void I_toolbar_enable(i_toolbar_t *, int button, bool enable);
void I_toolbar_init(i_toolbar_t *, int right);
void I_window_init(i_window_t *);

