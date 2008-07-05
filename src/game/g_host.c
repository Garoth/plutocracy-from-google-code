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
g_nation_t g_nations[G_NATIONS];

/******************************************************************************\
 If a client has sent garbage data, call this to kick them out.
\******************************************************************************/
static void corrupt_kick(int client)
{
        N_send(client, "112s", G_SM_POPUP, I_PI_NONE, -1,
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
                if (i < 0 || i >= G_NATIONS) {
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
        int i;

        g_globe_seed.value.n = (int)time(NULL);
        G_generate_globe();

        /* Stock nation names */
        g_nations[0].short_name = "red";
        g_nations[0].long_name = "Ruby";
        g_nations[1].short_name = "green";
        g_nations[1].long_name = "Emerald";
        g_nations[2].short_name = "blue";
        g_nations[2].long_name = "Sapphire";
        g_nations[G_NATION_PIRATE].short_name = "pirate";
        g_nations[G_NATION_PIRATE].long_name = "Pirate";

        /* Setup interface nation buttons */
        I_reset_nations();
        for (i = 0; i < G_NATIONS; i++)
                I_add_nation(g_nations[i].short_name, g_nations[i].long_name,
                             i == G_NATION_PIRATE);
        I_configure_nations();

        N_start_server((n_callback_f)server_callback,
                       (n_callback_f)G_client_callback);
        I_popup(I_PI_NONE, "Hosted a new game.", NULL);
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

