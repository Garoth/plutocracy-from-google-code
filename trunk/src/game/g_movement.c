/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Handles ship movement */

#include "g_common.h"

/* Maximum breadth of a path search */
#define SEARCH_BREADTH (R_PATH_MAX * 3)

/* Proportion of the remaining rotation a ship does per second */
#define ROTATION_RATE 3.f

/* Factor in the speed modifier equation that determines how quickly adding
   crew increases ship speed. The larger the factor the faster speed will
   rise with crew. */
#define CREW_SPEED_FACTOR 5.f

/* Structure for searched tile nodes */
typedef struct search_node {
        float dist;
        int tile, moves;
} search_node_t;

/******************************************************************************\
 Returns TRUE if a ship can sail into the given tile.
\******************************************************************************/
bool G_open_tile(int tile, int ship)
{
        return (g_tiles[tile].ship < 0 ||
                (ship >= 0 && g_tiles[tile].ship == ship)) &&
               R_water_terrain(r_tiles[tile].terrain);
}

/******************************************************************************\
 Returns the linear distance from one tile to another. This is the search
 function heuristic.
\******************************************************************************/
static float tile_dist(int a, int b)
{
        return C_vec3_len(C_vec3_sub(r_tiles[b].origin,
                                     r_tiles[a].origin));
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
                R_tile_neighbors(tile, neighbors);
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

        C_assert(tile >= 0 && tile < r_tiles_max);
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
        bool target_next;

        /* New destination? */
        if (g_ships[ship].target != target)
                g_ships[ship].modified = TRUE;

        /* Silent fail */
        if (target < 0 || target >= r_tiles_max ||
            g_ships[ship].tile == target) {
                g_ships[ship].path[0] = NUL;
                g_ships[ship].target = g_ships[ship].tile;
                if (g_ships[ship].client == n_client_id &&
                    g_selected_ship == ship)
                        R_select_path(-1, NULL);
                return;
        }

        /* Clear the target for now */
        g_ships[ship].target = g_ships[ship].tile;

        /* If the target tile is not available, try to get next to it instead
           of onto it */
        target_next = !G_open_tile(target, ship);

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
                R_tile_neighbors(node.tile, neighbors);
                for (i = 0; i < 3; i++) {
                        int stamp;
                        bool open;

                        /* Out of space? */
                        if (nodes_len >= SEARCH_BREADTH) {
                                C_warning("Out of search space");
                                return;
                        }

                        /* Made it to an adjacent tile */
                        if (target_next && neighbors[i] == target) {
                                nodes[nodes_len] = node;
                                goto rewind;
                        }

                        /* Tile blocked? */
                        open = G_open_tile(neighbors[i], ship) ||
                               ship_leaving_tile(neighbors[i]);

                        /* Can we open this node? */
                        stamp = g_tiles[neighbors[i]].search_stamp;
                        C_assert(stamp <= search_stamp);
                        if (stamp == search_stamp || !open ||
                            R_land_bridge(node.tile, neighbors[i]))
                                continue;
                        g_tiles[neighbors[i]].search_stamp = search_stamp;

                        /* Add to array */
                        nodes[nodes_len].tile = neighbors[i];
                        g_tiles[neighbors[i]].search_parent = node.tile;

                        /* Did we make it onto the target? */
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
                R_tile_neighbors(parent, neighbors);
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

        /* Can't reach target ship */
        g_ships[ship].target_ship = -1;

        if (g_ships[ship].client == n_client_id) {
                const char *fmt;

                /* Update ship path selection */
                if (g_selected_ship == ship)
                        R_select_path(g_ships[ship].tile, g_ships[ship].path);

                fmt = C_str("i-ship-destination",
                            "%s can't reach destination.");
                I_popup(&r_tiles[g_ships[ship].tile].origin,
                        C_va(fmt, g_ships[ship].name));
        }
}

/******************************************************************************\
 Return the speed of a ship with all modifiers applied.
\******************************************************************************/
static float ship_speed(int index)
{
        g_store_t *store;
        float crew, speed;

        speed = g_ship_classes[g_ships[index].type].speed;
        store = &g_ships[index].store;

        /* Crew modifier */
        C_assert(store->cargo[G_CT_CREW].amount >= 0);
        crew = (float)store->cargo[G_CT_CREW].amount /
               (G_SHIP_OPTIMAL_CREW * store->capacity);
        if (crew > 1.f)
                crew = 1.f;
        speed *= (-1.f / (CREW_SPEED_FACTOR * crew + 1.f) + 1.f) *
                 (1.f + 1.f / CREW_SPEED_FACTOR);

        return speed;
}

/******************************************************************************\
 Position and orient the ship's model.
\******************************************************************************/
static void ship_position_model(int ship)
{
        r_model_t *model;
        int new_tile, old_tile;

        if (ship < 0 || ship >= G_SHIPS_MAX || !g_ships[ship].in_use)
                return;
        new_tile = g_ships[ship].tile;
        old_tile = g_ships[ship].rear_tile;
        model = &g_ships[ship].model;

        /* If the ship is not moving, just place it on the tile */
        if (g_ships[ship].rear_tile < 0) {
                model->normal = r_tiles[new_tile].normal;
                model->origin = r_tiles[new_tile].origin;
        }

        /* Otherwise interpolate normal and origin */
        else {
                model->normal = C_vec3_lerp(r_tiles[old_tile].normal,
                                            g_ships[ship].progress,
                                            r_tiles[new_tile].normal);
                model->normal = C_vec3_norm(model->normal);
                model->origin = C_vec3_lerp(r_tiles[old_tile].origin,
                                            g_ships[ship].progress,
                                            r_tiles[new_tile].origin);
        }

        /* Rotate toward the forward vector */
        if (!C_vec3_eq(model->forward, g_ships[ship].forward)) {
                float lerp;

                lerp = ROTATION_RATE * c_frame_sec * ship_speed(ship);
                if (lerp > 1.f)
                        lerp = 1.f;
                model->forward = C_vec3_norm(model->forward);
                model->forward = C_vec3_rotate_to(model->forward, model->normal,
                                                  lerp, g_ships[ship].forward);
        }
}

/******************************************************************************\
 Move a ship to a new tile. Returns TRUE if the ship was moved. This is only
 for cases where the client and server are out of sync.
\******************************************************************************/
bool G_ship_move_to(int i, int new_tile)
{
        int old_tile, neighbors[3];

        old_tile = g_ships[i].tile;

        /* Don't bother if we are already there */
        if (g_ships[i].rear_tile == new_tile || new_tile == old_tile ||
            !G_open_tile(new_tile, i))
                return FALSE;

        /* Don't bother if we will be there on our next move */
        R_tile_neighbors(g_ships[i].tile, neighbors);
        if (g_ships[i].path[0] && neighbors[g_ships[i].path[0] - 1] == new_tile)
                return FALSE;

        /* Remove this ship from the old tile */
        C_assert(g_ships[i].rear_tile != g_ships[i].tile);
        if (g_ships[i].rear_tile >= 0)
                g_tiles[g_ships[i].rear_tile].ship = -1;

        /* Move to the new tile */
        g_ships[i].rear_tile = old_tile;
        g_ships[i].tile = new_tile;
        g_tiles[new_tile].ship = i;

        /* Make a new path to our target */
        G_ship_path(i, g_ships[i].target);

        return TRUE;
}

/******************************************************************************\
 Move a ship forward.
\******************************************************************************/
static void ship_move(int i)
{
        c_vec3_t forward;
        int old_tile, new_tile, neighbors[3];
        bool arrived, open;

        /* Stopped and target ship is moving */
        if (g_ships[i].rear_tile < 0 && g_ships[i].target_ship >= 0 &&
            g_ships[g_ships[i].target_ship].rear_tile >= 0)
                G_ship_path(i, g_ships[g_ships[i].target_ship].tile);

        /* Is this ship moving? */
        if (g_ships[i].path[0] <= 0 && g_ships[i].rear_tile < 0)
                return;
        g_ships[i].progress += c_frame_sec * ship_speed(i);

        /* Still in progress */
        if (g_ships[i].progress < 1.f)
                return;
        g_ships[i].progress = 1.f;

        /* Can't move while boarding */
        if (g_ships[i].boarding > 0)
                return;

        /* Keep track of the target ship */
        if (g_ships[i].target_ship >= 0 &&
            g_ships[g_ships[i].target_ship].tile != g_ships[i].target) {
                g_ships[i].target = g_ships[g_ships[i].target_ship].tile;
                g_ships[i].modified = TRUE;
        }

        /* Update the path */
        G_ship_path(i, g_ships[i].target);

        /* Get new destination tile */
        if (!(arrived = g_ships[i].path[0] <= 0)) {
                old_tile = g_ships[i].tile;
                R_tile_neighbors(old_tile, neighbors);
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
                        return;
        }

        /* If we arrived or hit an obstacle, stop */
        if (arrived || !open) {
                g_ships[i].path[0] = 0;
                g_ships[i].rear_tile = -1;
                return;
        }

        /* Consume a path move */
        memmove(g_ships[i].path, g_ships[i].path + 1,
                R_PATH_MAX - 1);
        if (g_selected_ship == i &&
            g_ships[i].client == n_client_id)
                R_select_path(new_tile, g_ships[i].path);

        /* Rotate to face the new tile */
        forward = C_vec3_norm(C_vec3_sub(r_tiles[new_tile].origin,
                                         r_tiles[old_tile].origin));

        g_ships[i].progress = g_ships[i].progress - 1.f;
        g_ships[i].rear_tile = old_tile;
        g_ships[i].tile = new_tile;
        g_ships[i].forward = forward;
        g_tiles[new_tile].ship = i;
}

/******************************************************************************\
 Update a ship's movement.
\******************************************************************************/
void G_ship_update_move(int i)
{
        ship_move(i);
        ship_position_model(i);
}

