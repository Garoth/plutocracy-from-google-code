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

/* This is the largest value that pixels can be stretched. This means that all
   GUI textures need to be created this many times larger than normal. The
   largest supported resoluton is WQXGA 2560x1600 (nVidia Geforce 8800GT)
   which requires a pixel scale of 2 to have a 2D display area of approximately
   1280x800. */
#define R_PIXEL_SCALE_MIN 0.5f
#define R_PIXEL_SCALE_MAX 2.0f

/* The wide range of 2D scaling can shrink fonts too small to be viewable,
   so we need to set a minimum (in points). */
#define R_FONT_SIZE_MIN 10

/* Ranges for zooming in and out */
#define R_ZOOM_MIN 8.f
#define R_ZOOM_MAX 16.f

/* OpenGL cannot address enough vertices to render more than 5 subdivisons'
   worth of tiles */
#define R_TILES_MAX 20480

/* Opaque texture object */
typedef struct r_texture r_texture_t;

/* Model instance type */
typedef struct r_model {
        struct r_static_mesh *lerp_meshes;
        c_vec3_t origin, angles;
        struct r_model_data *data;
        float scale;
        int anim, frame, last_frame, last_frame_time, time_left,
            use_lerp_meshes;
} r_model_t;

/* 2D textured quad sprite, can only be rendered in 2D mode */
typedef struct r_sprite {
        r_texture_t *texture;
        c_vec2_t origin, size;
        c_color_t modulate;
        float angle;
} r_sprite_t;

/* There is a fixed set of fonts available for the game */
typedef enum {
        R_FONT_CONSOLE,
        R_FONT_GUI,
        R_FONTS
} r_font_t;

/* Sometimes it is convenient to store the source text for a text sprite in a
   generic buffer and only re-render it if it has changed */
typedef struct r_text {
        r_sprite_t sprite;
        r_font_t font;
        float wrap, shadow;
        int invert;
        char buffer[256];
} r_text_t;

/* A quad strip composed of nine quads that stretch with the size parameter
   of the sprite. Adjust sprite parameters to change how the window is
   displayed. */
typedef struct r_window {
        r_sprite_t sprite;
        c_vec2_t corner;
} r_window_t;

/* Structure that contains configuration parameters for a tile */
typedef struct r_tile {
        float height;
        int terrain;
} r_tile_t;

/* r_assets.c */
int R_font_height(r_font_t);
int R_font_line_skip(r_font_t);
c_vec2_t R_font_size(r_font_t, const char *);
int R_font_width(r_font_t);
void R_free_fonts(void);
void R_load_fonts(void);

/* r_globe.c */
void R_configure_globe(r_tile_t *array);
void R_generate_globe(int seed, int subdiv4);
void R_get_tile_coords(int index, c_vec3_t verts[3]);
void R_find_tile_neighbors(int index, int neighbors[3]);
float R_screen_to_globe(int pixels);

extern int r_tiles;

/* r_model.c */
void R_model_cleanup(r_model_t *);
int R_model_init(r_model_t *, const char *filename);
void R_model_play(r_model_t *, const char *anim_name);
void R_model_render(r_model_t *);

/* r_render.c */
void R_cleanup(void);
void R_clip_left(float);
void R_clip_top(float);
void R_clip_right(float);
void R_clip_bottom(float);
void R_clip_push(void);
void R_clip_pop(void);
void R_clip_rect(c_vec2_t origin, c_vec2_t size);
void R_clip_disable(void);
void R_finish_frame(void);
void R_init(void);
void R_move_cam_by(c_vec2_t);
void R_zoom_cam_by(float);
void R_start_frame(void);

extern c_count_t r_count_faces;
extern int r_width_2d, r_height_2d;

/* r_sprite.c */
void R_sprite_cleanup(r_sprite_t *);
void R_sprite_init(r_sprite_t *, const char *filename);
void R_sprite_init_text(r_sprite_t *, r_font_t, float wrap, float shadow,
                        int invert, const char *text);
void R_sprite_render(const r_sprite_t *);
#define R_text_init(t) C_zero(t)
void R_text_configure(r_text_t *, r_font_t, float wrap, float shadow,
                      int invert, const char *text);
#define R_text_cleanup(t) R_sprite_cleanup(&(t)->sprite)
#define R_text_render(t) R_sprite_render(&(t)->sprite)
#define R_window_cleanup(w) R_sprite_cleanup(&(w)->sprite)
void R_window_init(r_window_t *, const char *filename);
void R_window_render(r_window_t *);

/* r_variables.c */
void R_register_variables(void);

extern c_var_t r_pixel_scale;

