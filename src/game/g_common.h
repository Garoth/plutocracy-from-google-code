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

/* A tile on the globe */
typedef struct g_tile {
        c_vec3_t origin, normal;
        r_model_t model;
        r_tile_t *render;
        struct g_tile *neighbors[3];
        float angle;
        int island;
} g_tile_t;

/* g_globe.c */
extern c_var_t g_globe_seed, g_globe_subdiv4;

/* g_variables.c */
extern c_var_t g_test_tile;

