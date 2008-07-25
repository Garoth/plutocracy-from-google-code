/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Deals with initialization, cleanup, and messages received from the server */

#include "g_common.h"

/* So we know where the corruption was detected */
#define corrupt_disconnect() corrupt_disc_full(__FILE__, __LINE__, __func__)

/* Array of connected clients */
g_client_t g_clients[N_CLIENTS_MAX];

/* Array of game nations */
g_nation_t g_nations[G_NATION_NAMES];

/* Array of cargo names */
const char *g_cargo_names[G_CARGO_TYPES];

/******************************************************************************\
 Update function for a national color.
\******************************************************************************/
static int nation_color_update(c_var_t *var, c_var_value_t value)
{
        C_color_update(var, value);
        I_update_colors();
        return TRUE;
}

/******************************************************************************\
 Send the server an update if our name changes.
\******************************************************************************/
static int name_update(c_var_t *var, c_var_value_t value)
{
        if (!value.s[0])
                return FALSE;
        N_send(N_SERVER_ID, "1s", G_CM_NAME, value.s);
        return TRUE;
}

/******************************************************************************\
 Initialize game structures before play.
\******************************************************************************/
void G_init(void)
{
        C_status("Initializing game elements");
        G_init_ships();

        /* Setup nations */
        g_nations[G_NN_NONE].short_name = "none";
        g_nations[G_NN_NONE].long_name = C_str("g-nation-none", "None");
        g_nations[G_NN_RED].short_name = "red";
        g_nations[G_NN_RED].long_name = C_str("g-nation-red", "Ruby");
        C_var_update_data(g_nation_colors + G_NN_RED, nation_color_update,
                          &g_nations[G_NN_RED].color);
        g_nations[G_NN_GREEN].short_name = "green";
        g_nations[G_NN_GREEN].long_name = C_str("g-nation-green", "Emerald");
        C_var_update_data(g_nation_colors + G_NN_GREEN, nation_color_update,
                          &g_nations[G_NN_GREEN].color);
        g_nations[G_NN_BLUE].short_name = "blue";
        g_nations[G_NN_BLUE].long_name = C_str("g-nation-blue", "Sapphire");
        C_var_update_data(g_nation_colors + G_NN_BLUE, nation_color_update,
                          &g_nations[G_NN_BLUE].color);
        g_nations[G_NN_PIRATE].short_name = "pirate";
        g_nations[G_NN_PIRATE].long_name = C_str("g-nation-pirate", "Pirate");
        C_var_update_data(g_nation_colors + G_NN_PIRATE, nation_color_update,
                          &g_nations[G_NN_PIRATE].color);

        /* Special cargo */
        g_cargo_names[G_CT_GOLD] = C_str("g-cargo-gold", "Gold");
        g_cargo_names[G_CT_CREW] = C_str("g-cargo-crew", "Crew");

        /* Food cargo */
        g_cargo_names[G_CT_RATIONS] = C_str("g-cargo-rations", "Rations");
        g_cargo_names[G_CT_BREAD] = C_str("g-cargo-bread", "Bread");
        g_cargo_names[G_CT_FISH] = C_str("g-cargo-fish", "Fish");
        g_cargo_names[G_CT_FRUIT] = C_str("g-cargo-fruit", "Fruit");

        /* Raw material cargo */
        g_cargo_names[G_CT_GRAIN] = C_str("g-cargo-grain", "Grain");
        g_cargo_names[G_CT_COTTON] = C_str("g-cargo-cotton", "Cotton");
        g_cargo_names[G_CT_LUMBER] = C_str("g-cargo-lumber", "Lumber");
        g_cargo_names[G_CT_IRON] = C_str("g-cargo-iron", "Iron");

        /* Luxury cargo */
        g_cargo_names[G_CT_RUM] = C_str("g-cargo-rum", "Rum");
        g_cargo_names[G_CT_FURNITURE] = C_str("g-cargo-furniture", "Furniture");
        g_cargo_names[G_CT_CLOTH] = C_str("g-cargo-cloth", "Cloth");
        g_cargo_names[G_CT_SUGAR] = C_str("g-cargo-sugar", "Sugar");

        /* Equipment cargo */
        g_cargo_names[G_CT_CANNON] = C_str("g-cargo-cannon", "Cannon");
        g_cargo_names[G_CT_ROUNDSHOT] = C_str("g-cargo-roundshot", "Roundshot");
        g_cargo_names[G_CT_PLATING] = C_str("g-cargo-plating", "Plating");
        g_cargo_names[G_CT_GILLNET] = C_str("g-cargo-gillner", "Gillnet");

        /* Prepare initial state */
        G_init_globe();

        /* Update the server when our name changes */
        C_var_unlatch(&g_name);
        g_name.update = (c_var_update_f)name_update;
        g_name.edit = C_VE_FUNCTION;
}

