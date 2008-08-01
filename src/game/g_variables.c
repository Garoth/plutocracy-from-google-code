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

/* Game testing */
c_var_t g_test_globe, g_test_tile;

/* Globe variables */
c_var_t g_forest, g_globe_seed, g_globe_subdiv4, g_islands, g_island_size,
        g_island_variance;

/* Nation colors */
c_var_t g_nation_colors[G_NATION_NAMES];

/* Player settings */
c_var_t g_draw_distance, g_name;

/* Server settings */
c_var_t g_players;

/******************************************************************************\
 Registers the game namespace variables.
\******************************************************************************/
void G_register_variables(void)
{
        /* Game testing */
        C_register_string(&g_test_tile, "g_test_tile", "",
                          "path of tile PLUM model to test");
        C_register_integer(&g_test_globe, "g_test_globe", FALSE,
                           "test globe tile click detection");
        g_test_globe.edit = C_VE_ANYTIME;

        /* Globe variables */
        C_register_integer(&g_globe_seed, "g_globe_seed", C_rand(),
                           "seed for globe terrain generator");
        g_globe_seed.archive = FALSE;
        C_register_integer(&g_globe_subdiv4, "g_globe_subdiv4", 4,
                           "globe subdivision iterations, 0-5");
        C_register_integer(&g_islands, "g_islands", 0,
                           "number of islands, 0 for default");
        C_register_integer(&g_island_size, "g_island_size", 0,
                           "maximum size of islands, 0 for default");
        C_register_float(&g_island_variance, "g_island_variance", -1.f,
                           "proportion of island size to randomize");
        C_register_float(&g_forest, "g_forest", 0.8,
                           "proportion of tiles that have trees");

        /* Nation colors */
        C_register_string(g_nation_colors + G_NN_RED, "g_color_red",
                          "#80ef2929", "red national color");
        C_register_string(g_nation_colors + G_NN_GREEN, "g_color_green",
                          "#808ae234", "green national color");
        C_register_string(g_nation_colors + G_NN_BLUE, "g_color_blue",
                          "#80729fcf", "blue national color");
        C_register_string(g_nation_colors + G_NN_PIRATE, "g_color_pirate",
                          "#80a0a0a0", "pirate color");

        /* Player settings */
        C_register_string(&g_name, "g_name", "Newbie", "player name");
        C_register_float(&g_draw_distance, "g_draw_distance", 15.f,
                         "model drawing distance from surface");
        g_draw_distance.edit = C_VE_ANYTIME;

        /* Server settings */
        C_register_integer(&g_players, "g_players", 12,
                           "maximum number of players");
}

