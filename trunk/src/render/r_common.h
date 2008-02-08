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

/* Texture class */
typedef struct r_texture {
        struct r_texture *prev, *next;
        SDL_Surface *surface;
        GLuint gl_name;
        int refs, format;
        char filename[256];
} r_texture_t;

/* Non-animated mesh */
typedef struct r_static_mesh {
        r_texture_t *texture;
        r_vertex_t *verts;
        unsigned short *indices;
        int verts_len, indices_len;
} r_static_mesh_t;

/* Model animation type */
typedef struct r_model_anim {
        int from, to, delay;
        char name[64], end_anim[64];
} r_model_anim_t;

/* Model object type */
typedef struct r_model_object {
        r_texture_t *texture;
        char name[64];
} r_model_object_t;

/* Animated, textured, multi-mesh model. The matrix contains enough room to
   store every object's static mesh for every frame, it is indexed by frame
   then by object. */
typedef struct r_model {
        r_static_mesh_t lerp_mesh, *matrix;
        r_model_anim_t *anims;
        r_model_object_t *objects;
        int anims_len, objects_len, anim, frame, frames, time_left;
        char filename[32];
} r_model_t;

/* r_assets.c */
void R_texture_free(r_texture_t *);
r_texture_t *R_texture_load(const char *filename);
void R_texture_ref(r_texture_t *);

/* r_model.c */
void R_model_free(r_model_t *);
r_model_t *R_model_load(const char *filename);
void R_model_play(r_model_t *, const char *anim_name);
void R_model_render(r_model_t *);

/* r_static_mesh.c */
void R_static_mesh_cleanup(r_static_mesh_t *);
r_static_mesh_t *R_static_mesh_load(const char *filename);
void R_static_mesh_render(r_static_mesh_t *);
void R_static_mesh_free(r_static_mesh_t *);

/* r_variables.c */
extern c_var_t r_width, r_height, r_colordepth, r_depth, r_windowed, r_vsync;

