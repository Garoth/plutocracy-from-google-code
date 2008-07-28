/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Implements globe terrain generation. Note that changing any of these
   parameters will invalidate the protocol! */

#include "g_common.h"

/* Maximum number of islands. Do not set this value above G_ISLAND_INVALID. */
#define ISLAND_NUM 128

/* Maximum island size */
#define ISLAND_SIZE 384

/* Island size will vary up to this proportion. This is a good way to control
   how "boring" the terrain generator is. With high variance there are a lot of
   oceans and strangely sized islands. */
#define ISLAND_VARIANCE 0.3f

/* Maximum height off the globe surface of a tile */
#define ISLAND_HEIGHT 4.f

/* Minimum number of land tiles for island to succeed */
#define ISLAND_LAND 8

/* Rate of model fades in alpha per second */
#define MODEL_FADE 1.f

/* Distance that models fade out */
#define MODEL_FADE_DIST 4.f

/* Island structure */
typedef struct g_island {
        int tiles, land, root;
} g_island_t;

/* Island tiles with game data */
g_tile_t g_tiles[R_TILES_MAX];

static g_island_t islands[ISLAND_NUM];
static float visible_range;
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
                if (r_tile_params[i].terrain != R_T_SHALLOW)
                        continue;
                region_len = R_get_tile_region(i, region);
                for (j = 0; j < region_len; j++)
                        if (r_tile_params[region[j]].terrain == R_T_WATER ||
                            g_tiles[region[j]].island != g_tiles[i].island)
                                goto skip_shallow;
                r_tile_params[i].terrain = R_T_SAND;
                islands[g_tiles[i].island].land++;
                land++;
