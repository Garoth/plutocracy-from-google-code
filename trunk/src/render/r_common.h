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
typedef struct r_vertex3 {
        c_vec2_t uv;
        c_vec3_t no;
        c_vec3_t co;
} r_vertex3_t;
#define R_VERTEX3_FORMAT GL_T2F_N3F_V3F
#pragma pack(pop)

/* Vertex type for sprites */
#pragma pack(push, 4)
typedef struct r_vertex2 {
        c_vec2_t uv;
        c_vec3_t co;
} r_vertex2_t;
#define R_VERTEX2_FORMAT GL_T2F_V3F
#pragma pack(pop)

/* Texture class */
typedef struct r_texture {
        c_ref_t ref;
        SDL_Surface *surface;
        GLuint gl_name;
        int alpha;
} r_texture_t;

/* Non-animated mesh */
typedef struct r_static_mesh {
        r_vertex3_t *verts;
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
typedef struct r_model_data {
        c_ref_t ref;
        r_static_mesh_t *matrix;
        r_model_anim_t *anims;
        r_model_object_t *objects;
        int anims_len, objects_len, frames;
} r_model_data_t;

/* Render modes */
typedef enum {
        R_MODE_NONE,
        R_MODE_3D,
        R_MODE_2D,
} r_mode_t;

/* r_assets.c */
void R_free_assets(void);
void R_load_assets(void);
#define R_texture_free(t) C_ref_down((c_ref_t *)(t))
r_texture_t *R_texture_load(const char *filename);
#define R_texture_ref(t) C_ref_up((c_ref_t *)(t))
void R_texture_select(r_texture_t *);

/* r_render.c */
#define R_check_errors() R_check_errors_full(__FILE__, __LINE__, __func__);
void R_check_errors_full(const char *file, int line, const char *func);
void R_set_mode(r_mode_t);

extern r_mode_t r_mode;

/* r_static_mesh.c */
void R_static_mesh_cleanup(r_static_mesh_t *);
unsigned short R_static_mesh_find_vert(const r_static_mesh_t *mesh,
                                       const r_vertex3_t *vert);
r_static_mesh_t *R_static_mesh_load(const char *filename);
void R_static_mesh_render(r_static_mesh_t *, r_texture_t *);
void R_static_mesh_free(r_static_mesh_t *);

/* r_text.c */
void R_cleanup_fonts(void);
void R_init_fonts(void);

/* r_variables.c */
extern c_var_t r_color_bits, r_depth_bits, r_gamma, r_height, r_width, r_vsync,
               r_windowed, r_pixel_scale;

