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

/******************************************************************************\
 Initialize and position a tile's model. Returns FALSE if the model failed to
 load.
\******************************************************************************/
bool G_tile_model(int tile, const char *filename)
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
        g_tiles[tile].model_shown = TRUE;
        g_tiles[tile].fade = 1.f;
        return TRUE;
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
        building_class = g_building_classes + tile->building;
        I_quick_info_show(building_class->name);

        /* Owner */
        I_quick_info_add("Owner:", NULL);

        /* Island */
        I_quick_info_add("Island:", NULL);

        /* Island */
        I_quick_info_add("Terrain:",
                         R_terrain_to_string(r_tile_params[index].terrain));

        /* No building */
        if (tile->building == G_BT_NONE)
                return;

        /* Health */
        color = I_COLOR_ALT;
        prop = (float)tile->health / building_class->health;
        if (prop >= 0.67)
                color = I_COLOR_GOOD;
        if (prop <= 0.33)
                color = I_COLOR_BAD;
        I_quick_info_add_color("Health:", C_va("%d/%d", tile->health,
                                               building_class->health), color);
}

/******************************************************************************\
 Selects a tile.
\******************************************************************************/
void G_tile_select(int tile)
{
        r_terrain_t terrain;

        if (g_selected_tile == tile)
                return;

        /* Can't select water */
        if (tile >= 0) {
                terrain = R_terrain_base(r_tile_params[tile].terrain);
                if (terrain != R_T_SAND && terrain != R_T_GROUND)
                        return;
        }

        /* Deselect previous tile */
        if (g_selected_tile >= 0)
                g_tiles[g_selected_tile].model.selected = R_MS_NONE;

        /* Select the new tile */
        if ((g_selected_tile = tile) >= 0) {
                g_tiles[tile].model.selected = R_MS_SELECTED;
                R_hover_tile(-1, R_ST_NONE);
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

        C_assert(tile < r_tiles);
        new_select_type = R_ST_NONE;
        terrain = R_terrain_base(r_tile_params[tile].terrain);

        /* Selecting a tile to move the current ship to */
        if (G_open_tile(tile, -1) &&
            G_ship_controlled_by(g_selected_ship, n_client_id))
                new_select_type = R_ST_GOTO;

        /* Selecting an island tile */
        else if (tile >= 0 && (terrain == R_T_SAND || terrain == R_T_GROUND)) {
                if (tile != g_selected_tile)
                        new_select_type = R_ST_TILE;
        }

        /* Selecting a ship */
        else if (g_tiles[tile].ship >= 0);

        /* Can't select this tile */
        else
                tile = -1;

        /* Still hovering over the same tile */
        if (tile == g_hover_tile && new_select_type == select_type) {
                G_ship_hover(tile >= 0 ? g_tiles[tile].ship : -1);

                /* Tile can get deselected for whatever reason */
                if (select_type != R_ST_NONE &&
                    g_tiles[tile].model.selected == R_MS_NONE)
                        g_tiles[tile].model.selected = R_MS_HOVER;
                return;
        }

        /* Deselect the old tile */
        if (g_hover_tile >= 0 && g_tiles[g_hover_tile].ship < 0 &&
            g_tiles[g_hover_tile].model.selected == R_MS_HOVER)
                g_tiles[g_hover_tile].model.selected = R_MS_NONE;

        /* Apply new hover */
        select_type = new_select_type;
        R_hover_tile(tile, select_type);
        if ((g_hover_tile = tile) < 0 || new_select_type == R_ST_NONE) {
                G_ship_hover(-1);
                return;
        }
        if (g_tiles[tile].ship >= 0) {
                G_ship_hover(g_tiles[tile].ship);
                return;
        }

        /* Select the tile building model if there is no ship there */
        if (select_type != R_ST_NONE &&
            g_tiles[tile].model.selected == R_MS_NONE)
                g_tiles[tile].model.selected = R_MS_HOVER;
}

