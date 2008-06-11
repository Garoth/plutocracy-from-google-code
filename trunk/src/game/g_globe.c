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

/* Maximum number of islands */
#define ISLAND_NUM 128

/* Maximum island size */
#define ISLAND_SIZE 384

/* Island size will vary up to this proportion */
#define ISLAND_VARIANCE 0.5f

/* Maximum height off the globe surface of a tile */
#define ISLAND_HEIGHT 5.f

/* Minimum number of land tiles for island to succeed */
#define ISLAND_LAND 20

/* Island structure */
typedef struct g_island {
        g_tile_t *root;
        int tiles, land;
} g_island_t;

/* Island tiles with game data */
g_tile_t g_tiles[R_TILES_MAX];

static r_tile_t render_tiles[R_TILES_MAX];
static g_island_t islands[ISLAND_NUM];
static float visible_limit;
static int islands_len;

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
        prop = 4.f * prop / C_PI - 1.f;
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

        /* During the first pass we convert shallow tiles to sand tiles */
        land = 0;
        for (i = 0; i < r_tiles; i++) {
                if (render_tiles[i].terrain != R_T_SHALLOW)
                        continue;
                region_len = R_get_tile_region(i, region);
                for (j = 0; j < region_len; j++)
                        if (render_tiles[region[j]].terrain == R_T_WATER ||
                            g_tiles[region[j]].island != g_tiles[i].island)
                                goto skip_shallow;
                render_tiles[i].terrain = R_T_SAND;
                islands[g_tiles[i].island].land++;
                land++;
skip_shallow:   ;
        }

        /* During the second pass, convert sand tiles to ground tiles and
           set terrain height */
        hot = cold = temp = 0;
        for (i = 0; i < r_tiles; i++) {
                if (render_tiles[i].terrain != R_T_SAND)
                        continue;
                region_len = R_get_tile_region(i, region);
                for (j = 0; j < region_len; j++)
                        if (render_tiles[region[j]].terrain == R_T_SHALLOW ||
                            render_tiles[region[j]].terrain == R_T_WATER ||
                            g_tiles[region[j]].island != g_tiles[i].island)
                                goto skip_sand;
                render_tiles[i].terrain = choose_terrain(i);

                /* Give height to tile region */
                render_tiles[i].height = C_rand_real() * ISLAND_HEIGHT;

                /* Keep track of terrain statistics */
                if (render_tiles[i].terrain == R_T_GROUND)
                        temp++;
                else if (render_tiles[i].terrain == R_T_GROUND_HOT)
                        hot++;
                else if (render_tiles[i].terrain == R_T_GROUND_COLD)
                        cold++;
skip_sand:      ;
        }

        /* During the third pass, smooth terrain height and remove failed
           islands*/
        for (i = 0; i < r_tiles; i++) {
                float height;

                /* Remove this tile if it belongs to a failed island */
                if (islands[g_tiles[i].island].land < ISLAND_LAND) {
                        render_tiles[i].terrain = R_T_WATER;
                        render_tiles[i].height = 0.f;
                        continue;
                }

                /* Smooth the height */
                region_len = R_get_tile_region(i, region);
                for (j = 0, height = 0.f; j < region_len; j++)
                        height += render_tiles[region[j]].height;
                render_tiles[i].height = (render_tiles[i].height +
                                          height / region_len) / 2.f;
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
                limits[i] = (int)((ISLAND_VARIANCE * (C_rand_real() - 0.5f) +
                                   1.f) * island_size);
                if (limits[i] > ISLAND_SIZE)
                        limits[i] = ISLAND_SIZE;
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
                        int index;

                        if (!sizes[i] || islands[i].tiles >= limits[i])
                                continue;
                        expanded = TRUE;
                        index = i * ISLAND_SIZE + (C_rand() % sizes[i]);
                        for (j = 0; j < 3; j++) {
                                next = edges[index]->neighbors[j];
                                if (next->island >= 0)
                                        continue;
                                edges[i * ISLAND_SIZE + sizes[i]++] = next;
                                next->island = i;
                                break;
                        }
                        if (j < 3) {
                                islands[next->island].tiles++;
                                continue;
                        }
                        edges[index]->render->terrain = R_T_SHALLOW;
                        memmove(edges + index, edges + index + 1,
                                ((i + 1) * ISLAND_SIZE - index - 1) *
                                sizeof (*edges));
                        sizes[i]--;
                }
        }
}

/******************************************************************************\
 Initialize and position a tile's model.
\******************************************************************************/
static int set_tile_model(int tile, const char *filename)
{
        R_model_cleanup(&g_tiles[tile].model);
        if (!R_model_init(&g_tiles[tile].model, filename))
                return FALSE;
        g_tiles[tile].model.origin = g_tiles[tile].origin;
        g_tiles[tile].model.normal = g_tiles[tile].normal;
        g_tiles[tile].model.forward = g_tiles[tile].forward;
        return TRUE;
}

