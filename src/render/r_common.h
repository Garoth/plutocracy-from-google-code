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
#include "r_shared.h"

typedef struct r_static_mesh {
    unsigned short ninds;
    unsigned short nverts;
    c_pt3_t* verts;
    c_pt3_t* norms;
    c_pt2_t* sts;
    unsigned short* inds;
} r_static_mesh_t;

/* r_static_mesh.c */
r_static_mesh_t* R_load_static_mesh(const char* filename);
void R_render_static_mesh(r_static_mesh_t*);

