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
#define ISLAND_HEIGHT 4.f

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
static int islands_len, selected_tile;

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
        g_tiles[tile].model.selected = selected_tile == tile;
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
 One-time globe initialization. Call after rendering has been initialized.
\******************************************************************************/
void G_init_globe(void)
{
        selected_tile = -1;

        /* Generate a starter globe */
        G_generate_globe();
}

/******************************************************************************\
 Generate a new globe.
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
 Render the globe and updates tile visibility.
\******************************************************************************/
void G_render_globe(void)
{
        int i;

        R_start_globe();

        /* Render tile models */
        for (i = 0; i < r_tiles; i++)
                if ((g_tiles[i].visible = tile_visible(i))) {
                        R_adjust_light_for(g_tiles[i].origin);
                        R_model_render(&g_tiles[i].model);
                }

        R_finish_globe();

        /* Render a test line from the selected tile */
        if (g_test_globe.value.n && selected_tile >= 0) {
                c_vec3_t b;

                b = C_vec3_add(g_tiles[selected_tile].origin,
                               g_tiles[selected_tile].normal);
                R_render_test_line(g_tiles[selected_tile].origin, b,
                                   C_color(0.f, 1.f, 0.f, 1.f));
        }
}

/******************************************************************************\
 Returns TRUE if the given ray intersects the tile.

 The algorithm used here projects the ray onto the triangle's plane and
 tests the barycentric coordinates of the intersection point to determine
 whether the triangle was hit. Source:
 http://www.devmaster.net/wiki/Ray-triangle_intersection
\******************************************************************************/
static int ray_intersects_tile(c_vec3_t O, c_vec3_t D, int tile)
{
        c_vec3_t P, triangle[3];
        c_vec2_t Q, B, C;
        float t, u, v, b_cross_c;
        int axis;

        R_get_tile_coords(tile, triangle);

        /* Find [P], the ray's location in the triangle plane */
        O = C_vec3_sub(O, triangle[0]);
        t = C_vec3_dot(g_tiles[tile].normal, O) /
            -C_vec3_dot(g_tiles[tile].normal, D);
        if (t <= 0.f)
                return FALSE;
        P = C_vec3_add(O, C_vec3_scalef(D, t));

        /* Project the points onto one axis to simplify calculations. Choose the
           dominant axis of the normal for numeric stability. */
        axis = C_vec3_dominant(g_tiles[tile].normal);
        Q = C_vec2_from_3(P, axis);
        B = C_vec2_from_3(C_vec3_sub(triangle[1], triangle[0]), axis);
        C = C_vec2_from_3(C_vec3_sub(triangle[2], triangle[0]), axis);

        /* Find the barycentric coordinates */
        b_cross_c = C_vec2_cross(B, C);
        u = C_vec2_cross(Q, C) / b_cross_c;
        v = C_vec2_cross(Q, B) / -b_cross_c;

        /* Check if the point is within triangle bounds */
        if (u >= 0.f && v >= 0.f && u + v <= 1.f) {
                if (g_test_globe.value.n) {
                        P = C_vec3_add(P, triangle[0]);
                        R_render_test_line(P,
                                           C_vec3_add(P, g_tiles[tile].normal),
                                           C_color(1.f, 0.f, 0.f, 1.f));
                }
                return TRUE;
        }
        return FALSE;
}

/******************************************************************************\
 Test ring callback function.
\******************************************************************************/
static void test_ring_callback(i_ring_icon_t icon)
{
        if (icon == I_RI_TEST_MILL)
                set_tile_model(selected_tile, "models/test/mill.plum");
        else if (icon == I_RI_TEST_TREE)
                set_tile_model(selected_tile,
                               "models/environment/tree-deciduous.plum");
        else if (icon == I_RI_TEST_SHIP)
                set_tile_model(selected_tile,
                               "models/ships/ship-sloop.plum");
        else
                set_tile_model(selected_tile, "");
}

/******************************************************************************\
 Call when we know the mouse ray missed without or after tracing it.
\******************************************************************************/
void G_mouse_ray_miss(void)
{
        R_select_tile(selected_tile = -1);
}

/******************************************************************************\
 The mouse screen position is transformed into a ray with [origin] and
 [forward] vector, this function will find which tile (if any) the mouse is
 hovering over.
\******************************************************************************/
void G_mouse_ray(c_vec3_t origin, c_vec3_t forward)
{
        float selected_z, z;
        int i;

        /* We can quit early if the selected tile is still being hovered over */
        if (selected_tile >= 0 && g_tiles[selected_tile].visible &&
            ray_intersects_tile(origin, forward, selected_tile)) {
                g_tiles[selected_tile].model.selected = TRUE;
                return;
        }

        /* Disable selected tile effect for old model */
        if (selected_tile >= 0)
                g_tiles[selected_tile].model.selected = FALSE;

        /* Iterate over all visible tiles to find the selected tile */
        selected_z = 0.f;
        R_select_tile(selected_tile = -1);
        for (i = 0; i < r_tiles; i++) {
                if (!g_tiles[i].visible ||
                    !ray_intersects_tile(origin, forward, i))
                        continue;

                /* Make sure we only select the closest match */
                z = C_vec3_dot(r_cam_forward, g_tiles[i].origin);
                if (z < selected_z) {
                        selected_tile = i;
                        selected_z = z;
                }
        }
        if (selected_tile < 0)
                return;

        R_select_tile(selected_tile);
        g_tiles[selected_tile].model.selected = TRUE;
}

/******************************************************************************\
 Called when the interface root window receives a click.
\******************************************************************************/
void G_process_click(int button)
{
        if (selected_tile < 0)
                return;

        /* If we clicked on a valid tile, popup the ring menu */
        I_reset_ring();
        I_add_to_ring(I_RI_TEST_BLANK, TRUE);
        I_add_to_ring(I_RI_TEST_MILL, TRUE);
        I_add_to_ring(I_RI_TEST_TREE, TRUE);
        I_add_to_ring(I_RI_TEST_SHIP, TRUE);
        I_add_to_ring(I_RI_TEST_DISABLED, FALSE);
        I_show_ring((i_ring_f)test_ring_callback);
}

