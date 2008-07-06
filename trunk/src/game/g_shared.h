/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Longest length of a path a ship can take */
#define G_PATH_LEN 128

/* Maximum number of ships in a game */
#define G_SHIPS_MAX 128

/* Ship class names */
typedef enum {
        G_SN_SLOOP,
        G_SHIP_NAMES,
        G_SN_NONE,
} g_ship_names_t;

/* There is a fixed number of nations */
typedef enum {
        G_NN_RED,
        G_NN_GREEN,
        G_NN_BLUE,
        G_NN_PIRATE,
        G_NATION_NAMES,
        G_NN_NONE,
} g_nation_name_t;

/* Structure for each nation */
typedef struct g_nation {
        const char *short_name, *long_name;
} g_nation_t;

/* Structure containing ship class information */
typedef struct g_ship_class {
        const char *model_path, *name;
        float speed;
} g_ship_class_t;

/* Structure containing ship information */
typedef struct g_ship {
        short tile;
        char ship_class, path[G_PATH_LEN];
} g_ship_t;

/* g_globe.c */
void G_mouse_ray(c_vec3_t origin, c_vec3_t forward);
void G_mouse_ray_miss(void);
void G_process_click(int button);
void G_render_globe(void);

/* g_host.c */
void G_change_nation(int index);
void G_cleanup(void);
void G_host_game(void);
void G_init(void);
void G_leave_game(void);
void G_update(void);

extern g_nation_t g_nations[G_NATION_NAMES];
extern g_ship_class_t g_ship_classes[G_SHIP_NAMES];
extern g_ship_t g_ships[G_SHIPS_MAX];

/* g_variables.c */
void G_register_variables(void);

