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
        I_EV_RENDER,            /* Should render */
        I_EV_CLEANUP,           /* Should free resources */
        I_EV_CONFIGURE,         /* Initialized, moved, or resized */
        I_EV_KEY_DOWN,          /* In focus and a key is pressed down */
        I_EV_KEY_UP,            /* In focus and a key is released */
        I_EV_MOUSE_IN,          /* Mouse just entered widget area */
        I_EV_MOUSE_OUT,         /* Mouse just left widget area */
        I_EV_MOUSE_MOVE,        /* Mouse is moved over widget */
        I_EV_MOUSE_DOWN,        /* A mouse button is pressed on the widget */
        I_EV_MOUSE_UP,          /* A mouse button is released on the widget */
        I_EVENTS
} i_event_t;

/* Function to handle mouse, keyboard, or other events */
typedef void (*i_event_f)(void *widget, i_event_t);

/* The basic properties all interface widgets need to have. This structure
   should be the first member of any derived interface widget. */
typedef struct i_widget {
        c_vec2_t origin, size, req_size;
        struct i_widget *parent, *next, *child;
        const char *class_name;
        i_event_f event_func;
        int configured;
        char heap, clickable, focusable, hover;
} i_widget_t;

/* Toolbars are horizontally packed containers */
typedef struct i_toolbar {
        i_widget_t widget;
        r_window_t window;
} i_toolbar_t;

/* i_variables.c */
extern c_var_t i_border, i_window_bg;

/* i_widget.c */
void I_toolbar_init(i_toolbar_t *toolbar);
void I_widget_add(i_widget_t *parent, i_widget_t *child);
#define I_widget_init(w) C_zero(w)
void I_widget_event(i_widget_t *, i_event_t);

extern SDLKey i_key;
extern int i_mouse_x, i_mouse_y, i_mouse, i_key_shift;

