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

/* Maximum breadth of a path search */
#define SEARCH_BREADTH (R_PATH_MAX * 3)

/* Proportion of rotation a ship does per second */
#define ROTATION_RATE 4.f

/* Structure for searched tile nodes */
typedef struct search_node {
        float dist;
        int tile, moves;
} search_node_t;

/* Ships and ship classes */
g_ship_t g_ships[G_SHIPS_MAX];
g_ship_class_t g_ship_classes[G_SHIP_NAMES];

/******************************************************************************\
 Returns TRUE if a ship can sail into the given tile.
\******************************************************************************/
bool G_open_tile(int tile, int ship)
{
        return (g_tiles[tile].ship < 0 ||
                (ship >= 0 && g_tiles[tile].ship == ship)) &&
               R_water_terrain(r_tile_params[tile].terrain);
}

/******************************************************************************\
 Find an available tile around [tile] (including [tile]) and spawn a new ship
 of the given class there. If [tile] is negative, the ship will be placed on
 a random open tile. If [index] is negative, the first available slot will be
 used.
\******************************************************************************/
int G_spawn_ship(int client, int tile, g_ship_name_t name, int index)
{
        if (!N_client_valid(client) || tile >= r_tiles ||
            name < 0 || name >= G_SHIP_NAMES) {
                C_warning("Invalid parameters (%d, %d, %d, %d)",
                          client, tile, name, index);
                return -1;
        }
        if (index >= G_SHIPS_MAX) {
                C_warning("Failed to spawn ship at tile %d, "
                          "index out of range (%d)", tile, index);
                return -1;
        }

        /* Find an available ship slot if [index] is not given */
        if (index < 0)
                for (index = 0; g_ships[index].in_use; index++)
                        if (index >= G_SHIPS_MAX) {
                                C_warning("Failed to spawn ship at tile %d, "
                                          "array full", tile);
                                return -1;
                        }

        /* If we are to spawn the ship anywhere, pick a random tile */
        if (tile < 0) {
                int start;

                start = C_rand() % r_tiles;
                for (tile = start + 1; tile < r_tiles; tile++)
                        if (G_open_tile(tile, -1))
                                goto init;
                for (tile = 0; tile <= start; tile++)
                        if (G_open_tile(tile, -1))
                                goto init;

                /* If this happens, the globe is completely full! */
                return -1;
        }

        /* Find an available tile to start the ship on */
        else if (!G_open_tile(tile, -1)) {
                int i, neighbors[12], len;

                /* Not being able to fit a ship in is common, so don't complain
                   if this happens */
                len = R_get_tile_region(tile, neighbors);
                for (i = 0; !G_open_tile(neighbors[i], -1); i++)
                        if (i >= len)
                                return -1;
                tile = neighbors[i];
        }

init:   /* Initialize ship structure */
        C_zero(g_ships + index);
        g_ships[index].in_use = TRUE;
        g_ships[index].class_name = name;
        g_ships[index].tile = g_ships[index].target = tile;
        g_ships[index].rear_tile = -1;
        g_ships[index].progress = 1.f;
        g_ships[index].client = client;
        g_ships[index].health = g_ship_classes[name].health;

        /* Start out unnamed */
        C_strncpy_buf(g_ships[index].name, C_va("Unnamed #%d", index));

        /* Place the ship on the tile */
        G_set_tile_model(tile, g_ship_classes[name].model_path);
        g_tiles[tile].ship = index;
        g_tiles[tile].fade = 0.f;

        /* If we are the server, tell other clients */
        if (n_client_id == N_HOST_CLIENT_ID)
                N_broadcast_except(N_HOST_CLIENT_ID, "11211", G_SM_SPAWN_SHIP,
                                   client, tile, name, index);

        /* If this is one of ours, name it */
        if (client == n_client_id) {
                G_get_name_buf(G_NT_SHIP, g_ships[index].name);
                N_send(N_SERVER_ID, "11s", G_CM_NAME_SHIP, index,
                       g_ships[index].name);
        }

        return index;
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

        if (i_limbo)
                return;
        for (i = 0; i < G_SHIPS_MAX; i++) {
                ship = g_ships + i;
                if (!ship->in_use)
                        continue;
                C_assert(ship->tile >= 0 && ship->tile < r_tiles);
                C_assert(g_tiles[ship->tile].ship == i);
                tile = g_tiles + ship->tile;

                /* If globe testing is on, draw a line from ship tile origins */
                if (g_test_globe.value.n) {
                        c_vec3_t b;

                        b = C_vec3_add(tile->origin, C_vec3_norm(tile->origin));
                        R_render_test_line(tile->origin, b,
                                           C_color(0.f, 1.f, 1.f, 1.f));
                }

                /* Don't bother rendering if the ship isn't visible */
                if (!tile->visible)
                        continue;

                /* Draw the status display */
                ship_class = g_ship_classes + ship->class_name;
                armor = (float)ship->armor / HEALTH_MAX;
                health = (float)ship->health / HEALTH_MAX;
                health_max = (float)ship_class->health / HEALTH_MAX;
                color = g_nations[g_clients[ship->client].nation].color;
                R_render_ship_status(&tile->model, health, health_max,
                                     armor, health_max, color,
                                     g_selected_ship == i,
                                     ship->client == n_client_id);
        }
}

