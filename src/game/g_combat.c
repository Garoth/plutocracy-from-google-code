/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Handles ship combat mechanics */

#include "g_common.h"

/* Milliseconds between boarding attack rolls */
#define BOARD_INTERVAL 2000

/* Number of dice to roll for a boarding attack */
#define BOARD_DICE 5

/******************************************************************************\
 Check if we want to start boarding.
\******************************************************************************/
static void start_boarding(int ship)
{
        int i, neighbors[3], target_ship;

        if (!g_ships[ship].target_board)
                return;
        target_ship = g_ships[ship].target_ship;

        /* The target may have turned friendly in the mean time */
        if (!G_ship_hostile(ship, g_ships[target_ship].client)) {
                g_ships[ship].target_board = FALSE;
                g_ships[ship].target_ship = -1;
                return;
        }

        /* The ship must be adjacent to begin boarding */
        R_tile_neighbors(g_ships[ship].tile, neighbors);
        for (i = 0; g_tiles[neighbors[i]].ship != target_ship; i++)
                if (i >= 2)
                        return;

        /* Start a boarding attack */
        g_ships[ship].boarding_ship = target_ship;
        g_ships[ship].boarding++;
        g_ships[ship].modified = TRUE;
        g_ships[target_ship].boarding++;

        /* Host boarding announcements */
        if (G_ship_controlled_by(ship, n_client_id))
                I_popup(&g_ships[ship].model.origin,
                        C_va(C_str("g-boarding", "%s boarding the %s!"),
                             g_ships[ship].name, g_ships[target_ship].name));
        else if (G_ship_controlled_by(target_ship, n_client_id))
                I_popup(&g_ships[ship].model.origin,
                        C_va(C_str("g-boarded", "%s is being boarded!"),
                             g_ships[target_ship].name));
}

/******************************************************************************\
 Perform a boarding attack/defend roll. Returns TRUE on victory.
\******************************************************************************/
static bool ship_board_attack(int ship, int defender, int power)
{
        int crew;

        /* How much crew did attacker kill? */
        crew = C_roll_dice(BOARD_DICE, power) / BOARD_DICE - 1;
        if (crew < 1)
                return FALSE;
        G_store_add(&g_ships[defender].store, G_CT_CREW, -crew);

        /* Did we win? */
        if (g_ships[defender].store.cargo[G_CT_CREW].amount >= 1)
                return FALSE;
        g_ships[ship].boarding--;
        g_ships[ship].modified = TRUE;
        g_ships[defender].boarding--;
        g_ships[defender].modified = TRUE;
        C_assert(g_ships[ship].boarding >= 0);
        C_assert(g_ships[defender].boarding >= 0);

        /* Reset orders for defeated ship */
        g_ships[defender].target_ship = -1;
        g_ships[defender].target = -1;

        /* Not enough crew to capture */
        if (g_ships[ship].store.cargo[G_CT_CREW].amount < 2) {
                G_ship_change_client(defender, N_SERVER_ID);
                return TRUE;
        }

        /* Capture the ship and transfer a crew unit */
        G_ship_change_client(defender, g_ships[ship].client);
        G_store_add(&g_ships[ship].store, G_CT_CREW, -1);
        G_store_add(&g_ships[defender].store, G_CT_CREW, 1);
        return TRUE;
}

/******************************************************************************\
 Update boarding mechanics.
\******************************************************************************/
static void ship_update_board(int ship)
{
        if (c_time_msec < g_ships[ship].combat_time ||
            g_ships[ship].boarding_ship < 0)
                return;
        if (ship_board_attack(ship, g_ships[ship].boarding_ship, 4) ||
            ship_board_attack(g_ships[ship].boarding_ship, ship, 6)) {
                g_ships[ship].boarding_ship = -1;
                return;
        }
        g_ships[ship].combat_time = c_time_msec + BOARD_INTERVAL;
}

/******************************************************************************\
 Pump combat mechanics. The ship may have lost its crew or health after this
 call!
\******************************************************************************/
void G_ship_update_combat(int ship)
{
        if (n_client_id != N_HOST_CLIENT_ID)
                return;

        /* In a boarding fight */
        if (g_ships[ship].boarding > 0) {
                ship_update_board(ship);
                return;
        }

        /* Can't start a fight or don't want to */
        if (g_ships[ship].rear_tile >= 0 || g_ships[ship].target_ship < 0 ||
            g_ships[ship].store.cargo[G_CT_CREW].amount < 2)
                return;

        start_boarding(ship);
}

