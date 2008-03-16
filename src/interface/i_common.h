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

/* Possible events. The arguments (mouse position etc) for the events are
   stored in global variables, so the interface system only processes one
   event at a time. */
typedef enum {
        I_EV_NONE,              /* Do nothing */
        I_EV_CLEANUP,           /* Should free resources */
        I_EV_CONFIGURE,         /* Initialized or resized */
        I_EV_KEY_DOWN,          /* A key is pressed down or repeated */
        I_EV_KEY_UP,            /* A key is released */
        I_EV_MOUSE_IN,          /* Mouse just entered widget area */
        I_EV_MOUSE_OUT,         /* Mouse just left widget area */
        I_EV_MOUSE_MOVE,        /* Mouse is moved over widget */
        I_EV_MOUSE_DOWN,        /* A mouse button is pressed on the widget */
        I_EV_MOUSE_UP,          /* A mouse button is released on the widget */
        I_EV_MOVED,             /* Widget was moved */
        I_EV_RENDER,            /* Should render */
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
        I_WS_HIDDEN,
        I_WS_READY,
        I_WS_HOVER,
        I_WS_ACTIVE,
        I_WIDGET_STATES
} i_widget_state_t;

/* Determines how a widget will use any extra space it has been assigned. The
   window can leave the space as-is, collapse toward its origin, or away from
   its origin (invert). */
typedef enum {
        I_COLLAPSE_NONE,
        I_COLLAPSE,
        I_COLLAPSE_INVERT,
} i_collapse_t;

/* Function to handle mouse, keyboard, or other events. Return FALSE to
   prevent the automatic propagation of an event to the widget's children. */
typedef int (*i_event_f)(void *widget, i_event_t);

/* Simple callback for handling widget-specific events */
typedef void (*i_callback_f)(void *widget);

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
        i_pack_t pack;
        i_widget_state_t state;
        float fade;
        int configured, entry, heap;
} i_widget_t;

/* Windows are decorated containers */
typedef struct i_window {
        i_widget_t widget;
        r_window_t window;
        i_pack_t pack_children;
        i_collapse_t collapse;
        int decorated;
} i_window_t;

/* Buttons can have an icon and/or text */
typedef struct i_button {
        i_widget_t widget;
        r_window_t normal, hover, active;
        r_sprite_t icon, text;
        i_callback_f on_click;
        int decorated;
        char buffer[64];
} i_button_t;

/* Labels only have text */
typedef struct i_label {
        i_widget_t widget;
        r_sprite_t text;
        char buffer[256];
} i_label_t;

/* A one-line text field */
typedef struct i_entry {
        i_widget_t widget;
        r_sprite_t text, cursor;
        r_window_t window;
        float scroll;
        int pos;
        char buffer[256];
} i_entry_t;

/* i_variables.c */
extern c_var_t i_border, i_color, i_debug, i_fade, i_shadow, i_theme,
               i_window, i_work_area;

/* i_widgets.c */
void I_button_init(i_button_t *, const char *icon, const char *text, int bg);
void I_entry_init(i_entry_t *, const char *text);
void I_label_init(i_label_t *, const char *text);
const char *I_event_string(i_event_t event);
void I_widget_add(i_widget_t *parent, i_widget_t *child);
#define I_widget_cleanup(w) I_widget_event(w, I_EV_CLEANUP)
void I_widget_event(i_widget_t *, i_event_t);
void I_widget_inited(const i_widget_t *);
void I_widget_pack(i_widget_t *, i_pack_t, i_collapse_t);
void I_widget_propagate(i_widget_t *, i_event_t);
void I_widget_set_name(i_widget_t *, const char *class_name);
void I_window_init(i_window_t *);

extern i_widget_t *i_key_focus;
extern int i_key, i_key_shift, i_key_unicode, i_mouse_x, i_mouse_y, i_mouse;

