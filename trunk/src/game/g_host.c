/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Primarily handles client messages for the server. Remember that the client
   is not trusted and all input must be checked for validity! */

#include "g_common.h"

/* Maximum number of crates to spawn */
#define CRATES_MAX 32

/* This game's client limit */
int g_clients_max;

/* TRUE if the server has finished initializing */
bool g_host_inited;

/******************************************************************************\
 Handles clients changing nations.
\******************************************************************************/
static void cm_affiliate(int client)
{
        int nation, old, tile, ship;

        if ((nation = G_receive_nation(client)) < 0 ||
            nation == g_clients[client].nation)
                return;
        old = g_clients[client].nation;
        g_clients[client].nation = nation;

        /* If this client just joined a nation for the first time,
           try to give them a starter ship */
        tile = -1;
        if (old == G_NN_NONE &&
            (ship = G_ship_spawn(-1, client, -1, G_ST_SPIDER)) >= 0) {
                tile = g_ships[ship].tile;
                G_store_add(&g_ships[ship].store, G_CT_GOLD, 500);
                G_store_add(&g_ships[ship].store, G_CT_CREW, 25);
                G_store_add(&g_ships[ship].store, G_CT_RATIONS, 25);
        }

        N_broadcast("1112", G_SM_AFFILIATE, client, nation, tile);
}

/******************************************************************************\
 Handles orders to move ships.
\******************************************************************************/
static void cm_ship_move(int client)
{
        int ship, tile;

        if ((ship = G_receive_ship(client)) < 0 ||
            (tile = G_receive_tile(client)) < 0 ||
            !G_ship_controlled_by(ship, client))
                return;
        g_ships[ship].target_ship = -1;
        G_ship_path(ship, tile);
}

/******************************************************************************\
 Called when a client wants to change their name.
\******************************************************************************/
static void cm_name(int client)
{
        int i, suffixes, name_len;
        char name_buf[G_NAME_MAX];

        N_receive_string_buf(name_buf);

        /* Must have a valid name */
        C_sanitize(name_buf);
        if (!name_buf[0])
                C_strncpy_buf(name_buf, "Newbie");

        /* Didn't actually change names */
        if (!strcmp(name_buf, g_clients[client].name))
                return;

        /* See if this name is taken */
        name_len = C_strlen(name_buf);
        for (suffixes = 2, i = 0; i < N_CLIENTS_MAX; i++) {
                if (i == client || strcasecmp(name_buf, g_clients[i].name))
                        continue;

                /* Its taken, add a suffix and try again */
                name_buf[name_len] = NUL;
                name_len = C_suffix_buf(name_buf, C_va(" %d", suffixes));

                /* Start over */
                i = -1;
                suffixes++;
        }

        /* Didn't actually change names (after suffix attachment) */
        if (!strcmp(name_buf, g_clients[client].name))
                return;

        C_debug("Client '%s' (%d) renamed to '%s'",
                g_clients[client].name, client, name_buf);
        N_broadcast("11s", G_SM_NAME, client, name_buf);
}

/******************************************************************************\
 Client wants to rename a ship.
\******************************************************************************/
static void cm_ship_name(int client)
{
        int index;
        char new_name[G_NAME_MAX];

        if ((index = G_receive_ship(client)) < 0 ||
            !G_ship_controlled_by(index, client))
                return;
        N_receive_string_buf(new_name);
        C_sanitize(new_name);
        if (!new_name[0])
                return;
        C_strncpy_buf(g_ships[index].name, new_name);
        N_broadcast_except(client, "11s", G_SM_SHIP_NAME, index,
                           g_ships[index].name);
        C_debug("'%s' named ship %d '%s'", g_clients[client].name,
                index, new_name);
}

/******************************************************************************\
 Client changed cargo prices.
\******************************************************************************/
static void cm_ship_prices(int client)
{
        int i, index, cargo, buy_price, sell_price, minimum, maximum;

        if ((index = G_receive_ship(client)) < 0 ||
            !G_ship_controlled_by(index, client))
                return;
        cargo = N_receive_char();
        if (cargo < 0 || cargo >= G_CARGO_TYPES) {
                G_corrupt_drop(client);
                return;
        }

        /* Prices */
        buy_price = N_receive_short();
        sell_price = N_receive_short();
        if (buy_price > 999)
                buy_price = 999;
        if (sell_price > 999)
                sell_price = 999;

        /* Quantities */
        minimum = N_receive_short();
        maximum = N_receive_short();

        /* Select clients that can see this store */
        for (i = 0; i < N_CLIENTS_MAX; i++)
                if (g_ships[index].store.visible[i])
                        n_clients[i].selected = TRUE;

        /* The host needs to see this message to process the update */
        n_clients[N_HOST_CLIENT_ID].selected = TRUE;

        /* Originating client already knows what the prices are */
        n_clients[client].selected = FALSE;

        N_send_selected("1112222", G_SM_SHIP_PRICES, index, cargo,
                        buy_price, sell_price, minimum, maximum);
}

