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

/* The client's index */
int g_client_id;

/******************************************************************************\
 Disconnect if the server has sent corrupted data.
\******************************************************************************/
static void corrupt_disconnect(void)
{
        I_popup(I_PI_NONE, "Server sent invalid data, disconnected.", NULL);
        N_disconnect();
}

/******************************************************************************\
 Client is receving a popup message from the server.
\******************************************************************************/
static void receive_popup(void)
{
        i_popup_icon_t icon;
        c_vec3_t *goto_pos;
        int focus_tile;
        char message[256];

        icon = (i_popup_icon_t)N_receive_char();
        focus_tile = N_receive_short();
        if (icon < 0 || focus_tile >= r_tiles) {
                corrupt_disconnect();
                return;
        }
        goto_pos = focus_tile >= 0 ? &g_tiles[focus_tile].origin : NULL;
        N_receive_string(message, sizeof (message));
        I_popup(icon, message, goto_pos);
}

/******************************************************************************\
 Some client changed their national affiliation.
\******************************************************************************/
static void receive_affiliate(void)
{
        const char *format, *nation_name;
        int client, nation;

        client = N_receive_char();
        nation = N_receive_char();
        if (nation < 0 || nation >= G_NATIONS || client < 0 ||
            client >= N_CLIENTS_MAX || !n_clients[client].connected) {
                corrupt_disconnect();
                return;
        }
        g_clients[client].nation = nation;
        if (client == n_client_id)
                I_select_nation(nation);
        nation_name = C_str(C_va("g-nation-%s", g_nations[nation].short_name),
                            g_nations[nation].long_name);

        /* Special case for pirates */
        if (nation == G_NATION_PIRATE) {
                if (client == n_client_id) {
                        I_popup(I_PI_NONE, C_str("g-join-pirate",
                                                 "You became a pirate."), NULL);
                        return;
                }
                format = C_str("g-join-pirate-client", "%s became a pirate.");
                I_popup(I_PI_NONE, C_va(format, g_clients[client].name), NULL);
                return;
        }

        /* Message for nations */
        if (client == n_client_id) {
                format = C_str("g-join-nation-you",
                               "You joined the %s nation.");
                I_popup(I_PI_NONE, C_va(format, nation_name), NULL);
        } else {
                format = C_str("g-join-nation-client",
                               "%s joined the %s nation.");
                I_popup(I_PI_NONE, C_va(format, g_clients[client].name,
                                        nation_name), NULL);
        }
}

/******************************************************************************\
 Client network event callback function.
\******************************************************************************/
void G_client_callback(int client, n_event_t event)
{
        g_server_msg_t token;

        C_assert(client == N_SERVER_ID);

        /* Disconnected */
        if (event == N_EV_DISCONNECTED) {
                I_enter_limbo();
                return;
        }

        /* Process messages */
        if (event != N_EV_MESSAGE)
                return;
        token = N_receive_char();
        switch (token) {
        default:
                break;
        case G_SM_POPUP:
                receive_popup();
                break;
        case G_SM_AFFILIATE:
                receive_affiliate();
                break;
        }
}

/******************************************************************************\
 Leave the current game.
\******************************************************************************/
void G_quit_game(void)
{
        N_disconnect();
        N_stop_server();
        I_popup(I_PI_NONE, "Left the current game.", NULL);
}

/******************************************************************************\
 Change the player's nation.
\******************************************************************************/
void G_change_nation(int index)
{
        N_send(N_SERVER_ID, "11", G_CM_AFFILIATE, index);
}

