/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Maximum number of icon buttons a ring can hold */
#define I_RING_BUTTONS 6

/* Ring icon names */
typedef enum {
        I_RI_TEST_BLANK,
        I_RI_TEST_MILL,
        I_RI_TEST_TREE,
        I_RI_TEST_DISABLED,
        I_RI_TEST_SHIP,
        I_RING_ICONS,
} i_ring_icon_t;

/* Popup icons */
typedef enum {
        I_PI_NONE,
} i_popup_icon_t;

/* Ring callback function */
typedef void (*i_ring_f)(i_ring_icon_t);

/* i_layout.c */
void I_cleanup(void);
void I_dispatch(const SDL_Event *);
void I_enter_limbo(void);
void I_init(void);
void I_leave_limbo(void);
void I_parse_config(void);
void I_render(void);

/* i_nations.c */
void I_add_nation(const char *short_name, const char *long_name, bool margin);
void I_configure_nations(void);
void I_reset_nations(void);
void I_select_nation(int nation);

/* i_ring.c */
void I_reset_ring(void);
void I_add_to_ring(i_ring_icon_t, int enabled);
void I_show_ring(i_ring_f callback);

/* i_variables.c */
void I_register_variables(void);

/* i_window.c */
void I_popup(i_popup_icon_t, const char *message, c_vec3_t *goto_pos);

