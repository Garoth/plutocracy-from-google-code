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

/* Island parameter limits */
#define ISLAND_NUM 128
#define ISLAND_SIZE 256
#define ISLAND_HEIGHT 2.5f

/* Island structure */
typedef struct g_island {
        g_tile_t *root;
        int tiles, land;
} g_island_t;

g_tile_t g_tiles[R_TILES_MAX];

static r_tile_t render_tiles[R_TILES_MAX];
static g_island_t islands[ISLAND_NUM];
static int islands_len;

/******************************************************************************\
 Render the game elements.
\******************************************************************************/
void G_render(void)
{
}

/******************************************************************************\
 Randomly selects a tile ground terrain based on climate approximations.
 FIXME: Does not choose terrain correctly, biased toward 'hot'.
\******************************************************************************/
static r_terrain_t choose_terrain(int tile)
{
        float prop;

        prop = R_get_tile_latitude(tile);
        if (prop < 0.f)
                prop = -prop;
        prop = 4.f * prop / M_PI - 1.f;
        if (prop >= 0.f) {
                if (C_rand_real() > prop)
                        return R_T_GROUND;
                return R_T_GROUND_COLD;
        }
        if (C_rand_real() > -prop)
                return R_T_GROUND;
        return R_T_GROUND_HOT;
}

/******************************************************************************\
 Iterates over the entire globe and configures ground tile terrain.
\******************************************************************************/
static void sanitise_terrain(void)
{
        int i, j, region_len, region[12], hot, cold, temp, sand, land;

        /* During the first we convert shallow tiles to sand tiles */
        land = 0;
        for (i = 0; i < r_tiles; i++) {
                if (render_tiles[i].terrain != R_T_SHALLOW)
                        continue;
                region_len = R_get_tile_region(i, region);
                for (j = 0; j < region_len; j++)
                        if (render_tiles[region[j]].terrain == R_T_WATER)
                                goto skip_shallow;
                render_tiles[i].terrain = R_T_SAND;
                islands[g_tiles[i].island].land++;
                land++;
skip_shallow:   ;
        }

        /* During the second run, convert sand tiles to ground tiles */
        hot = cold = temp = 0;
        for (i = 0; i < r_tiles; i++) {
                if (render_tiles[i].terrain != R_T_SAND)
                        continue;
                region_len = R_get_tile_region(i, region);
                for (j = 0; j < region_len; j++)
                        if (render_tiles[region[j]].terrain == R_T_SHALLOW)
                                goto skip_sand;
                render_tiles[i].terrain = choose_terrain(i);
                render_tiles[i].height = C_rand_real() * ISLAND_HEIGHT;
                if (render_tiles[i].terrain == R_T_GROUND)
                        temp++;
                else if (render_tiles[i].terrain == R_T_GROUND_HOT)
                        hot++;
                else if (render_tiles[i].terrain == R_T_GROUND_COLD)
                        cold++;
skip_sand:      ;
        }

        /* Output statistics */
        sand = land - hot - temp - cold;
        C_debug("%d land tiles (%d%%)", land, 100 * land / (r_tiles - land));
        if (land)
                C_debug("%d sand (%d%%), %d temp (%d%%), %d hot (%d%%), "
                        "%d cold (%d%%)", sand, 100 * sand / land,
                        temp, 100 * temp / land, hot, 100 * hot / land,
                        cold, 100 * cold / land);
        for (i = 0; i < islands_len; i++)
                C_trace("Island %d, %d of %d land tiles (%d%%)",
                        i, islands[i].land, islands[i].tiles,
                        100 * islands[i].land / islands[i].tiles);
}

/******************************************************************************\
 Seeds the globe with at most [num] island seeds and interatively grows their
 edges until all space is consumed or all islands reach [island_size].
\******************************************************************************/
static void grow_islands(int num, int island_size)
{
        g_tile_t *edges[ISLAND_NUM * ISLAND_SIZE];
        int i, j, expanded, sizes[ISLAND_NUM], limits[ISLAND_NUM];

        if (num > ISLAND_NUM)
                num = ISLAND_NUM;
        if (island_size > ISLAND_SIZE)
                island_size = ISLAND_SIZE;
        C_debug("Growing %d, %d-tile islands", num, island_size);

        /* Disperse the initial seeds and set limits */
        for (i = 0; i < num; i++) {
                islands[i].root = g_tiles + (C_rand() % r_tiles);
                if (islands[i].root->island >= 0) {
                        num--;
                        i--;
                        continue;
                }
                sizes[i] = 1;
                edges[i * ISLAND_SIZE] = islands[i].root;
                limits[i] = (0.5 + C_rand_real()) * island_size;
                islands[i].root->island = i;
                islands[i].tiles = 1;
                islands[i].land = 0;
        }
        islands_len = num;

        /* Iteratively grow each island */
        for (expanded = TRUE; expanded; ) {
                expanded = FALSE;
                for (i = 0; i < num; i++) {
                        g_tile_t *next;
                        int index, border;

                        if (!sizes[i] || islands[i].tiles >= limits[i])
                                continue;
                        expanded = TRUE;
                        index = i * ISLAND_SIZE + (C_rand() % sizes[i]);
                        border = FALSE;
                        for (j = 0; j < 3; j++) {
                                next = edges[index]->neighbors[j];
                                if (next->island >= 0) {
                                        border |= next->island != i;
                                        continue;
                                }
                                edges[i * ISLAND_SIZE + sizes[i]++] = next;
                                next->island = i;
                                break;
                        }
                        if (j < 3) {
                                islands[next->island].tiles++;
                                continue;
                        }
                        if (!border)
                                edges[index]->render->terrain = R_T_SHALLOW;
                        memmove(edges + index, edges + index + 1,
                                ((i + 1) * ISLAND_SIZE - index - 1) *
                                sizeof (*edges));
                        sizes[i]--;
                }
        }
}

/******************************************************************************\
 Find each tile's neighbors and set fields.
\******************************************************************************/
static void setup_tiles(void)
{
        int i, j, neighbors[3];

        C_debug("Locating tile neighbors");
        for (i = 0; i < r_tiles; i++) {
                render_tiles[i].terrain = R_T_WATER;
                render_tiles[i].height = 0;
                g_tiles[i].render = render_tiles + i;
                g_tiles[i].island = -1;
                R_get_tile_neighbors(i, neighbors);
                for (j = 0; j < 3; j++) {
                        if (neighbors[j] < 0 || neighbors[j] >= r_tiles)
                                C_error("Invalid neighboring tile %d",
                                        neighbors[j]);
                        g_tiles[i].neighbors[j] = g_tiles + neighbors[j];
                }
        }
        C_rand_seed(g_globe_seed.value.n);

        /* Globe size affects the island parameters */
        switch (g_globe_subdiv4.value.n) {
        case 5: grow_islands(75, 256);
                break;
        case 4: grow_islands(25, 256);
                break;
        case 3: grow_islands(5, 256);
                break;
        case 2: grow_islands(3, 128);
                break;
        default:
                islands_len = 0;
                return;
        }

        sanitise_terrain();
}

/******************************************************************************\
 Initialize an idle globe.
\******************************************************************************/
void G_init(void)
{
        C_status("Generating globe");
        C_var_unlatch(&g_globe_seed);
        C_var_unlatch(&g_globe_subdiv4);
        R_generate_globe(g_globe_subdiv4.value.n);
        setup_tiles();
        R_configure_globe(render_tiles);
}

