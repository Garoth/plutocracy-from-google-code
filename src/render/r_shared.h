/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* This is the minimum FPS that model animations must maintain. If they are
   slower than this, frames are interpolated. */
#define R_MODEL_ANIM_FPS 30

/* Model instance type */
typedef struct r_model {
        struct r_static_mesh *lerp_meshes;
        c_vec3_t origin, angles;
        struct r_model_data *data;
        float scale;
        int anim, frame, last_frame, last_frame_time, time_left,
            use_lerp_meshes;
} r_model_t;

/* r_model.c */
void R_model_cleanup(r_model_t *);
int R_model_init(r_model_t *, const char *filename);
void R_model_play(r_model_t *, const char *anim_name);
void R_model_render(r_model_t *);

/* r_render.c */
void R_render(void);
void R_render_cleanup(void);
int R_render_init(void);

extern c_count_t r_count_faces;

/* r_variables.c */
void R_register_variables(void);