/******************************************************************************\
 Client wants to buy something.
\******************************************************************************/
static void cm_ship_buy(int client)
{
        g_store_t *buyer, *seller;
        int ship, trade_tile, trade_ship, cargo, amount, gold;

        if ((ship = G_receive_ship(client)) < 0 ||
            (trade_tile = G_receive_tile(client)) < 0 ||
            (cargo = G_receive_cargo(client)) < 0 ||
            !G_ship_controlled_by(ship, client) ||
            !G_ship_can_trade_with(ship, trade_tile))
                return;
        trade_ship = g_tiles[trade_tile].ship;
        amount = N_receive_short();
        buyer = &g_ships[ship].store;
        seller = &g_ships[trade_ship].store;
        amount = G_limit_purchase(buyer, seller, cargo, amount);
        if (amount == 0)
                return;

        /* Do the transfer. Cargo must be subtracted before it is added or
           it could overflow and get clamped! */
        if (amount > 0) {
                gold = seller->cargo[cargo].sell_price * amount;
                G_store_add(buyer, G_CT_GOLD, -gold);
                G_store_add(seller, cargo, -amount);
                G_store_add(seller, G_CT_GOLD, gold);
                G_store_add(buyer, cargo, amount);
        } else {
                amount = -amount;
                gold = seller->cargo[cargo].buy_price * amount;
                G_store_add(seller, G_CT_GOLD, -gold);
                G_store_add(buyer, cargo, -amount);
                G_store_add(buyer, G_CT_GOLD, gold);
                G_store_add(seller, cargo, amount);
        }
}

/******************************************************************************\
 Client wants to drop some cargo.
\******************************************************************************/
static void cm_ship_drop(int client)
{
        int ship, cargo, amount;

        if ((ship = G_receive_ship(client)) < 0 ||
            (cargo = G_receive_cargo(client)) < 0 ||
            !G_ship_controlled_by(ship, client) ||
            (amount = N_receive_short()) < 0)
                return;
        G_ship_drop_cargo(ship, cargo, amount);
}

/******************************************************************************\
 Client typed something into chat.
\******************************************************************************/
static void cm_chat(int client)
{
        char chat_buffer[N_SYNC_MAX];

        N_receive_string_buf(chat_buffer);
        C_sanitize(chat_buffer);
        if (!chat_buffer[0])
                return;
        N_broadcast_except(client, "11s", G_SM_CHAT, client, chat_buffer);
}

/******************************************************************************\
 Client wants to do something to a tile via a ring command.
\******************************************************************************/
static void cm_tile_ring(int client)
{
        g_building_class_t *bc;
        i_ring_icon_t icon;
        int tile;

        tile = G_receive_tile(client);
        icon = (i_ring_icon_t)G_receive_range(client, 0, I_RING_ICONS);
        if (tile < 0 || icon < 0)
                return;

        /* Client wants to build a shipyard (tech preview) */
        if (icon == I_RI_SHIPYARD) {
                bc = g_building_classes + G_BT_SHIPYARD;

                /* Can you do it? */
                if (!G_pay(client, tile, &bc->cost, FALSE))
                        return;

                /* Pay for and build the town hall */
                G_pay(client, tile, &bc->cost, TRUE);
                G_tile_build(tile, G_BT_SHIPYARD, g_clients[client].nation);
                return;
        }

        /* Client wants to buy a ship from their shipyard */
        if (g_tiles[tile].building &&
            g_tiles[tile].building->type == G_BT_SHIPYARD) {
                g_ship_type_t type;
                int ship;

                /* What ship do they want? */
                if (icon == I_RI_SLOOP)
                        type = G_ST_SLOOP;
                else if (icon == I_RI_SPIDER)
                        type = G_ST_SPIDER;
                else if (icon == I_RI_GALLEON)
                        type = G_ST_GALLEON;
                else
                        return;

                /* Can they pay? */
                if (!G_pay(client, tile, &g_ship_classes[type].cost, FALSE))
                        return;

                /* Pay for and spawn the ship. Don't pay if we couldn't spawn
                   it for some reason! */
                ship = G_ship_spawn(-1, client, tile, type);
                if (ship < 0)
                        return;
                G_store_add(&g_ships[ship].store, G_CT_CREW, 1);
                G_pay(client, tile, &g_ship_classes[type].cost, TRUE);
                return;
        }
}

