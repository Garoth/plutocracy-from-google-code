/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Spawning, updating, and rendering ships */

#include "g_common.h"

/* Maximum health value */
#define HEALTH_MAX 100

/* Maximum crew value */
#define CREW_MAX (G_SHIP_OPTIMAL_CREW * 400)

/* Ships array */
g_ship_t g_ships[G_SHIPS_MAX];

/* The ship the mouse is hovering over and the currently selected ship */
int g_hover_ship, g_selected_ship;

static int focus_stamp;

/******************************************************************************\
 Cleanup a ship.
\******************************************************************************/
static void ship_cleanup(int index)
{
        R_model_cleanup(&g_ships[index].model);
        C_zero(g_ships + index);
}

/******************************************************************************\
 Cleanup all ships.
\******************************************************************************/
void G_cleanup_ships(void)
{
        int i;

        for (i = 0; i < G_SHIPS_MAX; i++)
                ship_cleanup(i);
}

/******************************************************************************\
 Find an available tile around [tile] (including [tile]) and spawn a new ship
 of the given class there. If [tile] is negative, the ship will be placed on
 a random open tile. If [index] is negative, the first available slot will be
 used.
\******************************************************************************/
int G_ship_spawn(int index, n_client_id_t client, int tile, g_ship_type_t type)
{
        g_ship_t *ship;

        if (!N_client_valid(client) || tile >= r_tiles_max ||
            type < 0 || type >= G_SHIP_TYPES) {
                C_warning("Invalid parameters (%d, %d, %d, %d)",
                          index, client, tile, type);
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
        if (tile < 0)
                tile = G_random_open_tile();
        if (tile < 0)
                return -1;

        /* Find an available tile to start the ship on */
        if (!G_tile_open(tile, -1)) {
                int i, neighbors[12], len;

                /* Not being able to fit a ship in is common, so don't complain
                   if this happens */
                len = R_tile_region(tile, neighbors);
                for (i = 0; !G_tile_open(neighbors[i], -1); i++)
                        if (i >= len)
                                return -1;
                tile = neighbors[i];
        }

        /* Initialize ship structure */
        ship_cleanup(index);
        ship = g_ships + index;
        ship->in_use = TRUE;
        ship->type = type;
        ship->tile = ship->target = tile;
        ship->target_ship = -1;
        ship->rear_tile = -1;
        ship->progress = 1.f;
        ship->client = client;
        ship->health = g_ship_classes[type].health;
        ship->forward = r_tiles[tile].forward;
        ship->trade_tile = -1;
        ship->focus_stamp = -1;
        ship->boarding_ship = -1;

        /* Start out unnamed */
        C_strncpy_buf(ship->name, C_va("Unnamed #%d", index));

        /* Place the ship on the tile */
        R_model_init(&ship->model, g_ship_classes[type].model_path, TRUE);
        G_tile_position_model(tile, &ship->model);
        g_tiles[tile].ship = index;

        /* Initialize store */
        G_store_init(&ship->store, g_ship_classes[ship->type].cargo);

        /* If we are the server, tell other clients */
        if (n_client_id == N_HOST_CLIENT_ID)
                N_broadcast_except(N_HOST_CLIENT_ID, "11121", G_SM_SHIP_SPAWN,
                                   index, client, tile, type);

        /* If this is one of ours, name it */
        if (client == n_client_id) {
                G_get_name_buf(G_NT_SHIP, ship->name);
                N_send(N_SERVER_ID, "11s", G_CM_SHIP_NAME, index, ship->name);
        }

        /* If we spawned on a gib, collect it */
        G_ship_collect_gib(index);

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
        float crew, crew_max, health, health_max;
        int i;

        if (i_limbo)
                return;
        for (i = 0; i < G_SHIPS_MAX; i++) {
                ship = g_ships + i;
                if (!ship->in_use)
                        continue;
                C_assert(ship->tile >= 0 && ship->tile < r_tiles_max);
                C_assert(g_tiles[ship->tile].ship == i);
                tile = g_tiles + ship->tile;

                /* Don't bother rendering if the ship isn't visible */
                if (!tile->visible)
                        continue;

                /* Draw the status display */
                ship_class = g_ship_classes + ship->type;
                crew_max = ship->store.capacity * G_SHIP_OPTIMAL_CREW /
                           CREW_MAX;
                crew = (float)ship->store.cargo[G_CT_CREW].amount / CREW_MAX;
                health = (float)ship->health / HEALTH_MAX;
                health_max = (float)ship_class->health / HEALTH_MAX;
                color = g_nations[g_clients[ship->client].nation].color;
                R_render_ship_status(&ship->model, health, health_max,
                                     crew, crew_max, color,
                                     g_selected_ship == i,
                                     ship->client == n_client_id);

                /* Draw boarding status indicator */
                if (ship->boarding_ship >= 0) {
                        c_vec3_t origin_b;

                        origin_b = g_ships[ship->boarding_ship].model.origin;
                        R_render_ship_boarding(ship->model.origin, origin_b,
                                               color);
                }
        }
}

/******************************************************************************\
 Configure the interface to reflect the ship's cargo and trade status.
\******************************************************************************/
static void ship_configure_trade(int index)
{
        g_ship_t *ship, *partner;
        int i;

        /* Our client can't actually see this cargo -- we probably don't have
           the right data for it anyway! */
        ship = g_ships + index;
        if (index < 0 || !ship->store.visible[n_client_id]) {
                I_disable_trade();
                return;
        }

        /* Do we have a trading partner? */
        G_store_space(&ship->store);
        if (ship->trade_tile > 0) {
                partner = g_ships + g_tiles[ship->trade_tile].ship;
                I_enable_trade(ship->client == n_client_id, partner->name,
                               ship->store.space_used, ship->store.capacity);
        } else {
                partner = NULL;
                I_enable_trade(ship->client == n_client_id, NULL,
                               ship->store.space_used, ship->store.capacity);
        }

        /* Configure cargo items */
        for (i = 0; i < G_CARGO_TYPES; i++) {
                i_cargo_info_t info;
                g_cargo_t *cargo;

                /* Our side */
                cargo = ship->store.cargo + i;
                info.amount = cargo->amount;
                info.auto_buy = cargo->auto_buy;
                info.auto_sell = cargo->auto_sell;
                info.buy_price = cargo->buy_price;
                info.sell_price = cargo->sell_price;
                info.minimum = cargo->minimum;
                info.maximum = cargo->maximum;

                /* No trade partner */
                info.p_buy_limit = 0;
                info.p_buy_price = -1;
                info.p_sell_limit = 0;
                info.p_sell_price = -1;
                if (!partner) {
                        info.p_amount = -1;
                        I_configure_cargo(i, &info);
                        continue;
                }

                /* Trade partner's side */
                cargo = partner->store.cargo + i;
                info.p_amount = cargo->amount;
                if (cargo->auto_sell) {
                        info.p_buy_price = partner->store.cargo[i].sell_price;
                        info.p_buy_limit = G_limit_purchase(&ship->store,
                                                            &partner->store,
                                                            i, 999);
                }
                if (cargo->auto_buy) {
                        info.p_sell_price = partner->store.cargo[i].buy_price;
                        info.p_sell_limit = -G_limit_purchase(&ship->store,
                                                              &partner->store,
                                                              i, -999);
                }
                I_configure_cargo(i, &info);
        }
}

/******************************************************************************\
 Returns TRUE if a ship is capable of trading.
\******************************************************************************/
static bool ship_can_trade(int index)
{
        return index >= 0 && index < G_SHIPS_MAX &&
               g_ships[index].in_use && g_ships[index].rear_tile < 0 &&
               g_ships[index].health > 0;
}

/******************************************************************************\
 Returns TRUE if a ship can trade with the given tile.
\******************************************************************************/
bool G_ship_can_trade_with(int index, int tile)
{
        int i, neighbors[3];

        R_tile_neighbors(g_ships[index].tile, neighbors);
        for (i = 0; i < 3; i++)
                if (neighbors[i] == tile)
                        return ship_can_trade(g_tiles[tile].ship);
        return FALSE;
}

/******************************************************************************\
 Check if the ship needs a new trading partner.
\******************************************************************************/
static void ship_update_trade(int index)
{
        g_ship_t *ship;
        int i, trade_tile, neighbors[3];

        /* Only need to do this for our own ships */
        ship = g_ships + index;
        if (ship->client != n_client_id)
                return;

        /* Find a trading partner */
        trade_tile = -1;
        if (ship->rear_tile < 0) {
                R_tile_neighbors(ship->tile, neighbors);
                for (i = 0; i < 3; i++) {
                        if (!ship_can_trade(g_tiles[neighbors[i]].ship))
                                continue;
                        trade_tile = neighbors[i];

                        /* Found our old trading partner */
                        if (trade_tile == g_ships[index].trade_tile)
                                return;
                }
        }

        /* Still no partner */
        if (trade_tile < 0 && ship->trade_tile < 0)
                return;

        /* Update to reflect our new trading partner */
        ship->trade_tile = trade_tile;
        if (g_selected_ship == index)
                ship_configure_trade(index);
}

/******************************************************************************\
 Send updated cargo information to clients. If [client] is negative, all
 clients that can see the ship's cargo are updated and only modified cargo
 entries are communicated.
\******************************************************************************/
void G_ship_send_cargo(int index, n_client_id_t client)
{
        int i;
        bool broadcast;

        /* Host already knows everything */
        if (client == N_HOST_CLIENT_ID || n_client_id != N_HOST_CLIENT_ID)
                return;

        /* If crew changed, send the new value via status update */
        if (g_ships[index].store.modified & (1 << G_CT_CREW)) {
                g_ships[index].store.modified &= ~(1 << G_CT_CREW);
                g_ships[index].modified = TRUE;
        }

        /* Nothing is sent if broadcasting an unmodified store */
        if ((broadcast = client < 0 || client == N_BROADCAST_ID) &&
            !g_ships[index].modified)
                return;

        /* Pack all the cargo information */
        N_send_start();
        N_send_char(G_SM_SHIP_CARGO);
        N_send_char(index);
        G_store_send(&g_ships[index].store,
                     !broadcast || client == N_SELECTED_ID);

        /* Already selected the target clients */
        if (client == N_SELECTED_ID) {
                N_send_selected(NULL);
                return;
        }

        /* Send to clients that can see complete cargo information */
        if (broadcast) {
                for (i = 0; i < N_CLIENTS_MAX; i++)
                        n_clients[i].selected = g_ships[index].store.visible[i];
                N_send_selected(NULL);
                return;
        }

        /* Send to a single client */
        if (g_ships[index].store.visible[client])
                N_send(client, NULL);
}

/******************************************************************************\
 Sends out the ship's current state.
\******************************************************************************/
void G_ship_send_state(int ship, n_client_id_t client)
{
        if (n_client_id != N_HOST_CLIENT_ID)
                return;
        N_broadcast_except(N_HOST_CLIENT_ID, "111211", G_SM_SHIP_STATE,
                           ship, g_ships[ship].health,
                           g_ships[ship].store.cargo[G_CT_CREW].amount,
                           g_ships[ship].boarding, g_ships[ship].boarding_ship);
}

/******************************************************************************\
 Update which clients can see [ship]'s cargo. Also updates the ship's
 neighbors' visibility due to the [ship] having moved into its tile.
\******************************************************************************/
static void ship_update_visible(int ship)
{
        int i, client, neighbors[3];
        bool old_visible[N_CLIENTS_MAX], selected;

        memcpy(old_visible, g_ships[ship].store.visible, sizeof (old_visible));
        C_zero_buf(g_ships[ship].store.visible);

        /* We can always see the stores of our own ships */
        client = g_ships[ship].client;
        if (client >= 0 && client < N_CLIENTS_MAX)
                g_ships[ship].store.visible[client] = TRUE;

        /* Neighboring ships' clients can see our store */
        R_tile_neighbors(g_ships[ship].tile, neighbors);
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

        /* No need to send updates here if we aren't hosting */
        if (n_client_id != N_HOST_CLIENT_ID)
                return;

        /* Only send the update to clients that can see the store now but
           couldn't see it before */
        for (selected = FALSE, i = 0; i < N_CLIENTS_MAX; i++) {
                n_clients[i].selected = FALSE;
                if (i == N_HOST_CLIENT_ID || old_visible[i] ||
                    !g_ships[ship].store.visible[i])
                        continue;
                n_clients[i].selected = TRUE;
                selected = TRUE;
        }
        if (selected)
                G_ship_send_cargo(ship, N_SELECTED_ID);
}

/******************************************************************************\
 Feed the ship's crew at regular interval.
\******************************************************************************/
static void ship_update_food(int ship)
{
        int crew, available;

        /* Only the host updates food */
        if (n_client_id != N_HOST_CLIENT_ID)
                return;

        /* Not time to eat yet */
        if (c_time_msec < g_ships[ship].lunch_time ||
            g_ships[ship].store.cargo[G_CT_CREW].amount <= 0)
                return;
        crew = g_ships[ship].store.cargo[G_CT_CREW].amount;

        /* Consume rations first */
        if (g_ships[ship].store.cargo[G_CT_RATIONS].amount > 0) {
                available = G_cargo_nutritional_value(G_CT_RATIONS);
                G_store_add(&g_ships[ship].store, G_CT_RATIONS, -1);
        }

        /* Out of food, it's cannibalism time */
        else {
                available = G_cargo_nutritional_value(G_CT_CREW);
                G_store_add(&g_ships[ship].store, G_CT_CREW, -1);
        }

        g_ships[ship].lunch_time = c_time_msec + available / crew;

        /* Did the crew just starve to death? */
        if (g_ships[ship].store.cargo[G_CT_CREW].amount <= 0)
                G_ship_change_client(ship, N_SERVER_ID);
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
                G_ship_update_move(i);
                G_ship_update_combat(i);
                ship_update_trade(i);
                ship_update_visible(i);
                ship_update_food(i);

                /* If the cargo manfiest changed, send updates once per frame */
                if (g_ships[i].store.modified)
                        G_ship_send_cargo(i, -1);

                /* If the ship state changed, send an update */
                if (g_ships[i].modified)
                        G_ship_send_state(i, -1);
        }
}