/******************************************************************************\
 Cleanup game structures.
\******************************************************************************/
void G_cleanup(void)
{
        G_cleanup_globe();
        G_cleanup_ships();
}

/******************************************************************************\
 Returns the amount of space the given cargo manifest contains.
\******************************************************************************/
int G_cargo_space(const g_cargo_t *cargo)
{
        int i, space;

        space = 0;
        for (i = 0; i < G_CARGO_TYPES; i++)
                space += cargo->amounts[i];

        /* Gold is special */
        space += cargo->amounts[G_CT_GOLD] / 100 - cargo->amounts[G_CT_GOLD];

        return space;
}

/******************************************************************************\
 Disconnect if the server has sent corrupted data.
\******************************************************************************/
static void corrupt_disc_full(const char *file, int line, const char *func)
{
        C_warning_full(file, line, func, "Server sent corrupt data");
        I_popup(NULL, "Server sent invalid data.");
        N_disconnect();
}

/******************************************************************************\
 Client is receving a popup message from the server.
\******************************************************************************/
static void client_popup(void)
{
        c_vec3_t *goto_pos;
        int focus_tile;
        char message[256];

        focus_tile = N_receive_short();
        if (focus_tile >= r_tiles) {
                corrupt_disconnect();
                return;
        }
        goto_pos = focus_tile >= 0 ? &g_tiles[focus_tile].origin : NULL;
        N_receive_string(message, sizeof (message));
        I_popup(goto_pos, message);
}

