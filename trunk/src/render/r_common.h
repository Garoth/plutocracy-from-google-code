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
        c_vec3_t *verts;
        c_vec3_t *norms;
        c_vec2_t *sts;
        unsigned short *inds;
} r_static_mesh_t;

#pragma pack(push, 4)

typedef struct r_vertex {
        float tu, tv, nx, ny, nz, x, y, z;
} r_vertex_t;

#define R_VERTEX_FLAGS GL_T2F_N3F_V3F
#pragma pack(pop)

typedef struct r_model {
        r_vertex_t *verts;
        unsigned short *indices;
        unsigned int verts_len, indices_len;
} r_model_t;

/* r_static_mesh.c */
r_static_mesh_t *R_static_mesh_load(const char *filename);
void R_static_mesh_render(r_static_mesh_t *);
void R_static_mesh_free(r_static_mesh_t *);

/* r_variables.c */
extern c_var_t r_width, r_height, r_colordepth, r_depth, r_windowed, r_vsync;

