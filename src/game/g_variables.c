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
c_var_t g_globe_islands, g_globe_island_size, g_globe_seed, g_globe_subdiv4;

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
        C_register_integer(&g_globe_islands, "g_globe_islands", 0,
                           "number of islands, 0 for default");
        C_register_integer(&g_globe_island_size, "g_globe_island_size", 0,
                           "maximum size of islands, 0 for default");
}