/******************************************************************************\
 Client wants to do something with a ship using the ring.
\******************************************************************************/
static void cm_ship_ring(int client)
{
        i_ring_icon_t icon;
        int ship, target_ship;

        if ((ship = G_receive_ship(client)) < 0)
                return;
        icon = (i_ring_icon_t)G_receive_range(client, 0, I_RING_ICONS);
        if ((target_ship = G_receive_ship(client)) < 0)
                return;

        /* Follow the target */
        if (icon == I_RI_FOLLOW)
                g_ships[ship].target_board = FALSE;

        /* Board the target */
        else if (icon == I_RI_BOARD) {

                /* Don't attack friendlies! */
                if (!G_ship_hostile(ship, g_ships[target_ship].client))
                        return;

                g_ships[ship].target_board = TRUE;
        }

        g_ships[ship].target_ship = target_ship;
        G_ship_path(ship, g_ships[target_ship].tile);
}

/******************************************************************************\
 Initialize a new client.
\******************************************************************************/
static void init_client(int client)
{
        int i;

        /* The server already has all of the information */
        if (client == N_HOST_CLIENT_ID)
                return;

        /* This client has already been counted toward the total, kick them
           if this is more players than we want */
        if (n_clients_len > g_clients_max) {
                N_send(client, "12ss", G_SM_POPUP,
                       "g-host-full", "Server is full.");
                N_drop_client(client);
                return;
        }

        C_debug("Initializing client %d", client);
        g_clients[client].nation = G_NN_NONE;

        /* Communicate the globe info */
        N_send(client, "12111422ff", G_SM_INIT, G_PROTOCOL, client,
               g_clients_max, g_globe_subdiv4.value.n, g_globe_seed.value.n,
               g_island_num.value.n, g_island_size.value.n,
               g_island_variance.value.f, r_solar_angle);

        /* Tell them about everyone already here */
        for (i = 0; i < N_CLIENTS_MAX; i++)
                if (n_clients[i].connected && g_clients[i].name[0])
                        N_send(client, "111s", G_SM_CLIENT, i,
                               g_clients[i].nation, g_clients[i].name);

        /* Tell them about the buildings and gibs on them globe */
        for (i = 0; i < r_tiles_max; i++) {
                if (g_tiles[i].building)
                        N_send(client, "121", G_SM_BUILDING, i,
                               g_tiles[i].building->type);
                if (g_tiles[i].gib)
                        N_send(client, "121", G_SM_GIB, i,
                               g_tiles[i].gib->type);
        }

        /* Tell them about all the ships on the globe */
        for (i = 0; i < G_SHIPS_MAX; i++) {
                if (!g_ships[i].in_use)
                        continue;
                N_send(client, "11121", G_SM_SHIP_SPAWN, i, g_ships[i].client,
                       g_ships[i].tile, g_ships[i].type);
                N_send(client, "11s", G_SM_SHIP_NAME, i, g_ships[i].name);
                if (!g_ships[i].modified)
                        G_ship_send_state(i, client);
        }
}

/******************************************************************************\
 A client has left the game and we need to clean up.
\******************************************************************************/
static void client_disconnected(int client)
{
        int i;

        if (g_clients[client].name[0])
                N_broadcast("11", G_SM_DISCONNECTED, client);
        C_debug("Client %d disconnected", client);

        /* Disown their ships */
        for (i = 0; i < G_SHIPS_MAX; i++)
                if (g_ships[i].client == client)
                        G_ship_change_client(i, N_SERVER_ID);
}

/******************************************************************************\
 Called from within the network namespace when a network event arrives for the
 server from one of the clients (including the server's client).
\******************************************************************************/
static void server_callback(int client, n_event_t event)
{
        g_client_msg_t token;

        /* Special client events */
        if (event == N_EV_CONNECTED) {
                N_broadcast("11", G_SM_CONNECTED, client);
                init_client(client);
                return;
        } else if (event == N_EV_DISCONNECTED) {
                client_disconnected(client);
                return;
        }

        /* Handle messages */
        if (event != N_EV_MESSAGE)
                return;
        token = (g_client_msg_t)N_receive_char();
        switch (token) {
        case G_CM_AFFILIATE:
                cm_affiliate(client);
                break;
        case G_CM_CHAT:
                cm_chat(client);
                break;
        case G_CM_NAME:
                cm_name(client);
                break;
        case G_CM_SHIP_BUY:
                cm_ship_buy(client);
                break;
        case G_CM_SHIP_DROP:
                cm_ship_drop(client);
                break;
        case G_CM_SHIP_NAME:
                cm_ship_name(client);
                break;
        case G_CM_SHIP_MOVE:
                cm_ship_move(client);
                break;
        case G_CM_SHIP_PRICES:
                cm_ship_prices(client);
                break;
        case G_CM_SHIP_RING:
                cm_ship_ring(client);
                break;
        case G_CM_TILE_RING:
                cm_tile_ring(client);
                break;
        default:
                break;
        }
}

