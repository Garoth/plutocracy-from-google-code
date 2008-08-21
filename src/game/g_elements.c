/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Initializes, cleans up, and manipulates gameplay elements */

#include "g_common.h"

/* Ship base classes */
g_ship_class_t g_ship_classes[G_SHIP_TYPES];

/* Array of game nations */
g_nation_t g_nations[G_NATION_NAMES];

/* Array of building base classes */
g_building_class_t g_building_classes[G_BUILDING_TYPES];

/* Array of cargo names */
const char *g_cargo_names[G_CARGO_TYPES];

/******************************************************************************\
 Update function for a national color.
\******************************************************************************/
static int nation_color_update(c_var_t *var, c_var_value_t value)
{
        C_color_update(var, value);
        I_update_colors();
        return TRUE;
}

/******************************************************************************\
 Convert a nation to a color index.
\******************************************************************************/
i_color_t G_nation_to_color(g_nation_name_t nation)
{
        if (nation == G_NN_RED)
                return I_COLOR_RED;
        else if (nation == G_NN_GREEN)
                return I_COLOR_GREEN;
        else if (nation == G_NN_BLUE)
                return I_COLOR_BLUE;
        else if (nation == G_NN_PIRATE)
                return I_COLOR_PIRATE;
        return I_COLOR_ALT;
}

/******************************************************************************\
 Initialize game elements.
\******************************************************************************/
void G_init_elements(void)
{
        g_ship_class_t *sc;
        g_building_class_t *bc;
        int i;

        C_status("Initializing game elements");

        /* Check our constants */
        C_assert(G_CLIENT_MESSAGES < 256);
        C_assert(G_SERVER_MESSAGES < 256);

        /* Nation constants */
        g_nations[G_NN_NONE].short_name = "";
        g_nations[G_NN_NONE].long_name = "";
        g_nations[G_NN_RED].short_name = "red";
        g_nations[G_NN_RED].long_name = C_str("g-nation-red", "Ruby");
        g_nations[G_NN_GREEN].short_name = "green";
        g_nations[G_NN_GREEN].long_name = C_str("g-nation-green", "Emerald");
        g_nations[G_NN_BLUE].short_name = "blue";
        g_nations[G_NN_BLUE].long_name = C_str("g-nation-blue", "Sapphire");
        g_nations[G_NN_PIRATE].short_name = "pirate";
        g_nations[G_NN_PIRATE].long_name = C_str("g-nation-pirate", "Pirate");

        /* Initialize nation window and color variables */
        for (i = G_NN_NONE + 1; i < G_NATION_NAMES; i++)
                C_var_update_data(g_nation_colors + i, nation_color_update,
                                  &g_nations[i].color);

        /* Special cargo */
        g_cargo_names[G_CT_GOLD] = C_str("g-cargo-gold", "Gold");
        g_cargo_names[G_CT_CREW] = C_str("g-cargo-crew", "Crew");

        /* Food cargo */
        g_cargo_names[G_CT_RATIONS] = C_str("g-cargo-rations", "Rations");
        g_cargo_names[G_CT_BREAD] = C_str("g-cargo-bread", "Bread");
        g_cargo_names[G_CT_FISH] = C_str("g-cargo-fish", "Fish");
        g_cargo_names[G_CT_FRUIT] = C_str("g-cargo-fruit", "Fruit");

        /* Raw material cargo */
        g_cargo_names[G_CT_GRAIN] = C_str("g-cargo-grain", "Grain");
        g_cargo_names[G_CT_COTTON] = C_str("g-cargo-cotton", "Cotton");
        g_cargo_names[G_CT_LUMBER] = C_str("g-cargo-lumber", "Lumber");
        g_cargo_names[G_CT_IRON] = C_str("g-cargo-iron", "Iron");

        /* Luxury cargo */
        g_cargo_names[G_CT_RUM] = C_str("g-cargo-rum", "Rum");
        g_cargo_names[G_CT_FURNITURE] = C_str("g-cargo-furniture", "Furniture");
        g_cargo_names[G_CT_CLOTH] = C_str("g-cargo-cloth", "Cloth");
        g_cargo_names[G_CT_SUGAR] = C_str("g-cargo-sugar", "Sugar");

        /* Equipment cargo */
        g_cargo_names[G_CT_CANNON] = C_str("g-cargo-cannon", "Cannon");
        g_cargo_names[G_CT_ROUNDSHOT] = C_str("g-cargo-roundshot", "Ammo");
        g_cargo_names[G_CT_PLATING] = C_str("g-cargo-plating", "Plating");
        g_cargo_names[G_CT_GILLNET] = C_str("g-cargo-gillner", "Gillnet");

        /* Nothing built */
        bc = g_building_classes + G_BT_NONE;
        bc->name = "Empty";
        bc->model_path = "";

        /* Trees */
        bc = g_building_classes + G_BT_TREE;
        bc->name = "Trees";
        bc->model_path = "models/tree/deciduous.plum";
        bc->health = 100;

        /* Town hall */
        bc = g_building_classes + G_BT_TOWN_HALL;
        bc->name = "Town Hall";
        bc->health = 250;
        bc->model_path = "models/test/mill.plum";
        bc->cost.cargo[G_CT_GOLD] = 500;
        bc->cost.cargo[G_CT_LUMBER] = 100;

        /* Sloop */
        sc = g_ship_classes + G_ST_SLOOP;
        sc->name = C_str("g-ship-sloop", "Sloop");
        sc->model_path = "models/ship/sloop.plum";
        sc->speed = 1.f;
        sc->health = 40;
        sc->cargo = 100;

        /* Spider */
        sc = g_ship_classes + G_ST_SPIDER;
        sc->name = C_str("g-ship-spider", "Spider");
        sc->model_path = "models/ship/spider.plum";
        sc->speed = 0.75f;
        sc->health = 80;
        sc->cargo = 250;

        /* Galleon */
        sc = g_ship_classes + G_ST_GALLEON;
        sc->name = C_str("g-ship-galleon", "Galleon");
        sc->model_path = "models/ship/galleon.plum";
        sc->speed = 0.5f;
        sc->health = 100;
        sc->cargo = 400;
}

/******************************************************************************\
 Converts a cost to a build time.
\******************************************************************************/
int G_build_time(const g_cost_t *cost)
{
        g_cost_t delay;
        int i, time;

        if (!cost)
                return 0;
        delay = *cost;
        delay.cargo[G_CT_GOLD] /= 100;
        for (time = i = 0; i < G_CARGO_TYPES; i++)
                time += delay.cargo[i] * 100;
        return time;
}

/******************************************************************************\
 Reset game structures.
\******************************************************************************/
void G_reset_elements(void)
{
        int i;

        g_host_inited = FALSE;

        /* Reset ships */
        memset(g_ships, 0, sizeof (g_ships));

        /* Reset clients, keeping names */
        for (i = 0; i < N_CLIENTS_MAX; i++)
                g_clients[i].nation = G_NN_NONE;

        /* Reset tiles */
        for (i = 0; i < r_tiles; i++)
                G_tile_build(i, G_BT_NONE, 0.f);

        /* The server "client" has fixed information */
        g_clients[N_SERVER_ID].nation = G_NN_PIRATE;

        /* We can reuse names now */
        G_reset_name_counts();

        /* Start out without a selected ship */
        G_ship_select(-1);

        /* Not hovering over anything */
        g_hover_ship = g_hover_tile = -1;
}

