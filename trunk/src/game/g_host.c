/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Initializes the game structures. Primarily handles client messages for the
   server. Remember that the client is not trusted and all input must be
   checked for validity! */

#include "g_common.h"

int g_clients_max;

/******************************************************************************\
 Send updated cargo information to clients. If [client] is negative, all
 clients that can see the ship's cargo are updated.
\******************************************************************************/
static void send_ship_cargo(int client, int index)
{
        int i;

        /* Host already knows everything */
        if (client == N_HOST_CLIENT_ID)
                return;

        /* First send accurate cargo information to the owner */
        if (client == g_ships[index].client || client < 0) {
                N_send_start();
                N_send_char(G_SM_SHIP_CARGO);
                N_send_char(index);
                for (i = 0; i < G_CARGO_TYPES; i++)
                        N_send_short(g_ships[index].store.cargo[i].amount);
                N_send(g_ships[index].client, NULL);
                if (client == g_ships[index].client)
                        return;
        }

        /* Select all clients that can see this ship */
        for (i = 0; i < N_CLIENTS_MAX; i++)
                n_clients[i].selected = g_ships[index].store.visible[i];
        n_clients[g_ships[index].client].selected = FALSE;

        /* Send out information about someone else's store */
        N_send_start();
        N_send_char(G_SM_SHIP_CARGO);
        N_send_char(index);
        for (i = 0; i < G_CARGO_TYPES; i++) {
                g_cargo_t *cargo;

                cargo = g_ships[index].store.cargo + i;

                /* Buying amount */
                N_send_short(cargo->maximum - cargo->amount);

                /* Selling amount */
                N_send_short(cargo->amount - cargo->minimum);

                /* Buy price */
                N_send_short(cargo->auto_buy ? cargo->buy_price : -1);

                /* Sell price */
                N_send_short(cargo->auto_sell ? cargo->sell_price : -1);
        }
        if (client < 0)
                N_send_selected(NULL);
        else
                N_send(client, NULL);
}

/******************************************************************************\
 If a client has sent garbage data, call this to kick them out.
\******************************************************************************/
static void corrupt_kick(int client)
{
        N_send(client, "12ss", G_SM_POPUP, -1,
               "g-host-invalid", "Your client sent invalid data.");
        N_drop_client(client);
}

/******************************************************************************\
 Handles clients changing nations.
\******************************************************************************/
static void cm_affiliate(int client)
{
        int nation, old, tile, ship;

        nation = N_receive_char();
        if (nation < 0 || nation >= G_NATION_NAMES) {
                corrupt_kick(client);
                return;
        }
        if (nation == g_clients[client].nation)
                return;
        old = g_clients[client].nation;
        g_clients[client].nation = nation;

        /* If this client just joined a nation for the first time,
           try to give them a starter ship */
        tile = -1;
        ship = G_spawn_ship(client, -1, G_SN_SLOOP, -1);
        if (old == G_NN_NONE && ship >= 0) {
                tile = g_ships[ship].tile;
                g_ships[ship].store.cargo[G_CT_GOLD].amount = 500;
                g_ships[ship].store.cargo[G_CT_CREW].amount = 20;
                g_ships[ship].store.cargo[G_CT_RATIONS].amount = 50;
                send_ship_cargo(-1, ship);

                /* Spawn a second ship for testing */
                ship = G_spawn_ship(client, tile, G_SN_SPIDER, -1);
                if (ship >= 0) {
                        send_ship_cargo(N_BROADCAST_ID, ship);
                        g_ships[ship].store.cargo[G_CT_GOLD].amount = 1000;
                        g_ships[ship].store.cargo[G_CT_CREW].amount = 30;
                        g_ships[ship].store.cargo[G_CT_RATIONS].amount = 70;
                        send_ship_cargo(-1, ship);
                }

                /* And spawn a third ship for testing also */
                ship = G_spawn_ship(client, tile, G_SN_GALLEON, -1);
                if (ship >= 0) {
                        send_ship_cargo(N_BROADCAST_ID, ship);
                        g_ships[ship].store.cargo[G_CT_GOLD].amount = 2000;
                        g_ships[ship].store.cargo[G_CT_CREW].amount = 50;
                        g_ships[ship].store.cargo[G_CT_RATIONS].amount = 100;
                        send_ship_cargo(-1, ship);
                }
        }

        N_broadcast("1112", G_SM_AFFILIATE, client, nation, tile);
}

