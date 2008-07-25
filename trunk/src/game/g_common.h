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
#include "../interface/i_shared.h"
#include "../network/n_shared.h"
#include "g_shared.h"

/* Network protocol, increment when no longer compatible */
#define G_PROTOCOL 1

/* Invalid island index */
#define G_ISLAND_INVALID 255

/* Longest allowed name */
#define G_NAME_MAX 16

/* Message tokens sent by clients */
typedef enum {
        G_CM_NONE,

        /* Messages for changing status */
        G_CM_AFFILIATE,
        G_CM_NAME,

        /* Communication */
        G_CM_CHAT,

        /* Commands */
        G_CM_SHIP_MOVE,

        G_CLIENT_MESSAGES
} g_client_msg_t;

/* Message tokens sent by the server */
typedef enum {
        G_SM_NONE,

        /* Synchronization messages */
        G_SM_INIT,
        G_SM_CLIENT,

        /* Messages for when clients change status */
        G_SM_CONNECTED,
        G_SM_DISCONNECTED,
        G_SM_AFFILIATE,
        G_SM_NAME,

        /* Communication */
        G_SM_POPUP,
        G_SM_CHAT,

        /* Entity updates */
        G_SM_SPAWN_SHIP,
        G_SM_SHIP_MOVE,

        G_SERVER_MESSAGES
} g_server_msg_t;

/* A tile on the globe */
typedef struct g_tile {
        r_model_t model;
        c_vec3_t origin, forward;
        int island, ship, search_parent, search_stamp;
        bool visible, model_visible;
} g_tile_t;

/* Structure for each player */
typedef struct g_client {
        c_count_t commands;
        int nation;
        char name[G_NAME_MAX];
} g_client_t;

/* g_client.c */
void G_client_callback(int client, n_event_t);
i_color_t G_nation_to_color(g_nation_name_t);
void G_select_tile(int tile);

extern g_client_t g_clients[N_CLIENTS_MAX];
extern int g_selected_ship, g_selected_tile;

/* g_globe.c */
void G_cleanup_globe(void);
void G_init_globe(void);
bool G_is_visible(c_vec3_t origin);
void G_generate_globe(void);
int G_set_tile_model(int tile, const char *path);

extern g_tile_t g_tiles[R_TILES_MAX];

/* g_ship.c */
void G_cleanup_ships(void);
void G_init_ships(void);
bool G_open_tile(int tile, int exclude_ship);
void G_render_ships(void);
bool G_ship_move_to(int index, int new_tile);
void G_ship_path(int ship, int tile);
int G_spawn_ship(int client, int tile, g_ship_name_t, int ship);
void G_update_ships(void);

/* g_variables.c */
extern c_var_t g_globe_islands, g_globe_island_size, g_globe_seed,
               g_globe_subdiv4, g_name, g_nation_colors[G_NATION_NAMES],
               g_test_globe, g_test_tile;

