/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "g_common.h"

/* Maximum health value */
#define HEALTH_MAX 100

/* Ships and ship classes */
g_ship_t g_ships[G_SHIPS_MAX];
g_ship_class_t g_ship_classes[G_SHIP_NAMES];

/******************************************************************************\
 Initialize ship assets.
\******************************************************************************/
void G_init_ships(void)
{
        g_ship_class_t *pc;
        int i;

        /* Setup ships array */
        for (i = 0; i < G_SHIPS_MAX; i++)
                g_ships[i].class_name = G_SN_NONE;

        /* Sloop */
        pc = g_ship_classes + G_SN_SLOOP;
        pc->name = C_str("g-ship-sloop", "Sloop");
        pc->model_path = "models/ship/sloop.plum";
        pc->speed = 1.f;
        pc->health = 100;
}

/******************************************************************************\
 Clean up ship assets.
\******************************************************************************/
void G_cleanup_ships(void)
{
}

/******************************************************************************\
 Returns TRUE if a ship can sail into the given tile.
\******************************************************************************/
static bool open_tile(int tile)
{
        return !g_tiles[tile].ship &&
               (g_tiles[tile].render->terrain == R_T_SHALLOW ||
                g_tiles[tile].render->terrain == R_T_WATER);
}

/******************************************************************************\
 Find an available tile around [tile] (including [tile]) and spawn a new ship
 of the given class there. If [tile] is negative, the ship will be placed on
 a random open tile. If [index] is negative, the first available slot will be
 used.
\******************************************************************************/
g_ship_t *G_spawn_ship(int client, int tile, g_ship_name_t name, int index)
{
        if (index >= G_SHIPS_MAX) {
                C_warning("Failed to spawn ship at tile %d, "
                          "index out of range (%d)", tile, index);
                return NULL;
        }

        /* Find an available ship slot if [index] is not given */
        if (index < 0)
                for (index = 0; g_ships[index].in_use; index++)
                        if (index >= G_SHIPS_MAX) {
                                C_warning("Failed to spawn ship at tile %d, "
                                          "array full", tile);
                                return NULL;
                        }

        /* If we are to spawn the ship anywhere, pick a random tile */
        if (tile < 0) {
                int i;

                /* FIXME: This isn't a great way to pick a random open tile and
                          it doesn't guarantee success */
                tile = C_rand() % r_tiles;
                for (i = 0; !open_tile(tile); i++) {
                        if (i >= 100)
                                return NULL;
                        tile = C_rand() % r_tiles;
                }
        }

        /* Find an available tile to start the ship on */
        else if (!open_tile(tile)) {
                int i, neighbors[12], len;

                /* Not being able to fit a ship in is common, so don't complain
                   if this happens */
                len = R_get_tile_region(tile, neighbors);
                for (i = 0; !open_tile(neighbors[i]); i++)
                        if (i >= len)
                                return NULL;
                tile = neighbors[i];
        }

        /* Initialize ship structure */
        C_zero(g_ships + index);
        g_ships[index].in_use = TRUE;
        g_ships[index].class_name = name;
        g_ships[index].tile = tile;
        g_ships[index].path[0] = -1;
        g_ships[index].client = client;
        g_ships[index].health = g_ship_classes[name].health;
        g_ships[index].armor = 0;

        /* Place the ship on the tile */
        g_tiles[tile].ship = g_ships + index;
        G_set_tile_model(tile, g_ship_classes[name].model_path);

        /* TODO: If we are the server, tell other clients */

        return g_ships + index;
}

/******************************************************************************\
 Render ship 2D effects.
\******************************************************************************/
void G_render_ships(void)
{
        const g_ship_class_t *ship_class;
        const g_ship_t *ship;
        const g_tile_t *tile;
        c_color_t color;
        float armor, health, health_max;
        int i;

        for (i = 0; i < G_SHIPS_MAX; i++) {
                ship = g_ships + i;
                if (!ship->in_use)
                        continue;
                tile = g_tiles + ship->tile;
                if (tile->visible != G_VISIBLE_NEAR)
                        return;
                ship_class = g_ship_classes + ship->class_name;
                armor = (float)ship->armor / HEALTH_MAX;
                health = (float)ship_class->health / HEALTH_MAX;
                health_max = (float)ship_class->health / HEALTH_MAX;
                color = g_nations[g_clients[ship->client].nation].color;
                R_render_ship_status(&tile->model, armor, health_max,
                                     health, health_max, color);
        }
}

