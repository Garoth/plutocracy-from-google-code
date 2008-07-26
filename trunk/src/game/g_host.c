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

/******************************************************************************\
 If a client has sent garbage data, call this to kick them out.
\******************************************************************************/
static void corrupt_kick(int client)
{
        N_send(client, "12s", G_SM_POPUP, -1,
               "Kicked for sending invalid data.");
        N_kick_client(client);
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
        if (old == G_NN_NONE &&
            (ship = G_spawn_ship(client, -1, G_SN_SLOOP, -1)) >= 0) {
                tile = g_ships[ship].tile;
                g_ships[ship].cargo.amounts[G_CT_GOLD] = 1000;
                g_ships[ship].cargo.amounts[G_CT_CREW] = 10;
                g_ships[ship].cargo.amounts[G_CT_RATIONS] = 10;

                /* Spawn a second ship for testing */
                if ((ship = G_spawn_ship(client, tile, G_SN_SPIDER, -1)) >= 0) {
                        g_ships[ship].cargo.amounts[G_CT_GOLD] = 500;
                        g_ships[ship].cargo.amounts[G_CT_CREW] = 15;
                        g_ships[ship].cargo.amounts[G_CT_RATIONS] = 20;
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
        int i;
        char new_name[G_NAME_MAX];

        N_receive_string_buf(new_name);

        /* Didn't actually change names */
        if (!strcmp(new_name, g_clients[client].name))
                return;

        /* Is this a valid name? */
        if (!new_name[0]) {
                N_send(client, "12s", G_SM_POPUP, -1, "Invalid name.");
                return;
        }

        /* See if this name is taken */
        for (i = 0; i < N_CLIENTS_MAX; i++) {
                if (i == client)
                        continue;
                if (!strcasecmp(new_name, g_clients[client].name)) {
                        N_send(client, "12s", G_SM_POPUP, -1,
                               C_va("Name '%s' already taken.", new_name));
                        return;
                }
        }

        C_debug("Client '%s' (%d) renamed to '%s'", g_clients[client].name,
                client, new_name);
        N_broadcast("11s", G_SM_NAME, client, new_name);
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
        if (!chat_buffer[0])
                return;
        N_broadcast_except(client, "11s", G_SM_CHAT, client, chat_buffer);
}

/******************************************************************************\
 Initialize a new client.
\******************************************************************************/
static void client_connected(int client)
{
        int i;

        if (client == N_HOST_CLIENT_ID)
                return;
        C_debug("Initializing client %d", client);
        g_clients[client].nation = G_NN_NONE;
        N_send(client, "1114f", G_SM_INIT, G_PROTOCOL, client,
               g_globe_seed.value.n, r_solar_angle);

        /* Start out nameless */
        g_clients[client].name[0] = NUL;

        /* Tell them about everyone already here */
        for (i = 0; i < N_CLIENTS_MAX; i++)
                if (n_clients[i].connected && g_clients[i].name[0])
                        N_send(client, "111s", G_SM_CLIENT, i,
                               g_clients[i].nation, g_clients[i].name);

        /* Tell everyone else about the new arrival */
        N_broadcast_except(client, "11", G_SM_CONNECTED, client);

        /* Tell them about all the ships on the globe */
        for (i = 0; i < G_SHIPS_MAX; i++)
                if (g_ships[i].in_use)
                        N_send(client, "11211", G_SM_SPAWN_SHIP,
                               g_ships[i].client, g_ships[i].tile,
                               g_ships[i].class_name, i);
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
                client_connected(client);
                return;
        } else if (event == N_EV_DISCONNECTED) {
                if (g_clients[client].name[0])
                        N_broadcast("11", G_SM_DISCONNECTED, client);
                C_debug("Client %d disconnected", client);
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
 Host a new game.
\******************************************************************************/
void G_host_game(void)
{
        G_leave_game();

        /* Clear game structures */
        memset(g_ships, 0, sizeof (g_ships));
        memset(g_clients, 0, sizeof (g_clients));

        /* Start the network server */
        if (!N_start_server((n_callback_f)server_callback,
                            (n_callback_f)G_client_callback)) {
                I_popup(NULL, "Failed to start server.");
                I_enter_limbo();
                return;
        }

        /* Generate a new globe */
        if (g_globe_seed.has_latched)
                C_var_unlatch(&g_globe_seed);
        else
                g_globe_seed.value.n = (int)time(NULL);
        G_generate_globe();

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

