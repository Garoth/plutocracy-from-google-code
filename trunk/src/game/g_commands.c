/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Receives all user commands from the interface */

#include "g_common.h"

/******************************************************************************\
 Interface wants us to leave the current game.
\******************************************************************************/
void G_leave_game(void)
{
        if (n_client_id == N_INVALID_ID)
                return;
        N_disconnect();
        I_popup(NULL, "Left the current game.");
}

/******************************************************************************\
 Change the player's nation.
\******************************************************************************/
void G_change_nation(int index)
{
        N_send(N_SERVER_ID, "11", G_CM_AFFILIATE, index);
}

/******************************************************************************\
 Callback for ring opened when right-clicking a tile.
\******************************************************************************/
static void tile_ring(i_ring_icon_t icon)
{
        if (g_selected_tile < 0)
                return;
        N_send(N_SERVER_ID, "121", G_CM_TILE_RING, g_selected_tile, icon);
}

/******************************************************************************\
 Called when the interface root window receives a click. Returns FALSE is
 no action was taken.
\******************************************************************************/
bool G_process_click(int button)
{
        /* Grabbing has no other effect */
        if (button == SDL_BUTTON_MIDDLE)
                return FALSE;

        /* Left-clicking selects only */
        if (button == SDL_BUTTON_LEFT) {

                /* Selecting a ship */
                if (g_hover_ship >= 0) {
                        G_tile_select(-1);
                        G_ship_select(g_hover_ship != g_selected_ship ?
                                      g_hover_ship : -1);
                        return TRUE;
                }

                /* Selecting a tile */
                if (g_hover_tile >= 0) {
                        G_ship_select(-1);
                        G_tile_select(g_hover_tile != g_selected_tile ?
                                      g_hover_tile : - 1);
                        return TRUE;
                }

                /* Failed to select anything */
                G_ship_select(-1);
                G_tile_select(-1);
                return FALSE;
        }

        /* Only process right-clicks from here on */
        if (button != SDL_BUTTON_RIGHT)
                return FALSE;

        /* Controlling a ship */
        if (g_selected_ship >= 0 &&
            g_ships[g_selected_ship].client == n_client_id) {

                /* Ordered an ocean move */
                if (g_hover_tile >= 0 && G_open_tile(g_hover_tile, -1))
                        N_send(N_SERVER_ID, "112", G_CM_SHIP_MOVE,
                               g_selected_ship, g_hover_tile);

                return TRUE;
        }

        /* Opening the selected tile's ring menu */
        if (g_selected_tile >= 0 && g_selected_tile == g_hover_tile) {
                g_building_class_t *bc;
                bool can_pay;

                /* Settling the island */
                if (g_islands[g_tiles[g_selected_tile].island].town_tile < 0) {
                        bc = g_building_classes + G_BT_TOWN_HALL;
                        can_pay = G_pay(n_client_id, g_selected_tile,
                                        &bc->cost, FALSE);
                        I_reset_ring();
                        I_add_to_ring(I_RI_TOWN_HALL, can_pay);
                        I_show_ring((i_ring_f)tile_ring);
                }

                return TRUE;
        }

        return FALSE;
}

/******************************************************************************\
 The user has typed chat into the box and hit enter.
\******************************************************************************/
void G_input_chat(char *message)
{
        i_color_t color;

        if (!message || !message[0])
                return;
        color = G_nation_to_color(g_clients[n_client_id].nation);
        C_sanitize(message);
        I_print_chat(g_clients[n_client_id].name, color, message);
        N_send(N_SERVER_ID, "1s", G_CM_CHAT, message);
}

/******************************************************************************\
 Interface wants us to join a game hosted at [address].
\******************************************************************************/
void G_join_game(const char *address)
{
        G_leave_game();

        /* Connect to server */
        if (N_connect(address, (n_callback_f)G_client_callback)) {
                I_popup(NULL, C_va("Connected to '%s'.", address));
                return;
        }
        I_popup(NULL, C_va("Failed to connect to '%s'.", address));
}

/******************************************************************************\
 Received updated trade parameters from the interface.
\******************************************************************************/
void G_trade_params(g_cargo_type_t index, int buy_price, int sell_price,
                    int minimum, int maximum)
{
        g_cargo_t old_cargo, *cargo;

        if (g_selected_ship < 0)
                return;
        C_assert(g_ships[g_selected_ship].client == n_client_id);
        cargo = g_ships[g_selected_ship].store.cargo + index;
        old_cargo = *cargo;

        /* Update cargo */
        if ((cargo->auto_buy = buy_price >= 0))
                cargo->buy_price = buy_price;
        if ((cargo->auto_sell = sell_price >= 0))
                cargo->sell_price = sell_price;
        cargo->minimum = minimum;
        cargo->maximum = maximum;

        /* Did anything actually change? */
        if (G_cargo_equal(&old_cargo, cargo))
                return;

        /* Tell the server */
        N_send(N_SERVER_ID, "1112222", G_CM_SHIP_PRICES, g_selected_ship,
               index, buy_price, sell_price, minimum, maximum);
}

/******************************************************************************\
 Transmit purchase/sale request for the currently selected ship from the
 interface to the server. Use a negative amount to indicate a sale.
\******************************************************************************/
void G_buy_cargo(g_cargo_type_t cargo, int amount)
{
        g_ship_t *ship, *partner;

        C_assert(cargo >= 0 && cargo < G_CARGO_TYPES);
        ship = g_ships + g_selected_ship;
        if (g_selected_ship < 0 || ship->trade_tile < 0 ||
            !G_ship_can_trade_with(g_selected_ship, ship->trade_tile))
                return;
        partner = g_ships + g_tiles[ship->trade_tile].ship;

        /* Clamp the amount to what we can actually transfer */
        if (!(amount = G_limit_purchase(&ship->store, &partner->store,
                                        cargo, amount)))
                return;

        N_send(N_SERVER_ID, "11212", G_CM_SHIP_BUY, g_selected_ship,
               ship->trade_tile, cargo, amount);
}

/******************************************************************************\
 A keypress event was forwarded from the interface.
\******************************************************************************/
void G_process_key(int key, bool shift, bool ctrl, bool alt)
{
        /* Focus on selected tile or ship */
        if (key == SDLK_SPACE) {
                if (g_selected_tile >= 0) {
                        R_rotate_cam_to(g_tiles[g_selected_tile].origin);
                        return;
                }
                G_focus_next_ship();
        }

        /* Deselect */
        else if (key == SDLK_ESCAPE) {
                G_tile_select(-1);
                G_ship_select(-1);
        }
}

