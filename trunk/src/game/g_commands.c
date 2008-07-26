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

/* The currently selected tile and ship index. Negative values indicate an
   invalid selection. */
int g_selected_tile, g_selected_ship;

static bool ring_valid;

/******************************************************************************\
 Leave the current game.
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
 Returns TRUE if there are possible ring interactions with the [tile].
\******************************************************************************/
static int can_interact(int tile)
{
        if (g_test_globe.value.n)
                return TRUE;
        return FALSE;
}

/******************************************************************************\
 Selects a tile during mouse hover. Pass -1 to deselect.
\******************************************************************************/
void G_select_tile(int tile)
{
        C_assert(tile < r_tiles);

        /* Deselect the old tile */
        if (g_selected_tile >= 0)
                g_tiles[g_selected_tile].model.selected = FALSE;
        R_select_tile(g_selected_tile = -1, R_ST_NONE);

        /* See if there is actually any interaction possible with this tile */
        ring_valid = FALSE;
        if (tile < 0)
                return;
        ring_valid = can_interact(tile);

        /* Selecting a ship */
        if (g_tiles[tile].ship >= 0) {
        }

        /* If we are controlling a ship, we might want to move here */
        else if (g_selected_ship >= 0 && G_open_tile(tile, -1) &&
                 g_ships[g_selected_ship].client == n_client_id) {
                R_select_tile(tile, R_ST_GOTO);
                g_selected_tile = tile;
                return;
        }

        /* Select a tile */
        else if (ring_valid)
                R_select_tile(tile, R_ST_TILE);

        /* Can't select this */
        else {
                R_select_tile(-1, R_ST_NONE);
                g_selected_tile = -1;
                return;
        }

        g_tiles[tile].model.selected = TRUE;
        g_selected_tile = tile;
}

/******************************************************************************\
 Test ring callback function.
\******************************************************************************/
static void test_ring_callback(i_ring_icon_t icon)
{
        if (icon == I_RI_TEST_MILL)
                G_set_tile_model(g_selected_tile, "models/test/mill.plum");
        else if (icon == I_RI_TEST_TREE)
                G_set_tile_model(g_selected_tile,
                               "models/tree/deciduous.plum");
        else if (icon == I_RI_TEST_SHIP)
                G_set_tile_model(g_selected_tile,
                               "models/ship/sloop.plum");
        else
                G_set_tile_model(g_selected_tile, "");
}

/******************************************************************************\
 Called when the interface root window receives a click.
\******************************************************************************/
void G_process_click(int button)
{
        /* The game only handles left and middle clicks */
        if (button != SDL_BUTTON_LEFT && button != SDL_BUTTON_MIDDLE)
                return;

        /* Clicking on an unusable space deselects */
        if (g_selected_tile < 0 || button != SDL_BUTTON_LEFT) {
                G_ship_select(-1);
                return;
        }

        /* Left-clicked on a ship */
        if (g_tiles[g_selected_tile].ship >= 0) {
                if (g_selected_ship != g_tiles[g_selected_tile].ship)
                        G_ship_select(g_tiles[g_selected_tile].ship);
                else
                        G_ship_select(-1);
                return;
        }

        /* Controlling a ship */
        if (g_selected_ship >= 0 &&
            g_ships[g_selected_ship].client == n_client_id) {

                /* Ordered an ocean move */
                if (g_selected_tile >= 0 && G_open_tile(g_selected_tile, -1)) {
                        N_send(N_SERVER_ID, "112", G_CM_SHIP_MOVE,
                               g_selected_ship, g_selected_tile);
                        return;
                }
        }

        /* Left-clicked on a tile */
        G_ship_select(-1);
        if (!ring_valid)
                return;

        /* When globe testing is on, always show the test ring */
        if (g_test_globe.value.n) {
                I_reset_ring();
                I_add_to_ring(I_RI_TEST_BLANK, TRUE);
                I_add_to_ring(I_RI_TEST_MILL, TRUE);
                I_add_to_ring(I_RI_TEST_TREE, TRUE);
                I_add_to_ring(I_RI_TEST_SHIP, TRUE);
                I_add_to_ring(I_RI_TEST_DISABLED, FALSE);
                I_show_ring(test_ring_callback);
        }
}

/******************************************************************************\
 The user has typed chat into the box and hit enter.
\******************************************************************************/
void G_input_chat(const char *message)
{
        i_color_t color;

        if (!message || !message[0])
                return;
        color = G_nation_to_color(g_clients[n_client_id].nation);
        I_print_chat(g_clients[n_client_id].name, color, message);
        N_send(N_SERVER_ID, "1s", G_CM_CHAT, message);
}

/******************************************************************************\
 Host a new game.
\******************************************************************************/
void G_join_game(const char *address)
{
        G_leave_game();
        G_reset_elements();

        /* Connect to server */
        if (N_connect(address, (n_callback_f)G_client_callback)) {
                I_popup(NULL, C_va("Connected to '%s'.", address));
                return;
        }
        I_popup(NULL, C_va("Failed to connect to '%s'.", address));
}