/******************************************************************************\
 Update which ship the mouse is hovering over and setup the hover window.
 Pass -1 to deselect.
\******************************************************************************/
void G_ship_hover(int index)
{
        r_model_t *model;

        if (g_hover_ship >= 0)
                model = &g_ships[g_hover_ship].model;
        if (g_hover_ship == index) {
                if (index < 0)
                        return;

                /* Ship can get unhighlighted for whatever reason */
                if (!model->selected)
                        model->selected = R_MS_HOVER;

                return;
        }

        /* Deselect last hovered-over ship */
        if (g_hover_ship >= 0 && model->selected == R_MS_HOVER)
                model->selected = R_MS_NONE;

        /* Highlight the new ship */
        if ((g_hover_ship = index) < 0)
                return;
        model = &g_ships[index].model;
        if (!model->selected)
                model->selected = R_MS_HOVER;
}

/******************************************************************************\
 Setup the quick info window to reflect a ship's information.
\******************************************************************************/
static void ship_quick_info(int index)
{
        g_ship_t *ship;
        g_ship_class_t *ship_class;
        i_color_t color;
        float prop;
        int i, total;

        if (index < 0) {
                I_quick_info_close();
                return;
        }
        ship = g_ships + index;
        ship_class = g_ship_classes + ship->type;
        I_quick_info_show(ship->name);

        /* Owner */
        color = G_nation_to_color(g_clients[ship->client].nation);
        I_quick_info_add_color("Owner:", g_clients[ship->client].name, color);

        /* Health */
        color = I_COLOR_ALT;
        prop = (float)ship->health / ship_class->health;
        if (prop >= 0.67)
                color = I_COLOR_GOOD;
        if (prop <= 0.33)
                color = I_COLOR_BAD;
        I_quick_info_add_color("Health:", C_va("%d/%d", ship->health,
                                               ship_class->health), color);

        /* Crew */
        color = I_COLOR_ALT;
        i = ship->store.cargo[G_CT_CREW].amount;
        total = (int)(ship->store.capacity * G_SHIP_OPTIMAL_CREW);
        prop = (float)i / total;
        if (prop >= 0.67)
                color = I_COLOR_GOOD;
        if (prop <= 0.33)
                color = I_COLOR_BAD;
        I_quick_info_add_color("Crew:", C_va("%d/%d", i, total), color);
        if (i <= 0)
                return;

        /* Food supply before resorting to cannibalism */
        if (!g_ships[index].store.visible[n_client_id])
                return;
        for (total = i = 0; i < G_CARGO_TYPES; i++) {
                if (i == G_CT_CREW)
                        continue;
                total += G_cargo_nutritional_value(i) *
                         g_ships[index].store.cargo[i].amount;
        }
        total = (total + g_ships[index].store.cargo[G_CT_CREW].amount - 1) /
                g_ships[index].store.cargo[G_CT_CREW].amount;
        if (total > 60000)
                I_quick_info_add_color("Food:", C_va("%d min", total / 60000),
                                       I_COLOR_GOOD);
        else if (total > 0)
                I_quick_info_add_color("Food:", C_va("%d sec", total / 1000),
                                       I_COLOR_ALT);
        else
                I_quick_info_add_color("Food:", "STARVING", I_COLOR_BAD);

}

