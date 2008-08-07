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

/* The interface uses a limited number of colors */
typedef enum {
        I_COLOR,
        I_COLOR_ALT,
        I_COLOR_RED,
        I_COLOR_GREEN,
        I_COLOR_BLUE,
        I_COLOR_PIRATE,
        I_COLORS
} i_color_t;

/* Ring icon names */
typedef enum {
        I_RI_TEST_BLANK,
        I_RI_TEST_MILL,
        I_RI_TEST_TREE,
        I_RI_TEST_DISABLED,
        I_RI_TEST_SHIP,
        I_RING_ICONS,
} i_ring_icon_t;

/* Ring callback function */
typedef void (*i_ring_f)(i_ring_icon_t);

/* Structure for passing information to trade window */
typedef struct i_cargo {
        int amount, buy_price, maximum, minimum, sell_price;
        bool auto_buy, auto_sell;
} i_cargo_t;

/* i_chat.c */
void I_print_chat(const char *name, i_color_t, const char *message);

/* i_layout.c */
void I_cleanup(void);
void I_dispatch(const SDL_Event *);
void I_enter_limbo(void);
void I_init(void);
void I_leave_limbo(void);
void I_parse_config(void);
void I_render(void);
void I_update_colors(void);

extern int i_limbo;

/* i_nations.c */
void I_configure_nation(int nation, const char *short_name,
                        const char *long_name);
void I_init_nation(int i, const char *short_name, const char *long_name);
void I_select_nation(int nation);

/* i_players.c */
void I_configure_player(int index, const char *name, i_color_t, bool host);
void I_configure_player_num(int num);

/* i_ring.c */
void I_reset_ring(void);
void I_add_to_ring(i_ring_icon_t, int enabled);
void I_show_ring(i_ring_f callback);

/* i_ship.c */
void I_deselect_ship(void);
void I_select_ship(i_color_t, const char *name,
                   const char *owner, const char *class_name);

/* i_trade.c */
void I_configure_cargo(int index, const i_cargo_t *left,
                       const i_cargo_t *right);
void I_disable_trade(void);
void I_enable_trade(bool left_own, bool right_own, const char *right_name);
void I_set_cargo_space(int used, int capacity);

/* i_variables.c */
void I_register_variables(void);

/* i_window.c */
void I_popup(c_vec3_t *goto_pos, const char *message);