/******************************************************************************\
 Handles orders to move ships.
\******************************************************************************/
static void cm_ship_move(int client)
{
        int ship, tile;

        ship = N_receive_char();
        tile = N_receive_short();
        if (ship < 0 || ship >= G_SHIPS_MAX || tile < 0 || tile >= r_tiles) {
                corrupt_kick(client);
                return;
        }
        if (!g_ships[ship].in_use || g_ships[ship].client != client)
                return;
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
static void cm_name_ship(int client)
{
        int index;
        char new_name[G_NAME_MAX];

        index = N_receive_char();
        if (index < 0 || index >= G_SHIPS_MAX) {
                corrupt_kick(client);
                return;
        }
        if (g_ships[index].client != client)
                return;
        N_receive_string_buf(new_name);
        C_sanitize(new_name);
        if (!new_name[0])
                return;
        C_strncpy_buf(g_ships[index].name, new_name);
        N_broadcast_except(client, "11s", G_SM_NAME_SHIP, index,
                           g_ships[index].name);
        C_debug("'%s' named ship %d '%s'", g_clients[client].name,
                index, new_name);
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
        N_send(client, "11111422ff", G_SM_INIT, G_PROTOCOL, client,
               g_clients_max, g_globe_subdiv4.value.n, g_globe_seed.value.n,
               g_islands.value.n, g_island_size.value.n,
               g_island_variance.value.f, r_solar_angle);

        /* Tell them about everyone already here */
        for (i = 0; i < N_CLIENTS_MAX; i++)
                if (n_clients[i].connected && g_clients[i].name[0])
                        N_send(client, "111s", G_SM_CLIENT, i,
                               g_clients[i].nation, g_clients[i].name);

        /* Tell them about the buildings on them globe */
        for (i = 0; i < r_tiles; i++)
                if (g_tiles[i].building != G_BN_NONE)
                        N_send(client, "121f", G_SM_BUILDING, i,
                               g_tiles[i].building, g_tiles[i].progress);

        /* Tell them about all the ships on the globe */
        for (i = 0; i < G_SHIPS_MAX; i++) {
                if (!g_ships[i].in_use)
                        continue;

                /* Owner, tile, class, index */
                N_send(client, "11211", G_SM_SPAWN_SHIP, g_ships[i].client,
                       g_ships[i].tile, g_ships[i].class_name, i);

                /* Name */
                N_send(client, "11s", G_SM_NAME_SHIP, i, g_ships[i].name);

                /* Cargo */
                send_ship_cargo(client, i);

                /* Movement */
                if (g_ships[i].target != g_ships[i].tile)
                        N_send(client, "1122", G_SM_SHIP_MOVE,
                               i, g_ships[i].tile, g_ships[i].target);
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
                if (g_ships[i].client == client) {
                        g_ships[i].client = N_SERVER_ID;
                        N_broadcast_except(N_HOST_CLIENT_ID, "111",
                                           G_SM_SHIP_OWNER, i, N_SERVER_ID);
                        G_ship_reselect(i, -1);
                }
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
        case G_CM_SHIP_MOVE:
                cm_ship_move(client);
                break;
        case G_CM_NAME:
                cm_name(client);
                break;
        case G_CM_NAME_SHIP:
                cm_name_ship(client);
                break;
        case G_CM_CHAT:
                cm_chat(client);
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

        for (i = 0; i < r_tiles; i++) {
                if (R_terrain_base(r_tile_params[i].terrain) != R_T_GROUND)
                        continue;
                if (C_rand_real() < g_forest.value.f)
                        G_build(i, G_BN_TREE, 1.f);
        }
}

/******************************************************************************\
 Kick a client via the interface.
\******************************************************************************/
void G_kick_client(int client)
{
        N_send(client, "12ss", G_SM_POPUP, -1,
               "g-host-kicked", "Kicked by host.");
        N_drop_client(client);
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
        C_var_unlatch(&g_islands);
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
        G_generate_globe(g_globe_subdiv4.value.n, g_islands.value.n,
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
}

/******************************************************************************\
 Called to update server-side structures. Does nothing if not hosting.
\******************************************************************************/
void G_update_host(void)
{
        if (n_client_id != N_HOST_CLIENT_ID || i_limbo)
                return;
        N_poll_server();
}