/******************************************************************************\
 Place starter buildings.
\******************************************************************************/
static void initial_buildings(void)
{
        int i;

        for (i = 0; i < r_tiles_max; i++) {
                if (R_terrain_base(r_tiles[i].terrain) != R_T_GROUND)
                        continue;
                if (C_rand_real() < g_forest.value.f)
                        G_tile_build(i, G_BT_TREE, G_NN_NONE);
        }
}

/******************************************************************************\
 Kick a client via the interface.
\******************************************************************************/
void G_kick_client(n_client_id_t client)
{
        N_send(client, "12ss", G_SM_POPUP, -1,
               "g-host-kicked", "Kicked by host.");
        N_drop_client(client);
}

/******************************************************************************\
 Check to see if a client has lost the game.
\******************************************************************************/
void G_check_loss(n_client_id_t client)
{
        if (n_client_id != N_HOST_CLIENT_ID ||
            !N_client_valid(client) ||
            g_clients[client].nation == G_NN_NONE ||
            g_clients[client].ships > 0)
                return;
        g_clients[client].nation = G_NN_NONE;
        N_broadcast("111", G_SM_AFFILIATE, client, G_NN_NONE);
}

/******************************************************************************\
 Host a new game.
\******************************************************************************/
void G_host_game(void)
{
        int i;

        if (n_client_id != N_HOST_CLIENT_ID)
                G_leave_game();
        G_reset_elements();

        /* Start off nation-less */
        I_select_nation(G_NN_NONE);

        /* Maximum number of clients */
        C_var_unlatch(&g_players);
        if (g_players.value.n < 1)
                g_players.value.n = 1;
        if (g_players.value.n > N_CLIENTS_MAX)
                g_players.value.n = N_CLIENTS_MAX;
        I_configure_player_num(g_clients_max = g_players.value.n);

        /* Start the network server */
        if (!N_start_server((n_callback_f)server_callback,
                            (n_callback_f)G_client_callback)) {
                I_popup(NULL, "Failed to start server.");
                I_enter_limbo();
                return;
        }

        /* Generate a new globe */
        C_var_unlatch(&g_globe_subdiv4);
        C_var_unlatch(&g_island_num);
        C_var_unlatch(&g_island_size);
        C_var_unlatch(&g_island_variance);
        C_var_unlatch(&g_name);
        if (g_globe_subdiv4.value.n < 3)
                g_globe_subdiv4.value.n = 3;
        if (g_globe_subdiv4.value.n > 5)
                g_globe_subdiv4.value.n = 5;
        if (g_island_variance.value.f > 1.f)
                g_island_variance.value.f = 1.f;
        if (!C_var_unlatch(&g_globe_seed))
                g_globe_seed.value.n = (int)time(NULL);
        G_generate_globe(g_globe_subdiv4.value.n, g_island_num.value.n,
                         g_island_size.value.n, g_island_variance.value.f);
        initial_buildings();

        /* Set our name */
        C_var_unlatch(&g_name);
        C_sanitize(g_name.value.s);
        C_strncpy_buf(g_clients[N_HOST_CLIENT_ID].name, g_name.value.s);

        /* Reinitialize any connected clients */
        for (i = 0; i < N_CLIENTS_MAX; i++) {
                if (!n_clients[i].connected)
                        continue;
                init_client(i);
                I_configure_player(i, g_clients[i].name,
                                   G_nation_to_color(g_clients[i].nation),
                                   TRUE);
        }

        /* Tell remote clients that we rehosted */
        N_broadcast_except(N_HOST_CLIENT_ID, "12ss", G_SM_POPUP, -1,
                           "g-host-rehost", "Host started a new game.");

        I_leave_limbo();
        I_popup(NULL, "Hosted a new game.");

        /* Finished initialization */
        g_host_inited = TRUE;
}

/******************************************************************************\
 Called to update server-side structures. Does nothing if not hosting.
\******************************************************************************/
void G_update_host(void)
{
        if (n_client_id != N_HOST_CLIENT_ID || i_limbo)
                return;
        N_poll_server();

        /* Spawn crates for the players */
        while (g_gibs < CRATES_MAX) {
                g_cost_t *loot;
                int tile;

                tile = G_tile_gib(-1, G_GT_CRATES);
                if (tile < 0)
                        break;

                /* Put some loot in the crate */
                loot = &g_tiles[tile].gib->loot;
                loot->cargo[G_CT_GOLD] = 10 * C_roll_dice(5, 15) - 250;
                loot->cargo[G_CT_CREW] = C_roll_dice(3, 3) - 3;
                loot->cargo[G_CT_RATIONS] = C_roll_dice(4, 4) - 4;
                loot->cargo[G_CT_WOOD] = C_roll_dice(5, 10) - 15;
                loot->cargo[G_CT_IRON] = C_roll_dice(5, 10) - 25;
        }
}

