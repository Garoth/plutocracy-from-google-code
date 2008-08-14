/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Deals messages received from the server */

#include "g_common.h"

/* Array of connected clients */
g_client_t g_clients[N_CLIENTS_MAX + 1];

/******************************************************************************\
 Client is receving a popup message from the server.
\******************************************************************************/
static void sm_popup(void)
{
        c_vec3_t *goto_pos;
        int focus_tile;
        char token[32], message[256];

        focus_tile = N_receive_short();
        if (focus_tile >= r_tiles) {
                G_corrupt_disconnect();
                return;
        }
        goto_pos = focus_tile >= 0 ? &g_tiles[focus_tile].origin : NULL;
        N_receive_string(token, sizeof (token));
        N_receive_string(message, sizeof (message));
        I_popup(goto_pos, C_str(token, message));
}

/******************************************************************************\
 Some client changed their national affiliation.
\******************************************************************************/
static void sm_affiliate(void)
{
        c_vec3_t *goto_pos;
        const char *fmt;
        int client, nation, tile;

        client = N_receive_char();
        nation = N_receive_char();
        tile = N_receive_short();
        if (nation < 0 || !N_client_valid(client)) {
                G_corrupt_disconnect();
                return;
        }
        g_clients[client].nation = nation;
        if (client == n_client_id)
                I_select_nation(nation);

        /* Reselect the ship if its client's nation changed */
        G_ship_reselect(-1, client);

        /* Update players window */
        I_configure_player(client, g_clients[client].name,
                           G_nation_to_color(g_clients[client].nation),
                           n_client_id == N_HOST_CLIENT_ID);

        /* Affiliation message might have a goto tile attached */
        goto_pos = NULL;
        if (tile >= 0 && tile < r_tiles)
                goto_pos = &g_tiles[tile].origin;

        /* Special case for pirates */
        if (nation == G_NN_PIRATE) {
                if (client == n_client_id) {
                        I_popup(goto_pos, C_str("g-join-pirate",
                                                "You became a pirate."));
                        return;
                }
                fmt = C_str("g-join-pirate-client", "%s became a pirate.");
                I_popup(goto_pos, C_va(fmt, g_clients[client].name));
                return;
        }

        /* Message for nations */
        if (client == n_client_id) {
                fmt = C_str("g-join-nation-you", "You joined the %s nation.");
                I_popup(goto_pos, C_va(fmt, g_nations[nation].long_name));
        } else {
                fmt = C_str("g-join-nation-client", "%s joined the %s nation.");
                I_popup(goto_pos, C_va(fmt, g_clients[client].name,
                                       g_nations[nation].long_name));
        }
}

/******************************************************************************\
 When a client connects, it gets information about current clients using this
 message type.
\******************************************************************************/
static void sm_client(void)
{
        n_client_id_t client;

        client = N_receive_char();
        if (client < 0 || client >= N_CLIENTS_MAX) {
                G_corrupt_disconnect();
                return;
        }
        n_clients[client].connected = TRUE;
        g_clients[client].nation = N_receive_char();
        N_receive_string_buf(g_clients[client].name);
        C_debug("Client %d is '%s'", client, g_clients[client].name);

        /* Update players window */
        I_configure_player(client, g_clients[client].name,
                           G_nation_to_color(g_clients[client].nation),
                           n_client_id == N_HOST_CLIENT_ID);
}

/******************************************************************************\
 Client just connected to the server or the server has re-hosted.
\******************************************************************************/
static void sm_init(void)
{
        float variance;
        int protocol, subdiv4, islands, island_size;

        C_assert(n_client_id != N_HOST_CLIENT_ID);
        G_reset_elements();

        /* Start off nation-less */
        I_select_nation(G_NN_NONE);

        /* Check the server's protocol */
        if ((protocol = N_receive_short()) != G_PROTOCOL) {
                C_warning("Server protocol (%d) not equal to client (%d)",
                          protocol, G_PROTOCOL);
                I_popup(NULL, C_str("g-incompatible",
                                    "Server protocol is not compatible"));
                N_disconnect();
                return;
        }

        /* Get the client ID */
        n_client_id = N_receive_char();
        if (n_client_id < 0 || n_client_id >= N_CLIENTS_MAX) {
                G_corrupt_disconnect();
                return;
        }
        n_clients[n_client_id].connected = TRUE;

        /* Maximum number of clients */
        g_clients_max = N_receive_char();
        if (g_clients_max < 1 || g_clients_max > N_CLIENTS_MAX) {
                G_corrupt_disconnect();
                return;
        }
        I_configure_player_num(g_clients_max);
        C_debug("Client ID %d of %d", n_client_id, g_clients_max);

        /* Generate matching globe */
        subdiv4 = N_receive_char();
        g_globe_seed.value.n = N_receive_int();
        islands = N_receive_short();
        island_size = N_receive_short();
        variance = N_receive_float();
        G_generate_globe(subdiv4, islands, island_size, variance);

        /* Get solar angle */
        r_solar_angle = N_receive_float();

        I_leave_limbo();
}

