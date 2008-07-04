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

/******************************************************************************\
 Called from within the network namespace when a message arrives for the
 server from one of the clients (including the server's client).
\******************************************************************************/
static void receive_server(int client)
{
        g_client_msg_t token;

        /* Discard messages from invalid clients */
        if (client < 0 || client >= N_CLIENTS_MAX)
                return;

        token = (g_client_msg_t)N_receive_char();
        switch (token) {
        case G_CM_AFFILIATE:
                g_clients[client].nation = N_receive_char();
                if (g_clients[client].nation < 0 ||
                    g_clients[client].nation > 4)
                        g_clients[client].nation = 0;
                return;
        default:
                return;
        }
}

/******************************************************************************\
 Host a new game.
\******************************************************************************/
void G_host_game(void)
{
        g_globe_seed.value.n = (int)time(NULL);
        G_generate_globe();
        N_start_server((n_receive_f)receive_server);
        I_popup(I_PI_NONE, "Hosted a new game.", NULL);
}

