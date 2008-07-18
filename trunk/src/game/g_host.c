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
static void server_affiliate(int client)
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
        }

        N_send(N_BROADCAST_ID, "1112", G_SM_AFFILIATE, client, nation, tile);
}

/******************************************************************************\
 Handles orders to move ships.
\******************************************************************************/
static void server_ship_move(int client)
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
        R_select_path(g_ships[ship].tile, g_ships[ship].path);

        /* TODO: Tell all other clients about the path change */
}

/******************************************************************************\
 Called from within the network namespace when a network event arrives for the
 server from one of the clients (including the server's client).
\******************************************************************************/
static void server_callback(int client, n_event_t event)
{
        g_client_msg_t token;

        /* Connect/disconnect */
        if (event == N_EV_CONNECTED) {
                C_debug("Client %d connected", client);
                g_clients[client].nation = G_NN_NONE;
                N_send(client, "114", G_SM_INIT, G_PROTOCOL,
                       g_globe_seed.value.n);
                return;
        } else if (event == N_EV_DISCONNECTED) {
                C_debug("Client %d disconnected", client);
                return;
        }

        /* Handle messages */
        if (event != N_EV_MESSAGE)
                return;
        token = (g_client_msg_t)N_receive_char();
        switch (token) {
        case G_CM_AFFILIATE:
                server_affiliate(client);
                break;
        case G_CM_SHIP_MOVE:
                server_ship_move(client);
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
        /* Clear game structures */
        memset(g_ships, 0, sizeof (g_ships));
        memset(g_clients, 0, sizeof (g_clients));

        /* Generate a new globe */
        if (g_globe_seed.has_latched)
                C_var_unlatch(&g_globe_seed);
        else
                g_globe_seed.value.n = (int)time(NULL);
        G_generate_globe();

        if (!N_start_server((n_callback_f)server_callback,
                            (n_callback_f)G_client_callback))
                return;
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

