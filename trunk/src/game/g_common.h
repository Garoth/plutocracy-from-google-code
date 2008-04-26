/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "../common/c_shared.h"
#include "../render/r_shared.h"
#include "g_shared.h"

/* Terrain enumeration */
typedef enum {
        G_T_WATER = 1,
        G_T_SHALLOW = 2,
        G_T_SAND = 4,
        G_T_GROUND = 5,
        G_T_GROUND_HOT = 6,
        G_T_GROUND_COLD = 7,
} g_terrain_t;

/* A tile on the globe */
typedef struct g_tile {
        struct g_tile *neighbors[3];
        r_tile_t *render;
        int island;
} g_tile_t;

/* g_globe.c */
c_var_t g_globe_seed, g_globe_subdiv4;