/******************************************************************************\
 A client has changed their name.
\******************************************************************************/
static void sm_name(void)
{
        int client;
        char old_name[G_NAME_MAX];

        if ((client = G_receive_cargo(-1)) < 0)
                return;
        C_strncpy_buf(old_name, g_clients[client].name);
        N_receive_string_buf(g_clients[client].name);

        /* Update players window */
        I_configure_player(client, g_clients[client].name,
                           G_nation_to_color(g_clients[client].nation),
                           n_client_id == N_HOST_CLIENT_ID);

        /* The first name-change on connect */
        if (!old_name[0])
                I_print_chat(C_va("%s joined the game.",
                                  g_clients[client].name), I_COLOR, NULL);
        else
                I_print_chat(C_va("%s renamed to %s.", old_name,
                                  g_clients[client].name), I_COLOR, NULL);
}

/******************************************************************************\
 Another client sent out a chat message.
\******************************************************************************/
static void sm_chat(void)
{
        i_color_t color;
        int client;
        char buffer[N_SYNC_MAX], *name;

        client = N_receive_char();
        N_receive_string_buf(buffer);
        if (!buffer[0])
                return;

        /* Normal chat message */
        if (client >= 0 && client < N_CLIENTS_MAX) {
                color = G_nation_to_color(g_clients[client].nation);
                name = g_clients[client].name;
                I_print_chat(name, color, buffer);
                return;
        }

        /* No-text message */
        I_print_chat(buffer, I_COLOR, NULL);
}

/******************************************************************************\
 A ship spawned on the server.
\******************************************************************************/
static void sm_ship_spawn(void)
{
        g_ship_type_t type;
        n_client_id_t client;
        int tile, index;

        index = N_receive_char();
        client = N_receive_char();
        tile = N_receive_short();
        type = N_receive_char();
        if (G_ship_spawn(index, client, tile, type) < 0)
                G_corrupt_disconnect();
}

/******************************************************************************\
 Receive a ship path.
\******************************************************************************/
static void sm_ship_move(void)
{
        int index, tile, target;

        if ((index = G_receive_ship(-1)) < 0)
                return;
        tile = N_receive_short();
        target = N_receive_short();

        /* Don't re-path unless something changed */
        if (G_ship_move_to(index, tile) || target != g_ships[index].target)
                G_ship_path(index, target);
}

/******************************************************************************\
 A ship's cargo manifest changed.
\******************************************************************************/
static void sm_ship_cargo(void)
{
        int i, index;

        if ((index = G_receive_ship(-1)) < 0)
                return;

        /* If we are hosting we already know all the information */
        if (n_client_id == N_HOST_CLIENT_ID) {
                G_ship_reselect(index, -1);
                return;
        }

        for (i = 0; i < G_CARGO_TYPES; i++) {
                g_cargo_t *cargo;

                cargo = g_ships[index].store.cargo + i;
                cargo->amount = N_receive_short();

                /* Clients determine the prices for their own ships */
                if (g_ships[index].client == n_client_id) {
                        N_receive_short();
                        N_receive_short();
                        N_receive_short();
                        N_receive_short();
                } else {
                        cargo->buy_price = N_receive_short();
                        cargo->sell_price = N_receive_short();
                        cargo->auto_buy = cargo->buy_price >= 0;
                        cargo->auto_sell = cargo->sell_price >= 0;
                        cargo->minimum = N_receive_short();
                        cargo->maximum = N_receive_short();
                }
        }
        G_store_space(&g_ships[index].store);
        G_ship_reselect(index, -1);
}

/******************************************************************************\
 The buy/sell prices of a cargo item have changed on some ship.
\******************************************************************************/
static void sm_ship_prices(void)
{
        g_cargo_t *cargo;
        int ship_i, cargo_i, buy_price, sell_price;

        ship_i = G_receive_ship(-1);
        cargo_i = G_receive_cargo(-1);
        if (ship_i < 0 || cargo_i < 0)
                return;

        /* Save prices */
        buy_price = N_receive_short();
        sell_price = N_receive_short();
        cargo = g_ships[ship_i].store.cargo + cargo_i;
        if ((cargo->auto_buy = buy_price >= 0))
                cargo->buy_price = buy_price;
        if ((cargo->auto_sell = sell_price >= 0))
                cargo->sell_price = sell_price;

        /* Save quantities */
        cargo->minimum = N_receive_short();
        cargo->maximum = N_receive_short();

        /* Update trade window */
        G_ship_reselect(-1, ship_i);
        if (g_tiles[g_ships[g_selected_ship].trade_tile].ship == ship_i)
                G_ship_reselect(-1, -1);
}