/******************************************************************************\
 Returns the linear distance from one tile to another. This is the search
 function heuristic.
\******************************************************************************/
static float tile_dist(int a, int b)
{
        return C_vec3_len(C_vec3_sub(g_tiles[b].origin, g_tiles[a].origin));
}

/******************************************************************************\
 Scan along a ship's path and find that farthest out open tile. Returns the
 new target tile. Note that any special rules that constrict path-finding
 must be applied here as well or the path-finding function could fall into
 an infinite loop.
\******************************************************************************/
static int farthest_path_tile(int ship)
{
        int tile, next, path_len, neighbors[3];

        tile = g_ships[ship].tile;
        for (path_len = 0; ; tile = next) {
                next = g_ships[ship].path[path_len++];
                if (next <= 0)
                        return tile;
                R_get_tile_neighbors(tile, neighbors);
                next = neighbors[next - 1];
                if (!G_open_tile(next, ship) || R_land_bridge(tile, next))
                        return tile;
        }
}

/******************************************************************************\
 Returns TRUE if the ship in a given tile is leaving it.
\******************************************************************************/
static bool ship_leaving_tile(int tile)
{
        int ship;

        C_assert(tile >= 0 && tile < r_tiles);
        ship = g_tiles[tile].ship;
        if (ship >= 0 && ship < G_SHIPS_MAX &&
            g_ships[ship].tile != g_ships[ship].rear_tile &&
            tile == g_ships[ship].rear_tile)
                return TRUE;
        return FALSE;
}

/******************************************************************************\
 Find a path from where the [ship] is to the target [tile] and sets that as
 the ship's new path.
\******************************************************************************/
void G_ship_path(int ship, int target)
{
        static int search_stamp;
        search_node_t nodes[SEARCH_BREADTH];
        int i, nodes_len, closest, path_len, neighbors[3];

        /* Tell other clients of the path change */
        if (n_client_id == N_HOST_CLIENT_ID)
                N_broadcast_except(N_HOST_CLIENT_ID, "1122", G_SM_SHIP_MOVE,
                                   ship, g_ships[ship].tile, target);

        /* Silent fail */
        if (target < 0 || target >= r_tiles || g_ships[ship].tile == target) {
                g_ships[ship].path[0] = NUL;
                g_ships[ship].target = g_ships[ship].tile;
                if (g_ships[ship].client == n_client_id &&
                    g_selected_ship == ship)
                        R_select_path(-1, NULL);
                return;
        }

        /* Set our target and see if its available */
        g_ships[ship].target = g_ships[ship].tile;
        if (!G_open_tile(target, ship))
                goto failed;

        /* Start with just the initial tile open */
        search_stamp++;
        nodes[0].tile = g_ships[ship].tile;
        nodes[0].dist = tile_dist(nodes[0].tile, target);
        nodes[0].moves = 0;
        nodes_len = 1;
        g_tiles[nodes[0].tile].search_parent = -1;
        g_tiles[nodes[0].tile].search_stamp = search_stamp;
        closest = 0;

        for (;;) {
                search_node_t node;

                node = nodes[closest];

                /* Ran out of search nodes -- no path to target */
                if (nodes_len < 1)
                        goto failed;

                /* Remove the node from the list */
                nodes_len--;
                memmove(nodes + closest, nodes + closest + 1,
                        (nodes_len - closest) * sizeof (*nodes));

                /* Add its children */
                R_get_tile_neighbors(node.tile, neighbors);
                for (i = 0; i < 3; i++) {
                        int stamp;
                        bool open;

                        /* Tile blocked? */
                        open = G_open_tile(neighbors[i], ship) ||
                               ship_leaving_tile(neighbors[i]);

                        /* Already opened? */
                        stamp = g_tiles[neighbors[i]].search_stamp;
                        C_assert(stamp <= search_stamp);
                        if (stamp == search_stamp || !open ||
                            R_land_bridge(node.tile, neighbors[i]))
                                continue;
                        g_tiles[neighbors[i]].search_stamp = search_stamp;

                        /* Out of space? */
                        if (nodes_len >= SEARCH_BREADTH) {
                                C_warning("Out of search space");
                                return;
                        }

                        /* Add to array */
                        nodes[nodes_len].tile = neighbors[i];
                        g_tiles[neighbors[i]].search_parent = node.tile;

                        /* Did we make it? */
                        if (neighbors[i] == target)
                                goto rewind;

                        /* Distance to destination */
                        nodes[nodes_len].dist = tile_dist(neighbors[i], target);
                        nodes[nodes_len].moves = node.moves + 1;

                        nodes_len++;
                }

                /* Find the new closest node */
                for (closest = 0, i = 1; i < nodes_len; i++)
                        if (2 * nodes[i].moves + nodes[i].dist <
                            2 * nodes[closest].moves + nodes[closest].dist)
                                closest = i;
        }

rewind: /* Count length of the path */
        path_len = -1;
        for (i = nodes[nodes_len].tile; i >= 0; i = g_tiles[i].search_parent)
                path_len++;

        /* The path is too long */
        if (path_len > R_PATH_MAX) {
                C_warning("Path is too long (%d tiles)", path_len);
                return;
        }

        /* Write the path backwards */
        g_ships[ship].path[path_len] = 0;
        for (i = nodes[nodes_len].tile; i >= 0; ) {
                int j, parent;

                parent = g_tiles[i].search_parent;
                if (parent < 0)
                        break;
                R_get_tile_neighbors(parent, neighbors);
                for (j = 0; neighbors[j] != i; j++);
                g_ships[ship].path[--path_len] = j + 1;
                i = parent;
        }

        /* Update ship selection */
        if (g_selected_ship == ship && g_ships[ship].client == n_client_id)
                R_select_path(g_ships[ship].tile, g_ships[ship].path);

        g_ships[ship].target = target;
        return;

failed: /* If we can't reach the target, and we have a valid path, try
           to at least get through some of it */
        i = farthest_path_tile(ship);
        if (i != target)
                G_ship_path(ship, i);
        else
                g_ships[ship].path[0] = 0;


        if (g_ships[ship].client == n_client_id) {
                const char *fmt;

                /* Update ship path selection */
                if (g_selected_ship == ship)
                        R_select_path(g_ships[ship].tile, g_ships[ship].path);

                fmt = C_str("i-ship-destination",
                            "%s can't reach destination.");
                I_popup(&g_tiles[g_ships[ship].tile].origin,
                        C_va(fmt, g_ships[ship].name));
        }
}