/******************************************************************************\
 Select a ship. Pass a negative [index] to deselect.
\******************************************************************************/
void G_ship_select(int index)
{
        r_model_t *model;

        if (g_selected_ship == index)
                return;

        /* Deselect previous ship */
        if (g_selected_ship >= 0) {
                model = &g_ships[g_selected_ship].model;
                model->selected = R_MS_NONE;
        }

        /* Select the new ship */
        if (index >= 0) {
                model = &g_ships[index].model;
                model->selected = R_MS_SELECTED;

                /* Only show the path of our own ships */
                if (g_ships[index].client == n_client_id)
                        R_select_path(g_ships[index].tile, g_ships[index].path);
                else
                        R_select_path(-1, NULL);
        }

        /* Deselect */
        else
                R_select_path(-1, NULL);

        ship_configure_trade(index);
        ship_quick_info(index);

        /* Selecting a ship resets the focus order */
        focus_stamp++;

        g_selected_ship = index;
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

/******************************************************************************\
 Returns TRUE if a ship index is valid and can be controlled by the given
 client. Also doubles as a way to check if the index is valid to begin with.
\******************************************************************************/
bool G_ship_controlled_by(int index, n_client_id_t client)
{
        return index >= 0 && index < G_SHIPS_MAX && g_ships[index].in_use &&
               g_ships[index].health > 0 && g_ships[index].client == client;
}

/******************************************************************************\
 Returns TRUE if the ship is owned by a hostile player.
\******************************************************************************/
bool G_ship_hostile(int index, n_client_id_t to_client)
{
        if (index < 0 || index >= G_SHIPS_MAX || !g_ships[index].in_use)
                return FALSE;

        /* For the tech preview, all nations are permanently at war */
        return g_clients[g_ships[index].client].nation !=
               g_clients[to_client].nation;
}

/******************************************************************************\
 Focus on the next suitable ship.
\******************************************************************************/
void G_focus_next_ship(void)
{
        float best_dist;
        int i, best_i, tile, available;

        /* If we have a ship selected, just center on that */
        if (g_selected_ship >= 0) {
                tile = g_ships[g_selected_ship].tile;
                R_rotate_cam_to(g_ships[g_selected_ship].model.origin);
                return;
        }

        /* Find the closest available ship */
        available = 0;
        best_i = -1;
        best_dist = C_FLOAT_MAX;
        for (i = 0; i < G_SHIPS_MAX; i++) {
                c_vec3_t origin;
                float dist;

                if (!G_ship_controlled_by(i, n_client_id) ||
                    g_ships[i].focus_stamp >= focus_stamp)
                        continue;
                available++;
                origin = g_ships[i].model.origin;
                dist = C_vec3_len(C_vec3_sub(r_cam_origin, origin));
                if (dist < best_dist) {
                        best_dist = dist;
                        best_i = i;
                }
        }
        if (available <= 1)
                focus_stamp++;
        if (best_i < 0)
                return;
        g_ships[best_i].focus_stamp = focus_stamp;
        tile = g_ships[best_i].tile;
        R_rotate_cam_to(g_ships[best_i].model.origin);
}

/******************************************************************************\
 Change a ship's owner.
\******************************************************************************/
void G_ship_change_client(int ship, n_client_id_t client)
{
        N_broadcast("111", G_SM_SHIP_OWNER, ship, client);
}

/******************************************************************************\
 Check if the ship's tile has a crate gib and give its contents to the ship.
\******************************************************************************/
void G_ship_collect_gib(int ship)
{
        int i, tile;

        tile = g_ships[ship].tile;
        if (n_client_id != N_HOST_CLIENT_ID || !g_tiles[tile].gib)
                return;
        for (i = 0; i < G_CARGO_TYPES; i++)
                if (g_tiles[tile].gib->loot.cargo[i] > 0)
                        G_store_add(&g_ships[ship].store, i,
                                    g_tiles[tile].gib->loot.cargo[i]);
        G_tile_gib(tile, G_GT_NONE);
}

/******************************************************************************\
 Create a crate gib around the ship with the dropped cargo.
\******************************************************************************/
void G_ship_drop_cargo(int ship, g_cargo_type_t type, int amount)
{
        g_gib_t *gib;
        int i, neighbors[3], tile;

        /* Limit amount */
        if (g_ships[ship].store.cargo[type].amount < amount)
                amount = g_ships[ship].store.cargo[type].amount;
        if (amount < 1)
                return;

        /* Don't let them drop the last crew member */
        if (type == G_CT_CREW &&
            g_ships[ship].store.cargo[type].amount - amount < 1)
                amount = g_ships[ship].store.cargo[type].amount - 1;

        /* Find an existing crate or an open tile */
        R_tile_neighbors(g_ships[ship].tile, neighbors);
        for (tile = -1, i = 0; i < 3; i++) {
                if (G_tile_open(neighbors[i], -1))
                        tile = neighbors[i];
                if ((gib = g_tiles[neighbors[i]].gib))
                        break;
        }

        /* Spawn a new gib */
        if (!gib) {
                if (tile < 0)
                        return;
                G_tile_gib(tile, G_GT_CRATES);
                gib = g_tiles[tile].gib;
        }

        gib->loot.cargo[type] += amount;
        G_store_add(&g_ships[ship].store, type, -amount);
}

