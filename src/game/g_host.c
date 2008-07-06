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

/* Array of connected clients */
g_client_t g_clients[N_CLIENTS_MAX];

/* Array of game nations */
g_nation_t g_nations[G_NATION_NAMES];

/* Ships and ship classes */
g_ship_t g_ships[G_SHIPS_MAX];
g_ship_class_t g_ship_classes[G_SHIP_NAMES];

/******************************************************************************\
 Initialize a client structure.
\******************************************************************************/
static void client_init(int client)
{
        g_clients[client].nation = G_NN_NONE;
}

/******************************************************************************\
 Initialize game structures before play.
\******************************************************************************/
void G_init(void)
{
        int i;

        C_status("Initializing game elements");

        /* Setup nations */
        g_nations[G_NN_RED].short_name = "red";
        g_nations[G_NN_RED].long_name = C_str("g-nation-red", "Ruby");
        g_nations[G_NN_GREEN].short_name = "green";
        g_nations[G_NN_GREEN].long_name = C_str("g-nation-green", "Emerald");
        g_nations[G_NN_BLUE].short_name = "blue";
        g_nations[G_NN_BLUE].long_name = C_str("g-nation-blue", "Sapphire");
        g_nations[G_NN_PIRATE].short_name = "pirate";
        g_nations[G_NN_PIRATE].long_name = C_str("g-nation-pirate", "Pirate");

        /* Setup ship classes */
        g_ship_classes[G_SN_SLOOP].name = C_str("g-ship-sloop", "Sloop");
        g_ship_classes[G_SN_SLOOP].speed = 1.f;
        g_ship_classes[G_SN_SLOOP].model_path = "models/ship/sloop.plum";

        /* Setup ships array */
        for (i = 0; i < G_SHIPS_MAX; i++)
                g_ships[i].ship_class = G_SN_NONE;

        /* Prepare initial state */
        G_init_globe();
}

/******************************************************************************\
 Cleanup game structures.
\******************************************************************************/
void G_cleanup(void)
{
        G_cleanup_globe();
}

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
 Called from within the network namespace when a network event arrives for the
 server from one of the clients (including the server's client).
\******************************************************************************/
static void server_callback(int client, n_event_t event)
{
        g_client_msg_t token;
        int i;

        /* Connect/disconnect */
        if (event == N_EV_CONNECTED) {
                C_debug("Client %d connected", client);
                client_init(client);
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
        default:
                break;

        /* Client wants to change to another nation */
        case G_CM_AFFILIATE:
                i = N_receive_char();
                if (i < 0 || i >= G_NATION_NAMES) {
                        corrupt_kick(client);
                        return;
                }
                if (i == g_clients[client].nation)
                        return;
                g_clients[client].nation = i;
                N_send(N_BROADCAST_ID, "111", G_SM_AFFILIATE, client, i);
                break;
        }
}

/******************************************************************************\
 Host a new game.
\******************************************************************************/
void G_host_game(void)
{
        g_globe_seed.value.n = (int)time(NULL);
        G_generate_globe();
        N_start_server((n_callback_f)server_callback,
                       (n_callback_f)G_client_callback);
        I_leave_limbo();
        I_popup(NULL, "Hosted a new game.");
}

/******************************************************************************\
 Updates the game for this frame.
\******************************************************************************/
void G_update(void)
{
        if (n_client_id != N_HOST_CLIENT_ID)
                return;
        N_poll_server();
}