/******************************************************************************\
 Position and orient the ship's model.
\******************************************************************************/
static void position_ship(int ship)
{
        r_model_t *model;
        int new_tile, old_tile;

        new_tile = g_ships[ship].tile;
        old_tile = g_ships[ship].rear_tile;
        model = &g_tiles[new_tile].model;

        /* If the ship is not moving, just place it on the tile */
        if (g_ships[ship].rear_tile < 0) {
                model->normal = r_tile_params[new_tile].normal;
                model->origin = g_tiles[new_tile].origin;
        }

        /* Otherwise interpolate */
        else {
                c_vec3_t forward;
                float lerp;

                /* Interpolate normal */
                model->normal = C_vec3_lerp(r_tile_params[old_tile].normal,
                                            g_ships[ship].progress,
                                            r_tile_params[new_tile].normal);
                model->normal = C_vec3_norm(model->normal);

                /* Interpolate origin */
                model->origin = C_vec3_lerp(g_tiles[old_tile].origin,
                                            g_ships[ship].progress,
                                            g_tiles[new_tile].origin);

                /* Gradually rotate */
                forward = C_vec3_norm(C_vec3_sub(g_tiles[new_tile].origin,
                                                 g_tiles[old_tile].origin));
                lerp = ROTATION_RATE * c_frame_sec *
                       g_ship_classes[g_ships[ship].class_name].speed;
                if (lerp > 1.f)
                        lerp = 1.f;
                model->forward = C_vec3_lerp(model->forward, lerp, forward);
        }

        /* Make sure the forward vector is valid */
        model->forward = C_vec3_in_plane(model->forward, model->normal);
        model->forward = C_vec3_norm(model->forward);
}

/******************************************************************************\
 Transfer the model of a ship from one tile to another.
\******************************************************************************/
static void transfer_model(int from, int to)
{
        if (from == to)
                return;
        R_model_cleanup(&g_tiles[to].model);
        g_tiles[to].model = g_tiles[from].model;
        g_tiles[to].model_shown = TRUE;
        g_tiles[to].fade = 1.f;
        C_zero(&g_tiles[from].model);
}