/******************************************************************************\
 Send the server an update if our name changes.
\******************************************************************************/
static int name_update(c_var_t *var, c_var_value_t value)
{
        if (!value.s[0])
                return FALSE;
        C_sanitize(value.s);
        N_send(N_SERVER_ID, "1s", G_CM_NAME, value.s);
        return TRUE;
}

/******************************************************************************\
 Client network event callback function.
\******************************************************************************/
void G_client_callback(int client, n_event_t event)
{
        g_server_msg_t token;
        int i, j;

        C_assert(client == N_SERVER_ID);

        /* Connected */
        if (event == N_EV_CONNECTED) {
                if (n_client_id == N_HOST_CLIENT_ID)
                        return;

                /* Send out our name */
                name_update(&g_name, g_name.value);
                return;
        }

        /* Disconnected */
        if (event == N_EV_DISCONNECTED) {
                I_enter_limbo();
                if (n_client_id != N_HOST_CLIENT_ID)
                        I_popup(NULL, "Disconnected from server.");
                return;
        }

        /* Process messages */
        if (event != N_EV_MESSAGE)
                return;
        token = N_receive_char();
        switch (token) {
        case G_SM_POPUP:
                sm_popup();
                break;
        case G_SM_AFFILIATE:
                sm_affiliate();
                break;
        case G_SM_INIT:
                sm_init();
                break;
        case G_SM_NAME:
                sm_name();
                break;
        case G_SM_CHAT:
                sm_chat();
                break;
        case G_SM_CLIENT:
                sm_client();
                break;
        case G_SM_SHIP_SPAWN:
                sm_ship_spawn();
                break;
        case G_SM_SHIP_MOVE:
                sm_ship_move();
                break;
        case G_SM_SHIP_CARGO:
                sm_ship_cargo();
                break;
        case G_SM_SHIP_PRICES:
                sm_ship_prices();
                break;

        /* Ship changed names */
        case G_SM_SHIP_NAME:
                if ((i = G_receive_ship(-1)) < 0)
                        return;
                N_receive_string_buf(g_ships[i].name);
                G_count_name(G_NT_SHIP, g_ships[i].name);
                G_ship_reselect(i, -1);
                break;

        /* Ship changed owners */
        case G_SM_SHIP_OWNER:
                if ((i = G_receive_ship(-1)) < 0 ||
                    (j = G_receive_cargo(-1)) < 0)
                        return;
                g_ships[i].client = j;
                G_ship_reselect(i, -1);
                break;

        /* Building changed */
        case G_SM_BUILDING:
                if ((i = G_receive_tile(-1)) < 0 ||
                    (j = G_receive_range(-1, 0, G_BUILDING_TYPES)) < 0)
                        return;
                G_build(i, j, N_receive_float());
                break;

        /* Somebody connected but we don't have their name yet */
        case G_SM_CONNECTED:
                if ((i = G_receive_range(-1, 0, N_CLIENTS_MAX)) < 0)
                        return;
                n_clients[i].connected = TRUE;
                C_zero(g_clients + i);
                C_debug("Client %d connected", i);
                break;

        /* Somebody disconnected */
        case G_SM_DISCONNECTED:
                i = N_receive_char();
                if (i < 0 || i >= N_CLIENTS_MAX) {
                        G_corrupt_disconnect();
                        return;
                }
                n_clients[i].connected = FALSE;
                I_print_chat(C_va("%s left the game.", g_clients[i].name),
                             I_COLOR, NULL);
                I_configure_player(i, NULL, I_COLOR, FALSE);
                C_debug("Client %d disconnected", i);
                break;

        default:
                break;
        }
}

/******************************************************************************\
 Called to update client-side structures.
\******************************************************************************/
void G_update_client(void)
{
        N_poll_client();
        if (i_limbo)
                return;
        G_update_ships();
}

/******************************************************************************\
 Initialize game structures.
\******************************************************************************/
void G_init(void)
{
        C_status("Initializing client");
        G_init_elements();
        G_init_globe();

        /* Update the server when our name changes */
        C_var_unlatch(&g_name);
        g_name.update = (c_var_update_f)name_update;
        g_name.edit = C_VE_FUNCTION;

        /* Parse names config */
        G_load_names();
}

/******************************************************************************\
 Cleanup game structures.
\******************************************************************************/
void G_cleanup(void)
{
        G_cleanup_globe();
}

