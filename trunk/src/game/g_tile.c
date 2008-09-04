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

/* Island tiles with game data */
g_tile_t g_tiles[R_TILES_MAX];

/* The tile the mouse is hovering over and the currently selected tile */
int g_hover_tile, g_selected_tile;

/* Number of gibs on the globe */
int g_gibs;

/******************************************************************************\
 Cleanup a building structure.
\******************************************************************************/
static void building_free(g_building_t *building)
{
        if (!building)
                return;
        R_model_cleanup(&building->model);
        C_free(building);
}

/******************************************************************************\
 Cleanup a gib structure.
\******************************************************************************/
static void gib_free(g_gib_t *gib)
{
        if (!gib)
                return;
        R_model_cleanup(&gib->model);
        C_free(gib);
        g_gibs--;
}

/******************************************************************************\
 Cleanup a tile structures.
\******************************************************************************/
void G_cleanup_tiles(void)
{
        int i;

        for (i = 0; i < r_tiles_max; i++) {
                building_free(g_tiles[i].building);
                gib_free(g_tiles[i].gib);
                C_zero(g_tiles + i);
        }
}

/******************************************************************************\
 Setup tile quick info.
\******************************************************************************/
static void tile_quick_info(int index)
{
        g_tile_t *tile;
        g_building_class_t *building_class;
        i_color_t color;
        float prop;

        if (index < 0) {
                I_quick_info_close();
                return;
        }
        tile = g_tiles + index;
        if (tile->building) {
                building_class = g_building_classes + tile->building->type;
                I_quick_info_show(building_class->name, &r_tiles[index].origin);
        } else
                I_quick_info_show(g_building_classes[G_BT_NONE].name,
                                  &r_tiles[index].origin);

        /* Terrain */
        I_quick_info_add("Terrain:",
                         R_terrain_to_string(r_tiles[index].terrain));

        /* No building */
        if (!tile->building)
                return;

        /* Health */
        color = I_COLOR_ALT;
        prop = (float)tile->building->health / building_class->health;
        if (prop >= 0.67)
                color = I_COLOR_GOOD;
        if (prop <= 0.33)
                color = I_COLOR_BAD;
        I_quick_info_add_color("Health:", C_va("%d/%d", tile->building->health,
                                               building_class->health), color);
}

/******************************************************************************\
 Set the selection state of a tile's building. Will not change a selected
 model's selection state if [protect_selection] is TRUE.
\******************************************************************************/
static void tile_building_select(int tile, r_model_select_t select,
                                 bool protect_selection)
{
        g_building_t *building;

        if (tile < 0 || tile >= r_tiles_max)
                return;
        building = g_tiles[tile].building;
        if (!building || (protect_selection &&
                          building->model.selected == R_MS_SELECTED))
                return;
        building->model.selected = select;
}

/******************************************************************************\
 Selects a tile.
\******************************************************************************/
void G_tile_select(int tile)
{
        g_building_t *building;
        r_terrain_t terrain;

        if (g_selected_tile == tile)
                return;
        building = g_tiles[tile].building;

        /* Can't select water */
        if (tile >= 0) {
                terrain = R_terrain_base(r_tiles[tile].terrain);
                if (terrain != R_T_SAND && terrain != R_T_GROUND)
                        return;
        }

        /* Deselect previous tile */
        tile_building_select(g_selected_tile, R_MS_NONE, FALSE);

        /* Select the new tile */
        if ((g_selected_tile = tile) >= 0) {
                R_hover_tile(-1, R_ST_NONE);
                tile_building_select(tile, R_MS_SELECTED, FALSE);
        }

        R_select_tile(tile, R_ST_TILE);
        tile_quick_info(tile);
}

/******************************************************************************\
 Updates which tile the mouse is hovering over. Pass -1 to turn off.
\******************************************************************************/
void G_tile_hover(int tile)
{
        static r_select_type_t select_type;
        r_select_type_t new_select_type;
        r_terrain_t terrain;

        C_assert(tile < r_tiles_max);
        new_select_type = R_ST_NONE;
        if (tile >= 0)
                terrain = R_terrain_base(r_tiles[tile].terrain);

        /* Selecting a tile to move the current ship to */
        if (G_ship_controlled_by(g_selected_ship, n_client_id) &&
            G_tile_open(tile, -1) && !g_game_over)
                new_select_type = R_ST_GOTO;

        /* Selecting an island tile */
        else if (tile >= 0 && (terrain == R_T_SAND || terrain == R_T_GROUND)) {
                if (tile != g_selected_tile)
                        new_select_type = R_ST_TILE;
        }

        /* If there is no ship, can't select this tile */
        else if (g_tiles[tile].ship < 0)
                tile = -1;

        /* Still hovering over the same tile */
        if (tile == g_hover_tile && new_select_type == select_type) {
                G_ship_hover(tile >= 0 ? g_tiles[tile].ship : -1);

                /* Tile can get deselected for whatever reason */
                if (select_type == R_ST_TILE)
                        tile_building_select(tile, R_MS_HOVER, TRUE);
                return;
        }

        /* Deselect the old tile */
        tile_building_select(g_hover_tile, R_MS_NONE, TRUE);

        /* Apply new hover */
        R_hover_tile(tile, (select_type = new_select_type));
        if ((g_hover_tile = tile) < 0 || new_select_type == R_ST_NONE) {
                G_ship_hover(-1);
                return;
        }
        if (g_tiles[tile].ship >= 0) {
                G_ship_hover(g_tiles[tile].ship);
                return;
        }

        /* Select the tile building model if there is no ship there */
        if (select_type != R_ST_NONE)
                tile_building_select(tile, R_MS_HOVER, TRUE);
}

