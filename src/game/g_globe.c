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

static r_tile_t render_tiles[R_TILES_MAX];
g_tile_t g_tiles[R_TILES_MAX];

/******************************************************************************\
 Render the game elements.
\******************************************************************************/
void G_render(void)
{
}

/******************************************************************************\
 Find each tile's neighbors and set fields.
\******************************************************************************/
static void setup_tiles(void)
{
        int i;

        C_debug("Locating tile neighbors");
        for (i = 0; i < r_tiles; i++) {
                render_tiles[i].terrain = 1 + (C_rand() % 3);
                render_tiles[i].height = C_rand_real() * 3;
                g_tiles[i].render = render_tiles + i;
        }
}

/******************************************************************************\
 Initialize an idle globe.
\******************************************************************************/
void G_init(void)
{
        C_status("Generating globe");
        C_var_unlatch(&g_globe_seed);
        C_var_unlatch(&g_globe_subdiv4);
        R_generate_globe(g_globe_seed.value.n, g_globe_subdiv4.value.n);
        setup_tiles();
        R_configure_globe(render_tiles);
}

