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
 Find an available tile around [tile] (including [tile]) and spawn a new ship
 of the given class there. If [tile] is negative, the ship will be placed on
 a random open tile. If [index] is negative, the first available slot will be
 used.
\******************************************************************************/
int G_spawn_ship(n_client_id_t client, int tile, g_ship_name_t name, int index)
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
        g_ships[index].forward = g_tiles[tile].forward;

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
 Configure the interface to reflect the ship's cargo and trade status.
\******************************************************************************/
static void ship_configure_trade(int index)
{
        int i;

        /* Our client can't actually see this cargo -- we probably don't have
           the right data for it anyway! */
        if (index < 0 || !g_ships[index].store.visible[n_client_id]) {
                I_enable_trade(FALSE, FALSE, FALSE);
                return;
        }

        I_enable_trade(TRUE, g_ships[index].client == n_client_id, FALSE);
        I_set_cargo_space(G_store_space(&g_ships[index].store),
                          g_ship_classes[g_ships[index].class_name].cargo);
        for (i = 0; i < G_CARGO_TYPES; i++) {
                i_cargo_data_t left;
                g_cargo_t *cargo;

                cargo = g_ships[index].store.cargo + i;

                /* Our cargo */
                left.amount = cargo->amount;
                left.minimum = cargo->minimum;
                left.maximum = cargo->maximum;
                left.sell_price = cargo->sell_price;
                left.buy_price = cargo->buy_price;
                left.auto_buy = cargo->auto_buy;
                left.auto_sell = cargo->auto_sell;

                I_configure_cargo(i, &left, NULL);
        }
}

/******************************************************************************\
 Update which clients can see [ship]'s cargo. Also updates the ship's
 neighbors' visibility due to the [ship] having moved into its tile.
\******************************************************************************/
static void ship_update_visible(int ship)
{
        int i, client, neighbors[3];
        bool old_visible[N_CLIENTS_MAX];

        memcpy(old_visible, g_ships[ship].store.visible, sizeof (old_visible));
        C_zero_buf(g_ships[ship].store.visible);

        /* We can always see the stores of our own ships */
        client = g_ships[ship].client;
        if (client >= 0 && client < N_CLIENTS_MAX)
                g_ships[ship].store.visible[client] = TRUE;

        /* Neighboring ships' clients can see our store */
        R_get_tile_neighbors(g_ships[ship].tile, neighbors);
        for (i = 0; i < 3; i++) {
                if (g_tiles[neighbors[i]].ship < 0)
                        continue;
                client = g_ships[g_tiles[neighbors[i]].ship].client;
                if (client >= 0 && client < N_CLIENTS_MAX)
                        g_ships[ship].store.visible[client] = TRUE;
        }

        /* Setup trade if our client's visibility toward this ship changed and
           we have it selected */
        if (g_selected_ship == ship && old_visible[n_client_id] !=
                                       g_ships[ship].store.visible[n_client_id])
                ship_configure_trade(ship);

        /* Update clients' cargo info */
        if (n_client_id != N_HOST_CLIENT_ID)
                return;

        /* Only send the update to clients that can see the store now but
           couldn't see it before */
        for (i = 0; i < N_CLIENTS_MAX; i++) {
                n_clients[i].selected = FALSE;
                if (i == N_HOST_CLIENT_ID)
                        continue;
                if (!old_visible[i] && g_ships[ship].store.visible[i])
                        n_clients[i].selected = TRUE;
        }

        G_ship_send_cargo(ship, N_SELECTED_ID);
}

/******************************************************************************\
 Updates ship positions and actions.
\******************************************************************************/
void G_update_ships(void)
{
        int i;

        for (i = 0; i < G_SHIPS_MAX; i++) {
                if (!g_ships[i].in_use)
                        continue;

                /* Update visibility every frame */
                ship_update_visible(i);

                /* Move the ship along its path */
                G_ship_update_move(i);
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
                ship_configure_trade(index);
        } else {
                R_select_path(-1, NULL);
                I_deselect_ship();
                ship_configure_trade(-1);
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