/******************************************************************************\
 Position a model on this tile.
\******************************************************************************/
void G_tile_position_model(int tile, r_model_t *model)
{
        if (!model)
                return;
        model->forward = r_tiles[tile].forward;
        model->origin = r_tiles[tile].origin;
        model->normal = r_tiles[tile].normal;
}

/******************************************************************************\
 Start constructing a building on this tile.
\******************************************************************************/
void G_tile_build(int tile, g_building_type_t type, g_nation_name_t nation)
{
        g_building_t *building;

        /* Range checks */
        if (tile < 0 || tile >= r_tiles_max ||
            type < 0 || type >= G_BUILDING_TYPES)
                return;

        building_free(g_tiles[tile].building);

        /* "None" building type is special */
        if (type == G_BT_NONE)
                g_tiles[tile].building = NULL;

        /* Allocate and initialize a building structure */
        else {
                building = C_malloc(sizeof (*g_tiles[tile].building));
                building->type = type;
                building->nation = nation;
                building->health = g_building_classes[type].health;
                R_model_init(&building->model,
                             g_building_classes[type].model_path, TRUE);
                G_tile_position_model(tile, &building->model);
                g_tiles[tile].building = building;

                /* Start out selected */
                if (g_selected_tile == tile)
                        building->model.selected = R_MS_SELECTED;
        }

        /* If we just built a new town hall, update the island */
        if (type == G_BT_TOWN_HALL)
                g_islands[g_tiles[tile].island].town_tile = tile;

        /* Let all connected clients know about this */
        if (!g_host_inited)
                return;
        N_broadcast_except(N_HOST_CLIENT_ID, "1211", G_SM_BUILDING,
                           tile, type, nation);
}

/******************************************************************************\
 Returns TRUE if a ship can sail into the given tile. If [exclude_ship] is
 non-negative then the tile is still considered open if [exclude_ship] is
 in it.
\******************************************************************************/
bool G_tile_open(int tile, int exclude_ship)
{
        return (g_tiles[tile].ship < 0 ||
                (exclude_ship >= 0 && g_tiles[tile].ship == exclude_ship)) &&
               R_water_terrain(r_tiles[tile].terrain);
}

/******************************************************************************\
 Pick a random open tile. Returns -1 if the globe is completely full.
\******************************************************************************/
int G_random_open_tile(void)
{
        int start, tile;

        start = C_rand() % r_tiles_max;
        for (tile = start + 1; tile < r_tiles_max; tile++)
                if (G_tile_open(tile, -1))
                        return tile;
        for (tile = 0; tile <= start; tile++)
                if (G_tile_open(tile, -1))
                        return tile;

        /* If this happens, the globe is completely full! */
        C_warning("Globe is full");
        return -1;
}

/******************************************************************************\
 Spawn gibs on this tile. Returns the selected tile.
\******************************************************************************/
int G_tile_gib(int tile, g_gib_type_t type)
{
        /* Pick a random tile */
        if (tile < 0)
                tile = G_random_open_tile();
        if (tile < 0)
                return -1;

        gib_free(g_tiles[tile].gib);
        if (type != G_GT_NONE) {
                g_gibs++;
                g_tiles[tile].gib = (g_gib_t *)C_calloc(sizeof (g_gib_t));
                g_tiles[tile].gib->type = type;

                /* Initialize the model */
                R_model_init(&g_tiles[tile].gib->model,
                             "models/gib/crates.plum", TRUE);
                G_tile_position_model(tile, &g_tiles[tile].gib->model);
        } else
                g_tiles[tile].gib = NULL;

        /* Let all connected clients know about this gib */
        if (g_host_inited)
                N_broadcast_except(N_HOST_CLIENT_ID, "121",
                                   G_SM_GIB, tile, type);

        return tile;
}

