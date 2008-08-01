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

/* Network protocol used by the client and server. Increment when no longer
   compatible before releasing a new version of the game.*/
#define G_PROTOCOL 2

/* Invalid island index */
#define G_ISLAND_INVALID 255

/* Message tokens sent by clients */
typedef enum {
        G_CM_NONE,

        /* Messages for changing status */
        G_CM_AFFILIATE,
        G_CM_NAME,
        G_CM_NAME_SHIP,

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
        G_SM_NAME_SHIP,

        /* Communication */
        G_SM_POPUP,
        G_SM_CHAT,

        /* Entity updates */
        G_SM_SPAWN_SHIP,
        G_SM_SHIP_MOVE,
        G_SM_SHIP_CARGO,
        G_SM_SHIP_OWNER,
        G_SM_BUILDING,

        G_SERVER_MESSAGES
} g_server_msg_t;

/* Ship class names */
typedef enum {
        G_SN_SLOOP,
        G_SN_SPIDER,
        G_SN_GALLEON,
        G_SHIP_NAMES,
        G_SN_NONE,
} g_ship_name_t;

/* Building types */
typedef enum {
        G_BN_NONE,
        G_BN_TREE,
        G_BUILDING_NAMES
} g_building_name_t;

/* Name classes */
typedef enum {
        G_NT_SHIP,
        G_NT_ISLAND,
        G_NAME_TYPES
} g_name_type_t;

/* A tile on the globe */
typedef struct g_tile {
        g_building_name_t building;
        r_model_t model;
        c_vec3_t origin, forward;
        float progress, fade;
        int island, ship, search_parent, search_stamp;
        bool model_shown, visible;
} g_tile_t;

/* Structure for each player */
typedef struct g_client {
        int nation;
        char name[G_NAME_MAX];
} g_client_t;

/* Structure containing ship class information */
typedef struct g_ship_class {
        const char *model_path, *name;
        float speed;
        int health, cargo;
} g_ship_class_t;

/* Structure containing ship information */
typedef struct g_ship {
        g_ship_name_t class_name;
        g_cargo_t cargo;
        float progress;
        int tile, rear_tile, target, client, health, armor;
        char path[R_PATH_MAX], name[G_NAME_MAX];
        bool in_use;
} g_ship_t;

/* Building structure */
typedef struct g_building_type {
        const char *name, *model_path;
} g_building_type_t;

/* g_client.c */
void G_client_callback(int client, n_event_t);
void G_hover_tile(int tile);
i_color_t G_nation_to_color(g_nation_name_t);

extern g_client_t g_clients[N_CLIENTS_MAX + 1];
extern int g_hover_tile, g_hover_ship, g_selected_ship;

/* g_elements.c */
void G_build(int tile, g_building_name_t, float progress);
void G_init_elements(void);
void G_reset_elements(void);

extern g_building_type_t g_building_types[G_BUILDING_NAMES];

/* g_globe.c */
void G_cleanup_globe(void);
void G_init_globe(void);
bool G_is_visible(c_vec3_t origin);
void G_generate_globe(int subdiv4, int islands, int island_size,
                      float variance);
int G_set_tile_model(int tile, const char *path);

extern g_tile_t g_tiles[R_TILES_MAX];

/* g_names.c */
void G_count_name(g_name_type_t, const char *name);
void G_get_name(g_name_type_t, char *buffer, int buffer_size);
#define G_get_name_buf(t, b) G_get_name(t, b, sizeof (b))
void G_load_names(void);
void G_reset_name_counts(void);

/* g_ship.c */
bool G_open_tile(int tile, int exclude_ship);
void G_render_ships(void);
bool G_ship_move_to(int ship, int new_tile);
void G_ship_path(int ship, int tile);
void G_ship_reselect(int ship, n_client_id_t);
void G_ship_select(int ship);
int G_spawn_ship(int client, int tile, g_ship_name_t, int ship);
void G_update_ships(void);

extern g_ship_class_t g_ship_classes[G_SHIP_NAMES];
extern g_ship_t g_ships[G_SHIPS_MAX];

/* g_variables.c */
extern c_var_t g_forest, g_globe_seed, g_globe_subdiv4, g_islands,
               g_island_size, g_island_variance, g_name,
               g_nation_colors[G_NATION_NAMES], g_players,
               g_test_globe, g_test_tile;