/******************************************************************************\
 Some client changed their national affiliation.
\******************************************************************************/
static void client_affiliate(void)
{
        c_vec3_t *goto_pos;
        const char *fmt;
        int client, nation, tile;

        client = N_receive_char();
        nation = N_receive_char();
        tile = N_receive_short();
        if (nation < 0 || !N_client_valid(client)) {
                corrupt_disconnect();
                return;
        }
        g_clients[client].nation = nation;
        if (client == n_client_id)
                I_select_nation(nation);

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
static void client_client(void)
{
        n_client_id_t client;

        client = N_receive_char();
        if (client < 0 || client >= N_CLIENTS_MAX) {
                corrupt_disconnect();
                return;
        }
        n_clients[client].connected = TRUE;
        g_clients[client].nation = N_receive_char();
        N_receive_string_buf(g_clients[client].name);
        C_debug("Client %d is '%s'", client, g_clients[client].name);
}

/******************************************************************************\
 Client just connected to the server or the server has re-hosted.
\******************************************************************************/
static void client_init(void)
{
        int protocol, seed;
        const char *msg;

        C_assert(n_client_id != N_HOST_CLIENT_ID);

        /* Check the server's protocol */
        protocol = N_receive_char();
        if (protocol != G_PROTOCOL) {
                msg = C_va("Server protocol (%d) does not match "
                           "client protocol (%d), disconnecting.",
                           protocol, G_PROTOCOL);
                I_popup(NULL, C_str("g-incompatible", msg));
                N_disconnect();
                return;
        }

        /* Get the client ID */
        n_client_id = N_receive_char();
        if (n_client_id < 0 || n_client_id >= N_CLIENTS_MAX) {
                corrupt_disconnect();
                return;
        }
        n_clients[n_client_id].connected = TRUE;

        /* Generate a new globe to match the server's */
        seed = N_receive_int();
        if (seed != g_globe_seed.value.n) {
                g_globe_seed.value.n = seed;
                G_generate_globe();
        }

        /* Get solar angle */
        r_solar_angle = N_receive_float();

        /* Start off nation-less */
        I_select_nation(G_NN_NONE);

        I_leave_limbo();
}

/******************************************************************************\
 A client has changed their name.
\******************************************************************************/
static void client_name(void)
{
        int client;
        char old_name[G_NAME_MAX];

        client = N_receive_char();
        if (!N_client_valid(client)) {
                corrupt_disconnect();
                return;
        }
        C_strncpy_buf(old_name, g_clients[client].name);
        N_receive_string_buf(g_clients[client].name);

        /* Don't say that other players joined if we just joined */
        if (n_client_id == N_UNASSIGNED_ID)
                return;

        /* The first name-change on connect */
        if (!old_name[0]) {
                I_print_chat(C_va("%s joined the game.",
                                  g_clients[client].name), I_COLOR, NULL);
                return;
        }

        I_print_chat(C_va("%s renamed to %s.", old_name,
                          g_clients[client].name), I_COLOR, NULL);
}

/******************************************************************************\
 Convert a nation to a color index.
\******************************************************************************/
i_color_t G_nation_to_color(g_nation_name_t nation)
{
        if (nation == G_NN_RED)
                return I_COLOR_RED;
        else if (nation == G_NN_GREEN)
                return I_COLOR_GREEN;
        else if (nation == G_NN_BLUE)
                return I_COLOR_BLUE;
        else if (nation == G_NN_PIRATE)
                return I_COLOR_PIRATE;
        return I_COLOR_ALT;
}

/******************************************************************************\
 Another client sent out a chat message.
\******************************************************************************/
static void client_chat(void)
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
 Spawn a ship.
\******************************************************************************/
static void client_spawn_ship(void)
{
        g_ship_name_t class_name;
        n_client_id_t client;
        int tile, index;

        client = N_receive_char();
        tile = N_receive_short();
        class_name = N_receive_char();
        index = N_receive_char();
        if (G_spawn_ship(client, tile, class_name, index) < 0)
                corrupt_disconnect();
}

/******************************************************************************\
 Receive a ship path.
\******************************************************************************/
static void client_ship_move(void)
{
        int index, tile, target;

        index = N_receive_char();
        tile = N_receive_short();
        target = N_receive_short();
        if (index < 0 || index >= G_SHIPS_MAX || !g_ships[index].in_use) {
                corrupt_disconnect();
                return;
        }
        G_ship_move_to(index, tile);
        G_ship_path(index, target);
}

/******************************************************************************\
 Client network event callback function.
\******************************************************************************/
void G_client_callback(int client, n_event_t event)
{
        g_server_msg_t token;
        int i;

        C_assert(client == N_SERVER_ID);

        /* Connected */
        if (event == N_EV_CONNECTED) {
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
                client_popup();
                break;
        case G_SM_AFFILIATE:
                client_affiliate();
                break;
        case G_SM_INIT:
                client_init();
                break;
        case G_SM_NAME:
                client_name();
                break;
        case G_SM_CHAT:
                client_chat();
                break;
        case G_SM_CLIENT:
                client_client();
                break;
        case G_SM_SPAWN_SHIP:
                client_spawn_ship();
                break;
        case G_SM_SHIP_MOVE:
                client_ship_move();
                break;
        case G_SM_CONNECTED:
                i = N_receive_char();
                if (i < 0 || i >= N_CLIENTS_MAX) {
                        corrupt_disconnect();
                        return;
                }
                n_clients[i].connected = TRUE;
                C_zero(g_clients + i);
                C_debug("Client %d connected", i);
                break;
        case G_SM_DISCONNECTED:
                i = N_receive_char();
                if (i < 0 || i >= N_CLIENTS_MAX) {
                        corrupt_disconnect();
                        return;
                }
                n_clients[i].connected = FALSE;
                I_print_chat(C_va("%s left the game.", g_clients[i].name),
                             I_COLOR, NULL);
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

