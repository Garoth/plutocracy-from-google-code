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

/* Vertex type for meshes */
#pragma pack(push, 4)
typedef struct r_vertex {
        c_vec2_t uv;
        c_vec3_t no;
        c_vec3_t co;
} r_vertex_t;
#define R_VERTEX_FORMAT GL_T2F_N3F_V3F
#pragma pack(pop)

/* Non-animated, untextured mesh */
typedef struct r_static_mesh {
        r_vertex_t *verts;
        int verts_len, indices_len;
        unsigned short *indices;
} r_static_mesh_t;

/* Texture class */
typedef struct r_texture {
        struct r_texture *prev, *next;
        SDL_Surface *surface;
        int refs;
        char filename[256];
} r_texture_t;

/* Animated, textured, multi-mesh model */
typedef struct r_model {
        /* TODO */
} r_model_t;

/* r_static_mesh.c */
r_static_mesh_t *R_static_mesh_load(const char *filename);
void R_static_mesh_render(r_static_mesh_t *);
void R_static_mesh_free(r_static_mesh_t *);

/* r_model.c */
r_model_t *R_model_load(const char *filename);
void R_model_free(r_model_t *model);

/* r_variables.c */
extern c_var_t r_width, r_height, r_colordepth, r_depth, r_windowed, r_vsync;