skip_shallow:   ;
        }

        /* During the second pass, convert sand tiles to ground tiles and
           set terrain height */
        hot = cold = temp = 0;
        for (i = 0; i < r_tiles; i++) {
                if (r_tile_params[i].terrain != R_T_SAND)
                        continue;
                region_len = R_get_tile_region(i, region);
                for (j = 0; j < region_len; j++)
                        if (r_tile_params[region[j]].terrain == R_T_SHALLOW ||
                            r_tile_params[region[j]].terrain == R_T_WATER ||
                            g_tiles[region[j]].island != g_tiles[i].island)
                                goto skip_sand;
                r_tile_params[i].terrain = choose_terrain(i);

                /* Give height to tile region */
                r_tile_params[i].height = C_rand_real() * ISLAND_HEIGHT;

                /* Keep track of terrain statistics */
                if (r_tile_params[i].terrain == R_T_GROUND)
                        temp++;
                else if (r_tile_params[i].terrain == R_T_GROUND_HOT)
                        hot++;
                else if (r_tile_params[i].terrain == R_T_GROUND_COLD)
                        cold++;
skip_sand:      ;
        }

        /* During the third pass, smooth terrain height and remove failed
           islands*/
        for (i = 0; i < r_tiles; i++) {
                float height;

                /* Remove this tile if it belongs to a failed island */
                if (islands[g_tiles[i].island].land < ISLAND_LAND) {
                        r_tile_params[i].terrain = R_T_WATER;
                        r_tile_params[i].height = 0.f;
                        g_tiles[i].island = G_ISLAND_INVALID;
                        continue;
                }

                /* Smooth the height */
                region_len = R_get_tile_region(i, region);
                for (j = 0, height = 0.f; j < region_len; j++)
                        height += r_tile_params[region[j]].height;
                r_tile_params[i].height = (r_tile_params[i].height +
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
        int i, j, expanded, sizes[ISLAND_NUM], limits[ISLAND_NUM],
            edges[ISLAND_NUM * ISLAND_SIZE];

        if (num > ISLAND_NUM)
                num = ISLAND_NUM;
        if (island_size > ISLAND_SIZE)
                island_size = ISLAND_SIZE;
        C_debug("Growing %d, %d-tile islands", num, island_size);

        /* Disperse the initial seeds evenly and set limits */
        for (i = 0; i < num; i++) {
                islands[i].root = i * r_tiles / num;
                sizes[i] = 1;
                edges[i * ISLAND_SIZE] = islands[i].root;
                limits[i] = (int)((2 * ISLAND_VARIANCE *
                                   (C_rand_real() - 0.5f) + 1.f) * island_size);
                if (limits[i] < ISLAND_LAND * 3)
                        limits[i] = ISLAND_LAND * 3;
                if (limits[i] > ISLAND_SIZE)
                        limits[i] = ISLAND_SIZE;
                g_tiles[islands[i].root].island = i;
                islands[i].tiles = 1;
                islands[i].land = 0;
        }
        islands_len = num;

        /* Iteratively grow each island */
        for (expanded = TRUE; expanded; ) {
                expanded = FALSE;
                for (i = 0; i < num; i++) {
                        int index, next, neighbors[3];

                        if (!sizes[i] || islands[i].tiles >= limits[i])
                                continue;
                        expanded = TRUE;
                        index = i * ISLAND_SIZE + (C_rand() % sizes[i]);
                        R_get_tile_neighbors(edges[index], neighbors);
                        for (j = 0; j < 3; j++) {
                                next = neighbors[j];
                                if (g_tiles[next].island != G_ISLAND_INVALID)
                                        continue;
                                edges[i * ISLAND_SIZE + sizes[i]++] = next;
                                g_tiles[next].island = i;
                                break;
                        }
                        if (j < 3) {
                                islands[g_tiles[next].island].tiles++;
                                continue;
                        }
                        r_tile_params[edges[index]].terrain = R_T_SHALLOW;
                        memmove(edges + index, edges + index + 1,
                                ((i + 1) * ISLAND_SIZE - index - 1) *
                                sizeof (*edges));
                        sizes[i]--;
                }
        }
}

/******************************************************************************\
 Initialize and position a tile's model. Returns FALSE if the model failed to
 load.
\******************************************************************************/
int G_set_tile_model(int tile, const char *filename)
{
        /* Fade out if clearing the tile */
        if (!filename || !filename[0]) {
                g_tiles[tile].model_shown = FALSE;
                return TRUE;
        }

        /* Try to load the new model */
        R_model_cleanup(&g_tiles[tile].model);
        if (!R_model_init(&g_tiles[tile].model, filename, TRUE))
                return FALSE;

        /* Fade the new model in */
        g_tiles[tile].model.origin = g_tiles[tile].origin;
        g_tiles[tile].model.normal = r_tile_params[tile].normal;
        g_tiles[tile].model.forward = g_tiles[tile].forward;
        g_tiles[tile].model.selected = g_selected_tile == tile;
        g_tiles[tile].model_shown = TRUE;
        g_tiles[tile].fade = 1.f;
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
 Update function for [g_test_tiles]. Only land tiles are used for testing.
\******************************************************************************/
static int test_tiles_update(c_var_t *var, c_var_value_t value)
{
        int i;
        bool failed;

        failed = FALSE;
        for (i = 0; i < r_tiles; i++)
                if (!R_water_terrain(r_tile_params[i].terrain)) {
                        if (failed)
                                G_set_tile_model(i, "");
                        else
                                failed = !G_set_tile_model(i, value.s);
                }
        return TRUE;
}

/******************************************************************************\
 One-time globe initialization. Call after rendering has been initialized.
\******************************************************************************/
void G_init_globe(void)
{
        /* Generate a starter globe */
        C_var_unlatch(&g_globe_islands);
        C_var_unlatch(&g_globe_island_size);
        G_generate_globe(g_globe_islands.value.n, g_globe_island_size.value.n);
}

/******************************************************************************\
 Generate a new globe.
\******************************************************************************/
void G_generate_globe(int override_islands, int override_size)
{
        int i, islands, island_size;

        C_status("Generating globe");
        C_var_unlatch(&g_globe_seed);
        C_var_unlatch(&g_globe_subdiv4);
        G_cleanup_globe();
        R_generate_globe(g_globe_subdiv4.value.n);
        C_rand_seed(g_globe_seed.value.n);

        /* Initialize tile structures */
        for (i = 0; i < r_tiles; i++) {
                r_tile_params[i].terrain = R_T_WATER;
                r_tile_params[i].height = 0;
                g_tiles[i].island = G_ISLAND_INVALID;
                g_tiles[i].ship = -1;
                C_zero(&g_tiles[i].model);
        }

        /* Grow the islands and set terrain. Globe size affects the island
           growth parameters. */
        switch (g_globe_subdiv4.value.n) {
        case 5: islands = 125;
                island_size = 220;
                break;
        case 4: islands = 40;
                island_size = 160;
                break;
        case 3: islands = 10;
                island_size = 160;
                break;
        case 2: islands = 3;
                island_size = 160;
                break;
        default:
                islands_len = 0;
                return;
        }
        if (override_islands > 0)
                islands = override_islands;
        if (override_size > 0)
                island_size = override_size;
        grow_islands(islands, island_size);
        sanitise_terrain();

        /* This call actually raises the tiles to match terrain height */
        R_configure_globe();

        /* Calculate tile vectors and place starter buildings */
        for (i = 0; i < r_tiles; i++) {
                c_vec3_t coords[3];

                R_get_tile_coords(i, coords);

                /* Centroid */
                g_tiles[i].origin = C_vec3_add(coords[0], coords[1]);
                g_tiles[i].origin = C_vec3_add(g_tiles[i].origin, coords[2]);
                g_tiles[i].origin = C_vec3_divf(g_tiles[i].origin, 3.f);

                /* Forward Vector */
                g_tiles[i].forward = C_vec3_norm(C_vec3_sub(coords[0],
                                                            g_tiles[i].origin));
        }

        /* We can now set test tiles */
        C_var_update(&g_test_tile, (c_var_update_f)test_tiles_update);

        /* Deselect everything */
        g_selected_tile = -1;
        g_selected_ship = -1;
}

/******************************************************************************\
 Returns TRUE if the point is within the visible globe hemisphere.
\******************************************************************************/
static bool is_visible(c_vec3_t origin)
{
        return C_vec3_dot(r_cam_forward, origin) < visible_range;
}

/******************************************************************************\
 Returns a modulating factor for fading models out of visible range.
\******************************************************************************/
static float model_fade_mod(c_vec3_t origin)
{
        float dist;

        dist = C_vec3_dot(r_cam_forward, origin);
        if (dist < visible_range - MODEL_FADE_DIST)
                return 1.f;
        if (dist > visible_range)
                return 0.f;
        return (visible_range - dist) / MODEL_FADE_DIST;
}

/******************************************************************************\
 Render the globe and updates tile visibility.
\******************************************************************************/
void G_render_globe(void)
{
        int i;

        /* Set the invisible tile boundary */
        visible_range = -r_globe_radius + g_draw_distance.value.f;

        /* Render tile models */
        R_start_globe();
        for (i = 0; i < r_tiles; i++) {
                float mod;

                g_tiles[i].visible = is_visible(g_tiles[i].origin);
                mod = model_fade_mod(g_tiles[i].model.origin);
                if (mod <= 0.f || !g_tiles[i].model.data)
                        continue;

                /* Fade the model in */
                if (g_tiles[i].model_shown) {
                        g_tiles[i].fade += MODEL_FADE * c_frame_sec;
                        if (g_tiles[i].fade > 1.f)
                                g_tiles[i].fade = 1.f;
                }

                /* Fade the model out */
                else {
                        g_tiles[i].fade -= MODEL_FADE * c_frame_sec;
                        if (g_tiles[i].fade <= 0.f) {
                                R_model_cleanup(&g_tiles[i].model);
                                continue;
                        }
                }

                R_adjust_light_for(g_tiles[i].model.origin);
                g_tiles[i].model.modulate.a = g_tiles[i].fade * mod;
                R_model_render(&g_tiles[i].model);
        }
        R_finish_globe();

        /* Render a test line from the selected tile */
        if (g_test_globe.value.n && g_selected_tile >= 0) {
                c_vec3_t b;

                b = C_vec3_add(g_tiles[g_selected_tile].origin,
                               r_tile_params[g_selected_tile].normal);
                R_render_test_line(g_tiles[g_selected_tile].origin, b,
                                   C_color(0.f, 1.f, 0.f, 1.f));
        }

        G_render_ships();
}

/******************************************************************************\
 Returns TRUE if the given ray intersects the tile.

 The algorithm used here projects the ray onto the triangle's plane and
 tests the barycentric coordinates of the intersection point to determine
 whether the triangle was hit. Source:
 http://www.devmaster.net/wiki/Ray-triangle_intersection
\******************************************************************************/
static int ray_intersects_tile(c_vec3_t o, c_vec3_t d, int tile)
{
        c_vec3_t p, triangle[3], normal;
        c_vec2_t q, b, c;
        float t, u, v, b_cross_c;
        int axis;

        R_get_tile_coords(tile, triangle);
        normal = r_tile_params[tile].normal;

        /* Find [P], the ray's location in the triangle plane */
        o = C_vec3_sub(o, triangle[0]);
        t = C_vec3_dot(normal, o) / -C_vec3_dot(normal, d);
        if (t <= 0.f)
                return FALSE;
        p = C_vec3_add(o, C_vec3_scalef(d, t));

        /* Project the points onto one axis to simplify calculations. Choose the
           dominant axis of the normal for numeric stability. */
        axis = C_vec3_dominant(normal);
        q = C_vec2_from_3(p, axis);
        b = C_vec2_from_3(C_vec3_sub(triangle[1], triangle[0]), axis);
        c = C_vec2_from_3(C_vec3_sub(triangle[2], triangle[0]), axis);

        /* Find the barycentric coordinates */
        b_cross_c = C_vec2_cross(b, c);
        u = C_vec2_cross(q, c) / b_cross_c;
        v = C_vec2_cross(q, b) / -b_cross_c;

        /* Check if the point is within triangle bounds */
        if (u >= 0.f && v >= 0.f && u + v <= 1.f) {
                if (g_test_globe.value.n) {
                        p = C_vec3_add(p, triangle[0]);
                        R_render_test_line(p, C_vec3_add(p, normal),
                                           C_color(1.f, 0.f, 0.f, 1.f));
                }
                return TRUE;
        }

        return FALSE;
}

/******************************************************************************\
 Call when we know the mouse ray missed without or after tracing it.
\******************************************************************************/
void G_mouse_ray_miss(void)
{
        if (g_selected_tile < 0)
                return;
        g_tiles[g_selected_tile].model.selected = FALSE;
        R_select_tile(g_selected_tile = -1, R_ST_NONE);
}

/******************************************************************************\
 The mouse screen position is transformed into a ray with [origin] and
 [forward] vector, this function will find which tile (if any) the mouse is
 hovering over.
\******************************************************************************/
void G_mouse_ray(c_vec3_t origin, c_vec3_t forward)
{
        float tile_z, z;
        int i, tile;

        /* We can quit early if the selected tile is still being hovered over */
        if (g_selected_tile >= 0 && g_tiles[g_selected_tile].visible &&
            ray_intersects_tile(origin, forward, g_selected_tile)) {
                G_select_tile(g_selected_tile);
                return;
        }

        /* Disable selected tile effect for old model */
        if (g_selected_tile >= 0)
                g_tiles[g_selected_tile].model.selected = FALSE;

        /* Iterate over all visible tiles to find the selected tile */
        for (i = 0, tile = -1, tile_z = 0.f; i < r_tiles; i++) {
                if (!g_tiles[i].visible ||
                    !ray_intersects_tile(origin, forward, i))
                        continue;

                /* Make sure we only select the closest match */
                z = C_vec3_dot(r_cam_forward, g_tiles[i].origin);
                if (z < tile_z) {
                        tile = i;
                        tile_z = z;
                }
        }

        G_select_tile(tile);
}