/******************************************************************************\
 Cleanup globe assets.
\******************************************************************************/
void G_cleanup_globe(void)
{
        int i;

        for (i = 0; i < r_tiles; i++)
                R_model_cleanup(&g_tiles[i].model);
}

/******************************************************************************\
 Update function for [g_test_tiles].
\******************************************************************************/
static int test_tiles_update(c_var_t *var, c_var_value_t value)
{
        int i;

        for (i = 0; i < r_tiles; i++)
                if (g_tiles[i].render->terrain != R_T_WATER &&
                    g_tiles[i].render->terrain != R_T_SHALLOW &&
                    !set_tile_model(i, value.s)) {
                        for (; i < r_tiles; i++)
                                R_model_cleanup(&g_tiles[i].model);
                        return TRUE;
                }
        return TRUE;
}

/******************************************************************************\
 Initialize an idle globe.
\******************************************************************************/
void G_generate_globe(void)
{
        int i, j, islands, island_size;

        C_status("Generating globe");
        C_var_unlatch(&g_globe_seed);
        C_var_unlatch(&g_globe_subdiv4);
        C_var_unlatch(&g_globe_islands);
        C_var_unlatch(&g_globe_island_size);
        G_cleanup_globe();
        R_generate_globe(g_globe_subdiv4.value.n);
        C_rand_seed(g_globe_seed.value.n);

        /* Locate tile neighbors */
        for (i = 0; i < r_tiles; i++) {
                int neighbors[3];

                render_tiles[i].terrain = R_T_WATER;
                render_tiles[i].height = 0;
                g_tiles[i].render = render_tiles + i;
                g_tiles[i].island = -1;
                C_zero(&g_tiles[i].model);
                R_get_tile_neighbors(i, neighbors);
                for (j = 0; j < 3; j++) {
                        if (neighbors[j] < 0 || neighbors[j] >= r_tiles)
                                C_error("Invalid neighboring tile %d",
                                        neighbors[j]);
                        g_tiles[i].neighbors[j] = g_tiles + neighbors[j];
                }
        }

        /* Grow the islands and set terrain. Globe size affects the island
           growth parameters. */
        switch (g_globe_subdiv4.value.n) {
        case 5: islands = 75;
                island_size = 180;
                break;
        case 4: islands = 40;
                island_size = 180;
                break;
        case 3: islands = 10;
                island_size = 140;
                break;
        case 2: islands = 3;
                island_size = 100;
                break;
        default:
                islands_len = 0;
                return;
        }
        if (g_globe_islands.value.n > 0)
                islands = g_globe_islands.value.n;
        if (g_globe_island_size.value.n > 0)
                island_size = g_globe_island_size.value.n;
        grow_islands(islands, island_size);
        sanitise_terrain();

        /* This call actually raises the tiles to match terrain height */
        R_configure_globe(render_tiles);

        /* Calculate tile vectors */
        for (i = 0; i < r_tiles; i++) {
                c_vec3_t coords[3], ab, ac;

                R_get_tile_coords(i, coords);

                /* Centroid */
                g_tiles[i].origin = C_vec3_add(coords[0], coords[1]);
                g_tiles[i].origin = C_vec3_add(g_tiles[i].origin, coords[2]);
                g_tiles[i].origin = C_vec3_divf(g_tiles[i].origin, 3.f);

                /* Normal Vector */
                ab = C_vec3_sub(coords[1], coords[0]);
                ac = C_vec3_sub(coords[2], coords[0]);
                g_tiles[i].normal = C_vec3_norm(C_vec3_cross(ab, ac));

                /* Forward Vector */
                g_tiles[i].forward = C_vec3_norm(C_vec3_sub(coords[0],
                                                            g_tiles[i].origin));
        }

        /* We can now set test tiles */
        C_var_update(&g_test_tile, (c_var_update_f)test_tiles_update);

        /* Set the invisible tile boundary */
        visible_limit = 0.f;
        if (g_globe_subdiv4.value.n >= 4)
                visible_limit = -11.25f;
        if (g_globe_subdiv4.value.n >= 5)
                visible_limit = -36.f;
}

/******************************************************************************\
 Returns TRUE if the tile is within the visible hemisphere. The visible limit
 refers to how far along the camera forward vector the tile is. Smaller values
 are closer to the camera.
\******************************************************************************/
static int tile_visible(int tile)
{
        return C_vec3_dot(r_cam_forward, g_tiles[tile].origin) < visible_limit;
}

/******************************************************************************\
 Render the globe.
\******************************************************************************/
void G_render_globe(void)
{
        int i;

        R_start_globe();
        for (i = 0; i < r_tiles; i++)
                if (tile_visible(i)) {
                        R_adjust_light_for(g_tiles[i].origin);
                        R_model_render(&g_tiles[i].model);
                }
        R_finish_globe();
}

/******************************************************************************\
 After a mouse click is transformed into a ray with [origin] and [forward]
 vector, this function will find which tile (if any) was clicked on and
 trigger any actions this may have caused.
\******************************************************************************/
void G_click_ray(c_vec3_t origin, c_vec3_t forward)
{
}

