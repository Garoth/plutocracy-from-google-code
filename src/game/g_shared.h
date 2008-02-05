/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin, Devin Papineau

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Type to hold neighbours of a vertex */
typedef struct g_vert_neighbors {
    int count;
    unsigned short indices[6]; /* Never more than 6 neighbours. */
} g_vert_neighbors_t;

/* Type to hold the spherical map */
typedef struct g_sphere {
    int nverts;
    int ninds;
    c_vec3_t *verts;
    g_vert_neighbors_t *neighbors_lists;
    unsigned short *inds; /* triangles if you want 'em */
} g_sphere_t;

/* g_sphere.c */
g_sphere_t *G_sphere_alloc(int subdiv_levels);