/******************************************************************************\
 Move a ship to a new tile. Returns TRUE if the ship was moved.
\******************************************************************************/
bool G_ship_move_to(int i, int new_tile)
{
        int old_tile;

        old_tile = g_ships[i].tile;
        if (g_ships[i].rear_tile == new_tile || new_tile == old_tile ||
            !G_open_tile(new_tile, i))
                return FALSE;

        /* Remove this ship from the old tile */
        C_assert(g_ships[i].rear_tile != g_ships[i].tile);
        if (g_ships[i].rear_tile >= 0)
                g_tiles[g_ships[i].rear_tile].ship = -1;

        transfer_model(old_tile, new_tile);
        g_ships[i].rear_tile = old_tile;
        g_ships[i].tile = new_tile;
        g_tiles[new_tile].ship = i;
        return TRUE;
}

/******************************************************************************\
 Updates ship positions and actions.
\******************************************************************************/
void G_update_ships(void)
{
        int i;

        for (i = 0; i < G_SHIPS_MAX; i++) {
                float speed;

                if (!g_ships[i].in_use ||
                    (g_ships[i].path[0] <= 0 && g_ships[i].rear_tile < 0))
                        continue;
                speed = g_ship_classes[g_ships[i].class_name].speed;
                g_ships[i].progress += c_frame_sec * speed;

                /* Started moving to a new tile */
                if (g_ships[i].progress >= 1.f || g_ships[i].rear_tile < 0) {
                        int old_tile, new_tile, neighbors[3];
                        bool arrived, open;

                        g_ships[i].progress = 1.f;

                        /* Update the path */
                        G_ship_path(i, g_ships[i].target);

                        /* Get new destination tile */
                        if (!(arrived = g_ships[i].path[0] <= 0)) {
                                old_tile = g_ships[i].tile;
                                R_get_tile_neighbors(old_tile, neighbors);
                                new_tile = (int)(g_ships[i].path[0] - 1);
                                new_tile = neighbors[new_tile];
                        }

                        /* Remove this ship from the old tile */
                        C_assert(g_ships[i].rear_tile != g_ships[i].tile);
                        if (g_ships[i].rear_tile >= 0)
                                g_tiles[g_ships[i].rear_tile].ship = -1;

                        /* See if we hit an obstacle */
                        if (!arrived) {
                                open = G_open_tile(new_tile, i);

                                /* If there is a ship leaving the next tile,
                                   wait for it to move out instead of pathing */
                                if (!open && ship_leaving_tile(new_tile))
                                        continue;
                        }

                        /* If we arrived or hit an obstacle, stop */
                        if (arrived || !open) {
                                g_ships[i].path[0] = 0;
                                g_ships[i].rear_tile = -1;
                                position_ship(i);
                                continue;
                        }

                        /* Consume a path move */
                        memmove(g_ships[i].path, g_ships[i].path + 1,
                                R_PATH_MAX - 1);
                        if (g_selected_ship == i &&
                            g_ships[i].client == n_client_id)
                                R_select_path(new_tile, g_ships[i].path);

                        transfer_model(old_tile, new_tile);
                        g_ships[i].progress = g_ships[i].progress - 1.f;
                        g_ships[i].rear_tile = old_tile;
                        g_ships[i].tile = new_tile;
                        g_tiles[new_tile].ship = i;
                }

                /* Position the ship visually */
                position_ship(i);
        }
}

/******************************************************************************\
 Select a ship. Pass a negative [index] to deselect.
\******************************************************************************/
void G_ship_select(int index)
{
        g_ship_name_t class_name;
        i_color_t color;
        n_client_id_t client;
        bool own;

        if (g_selected_ship == index)
                return;
        g_selected_ship = index;
        own = FALSE;
        if (index >= 0) {
                own = g_ships[index].client == n_client_id;
                if (own && g_selected_ship == index)
                        R_select_path(g_ships[index].tile, g_ships[index].path);
                else
                        R_select_path(-1, NULL);
                client = g_ships[index].client;
                color = G_nation_to_color(g_clients[client].nation);
                class_name = g_ships[index].class_name;
                I_select_ship(color, g_ships[index].name,
                              g_clients[client].name,
                              g_ship_classes[class_name].name);
        } else {
                R_select_path(-1, NULL);
                I_deselect_ship();
        }
}

/******************************************************************************\
 Re-selects the currently selected ship if it is [index] and if its client
 is [client]. If either [index] or [client] is negative, they are ignored.
\******************************************************************************/
void G_ship_reselect(int index, n_client_id_t client)
{
        if (g_selected_ship < 0 ||
            (index >= 0 && g_selected_ship != index) ||
            (client >= 0 && g_ships[g_selected_ship].client != client))
                return;
        index = g_selected_ship;
        g_selected_ship = -1;
        G_ship_select(index);
}

