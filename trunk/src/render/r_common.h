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

/* Tile sheet grid parameters */
#define R_TILE_SHEET_W 6
#define R_TILE_SHEET_H 3

/* Proportion of a tile that is used as a buffer against its neighbors */
#define R_TILE_BORDER (6.f / 256)

/* Supported extensions */
typedef enum {
        R_EXT_MULTITEXTURE,
        R_EXT_POINT_SPRITE,
        R_EXT_ANISOTROPY,
        R_EXT_VERTEX_BUFFER,
        R_EXTENSIONS,
} r_extension_t;

/* Vertex type for meshes */
#pragma pack(push, 4)
typedef struct r_vertex3 {
        c_vec2_t uv;
        c_vec3_t no;
        c_vec3_t co;
} r_vertex3_t;
#pragma pack(pop)
#define R_VERTEX3_FORMAT GL_T2F_N3F_V3F

/* Vertex type for sprites */
#pragma pack(push, 4)
typedef struct r_vertex2 {
        c_vec2_t uv;
        c_vec3_t co;
} r_vertex2_t;
#pragma pack(pop)
#define R_VERTEX2_FORMAT GL_T2F_V3F

/* Texture class */
struct r_texture {
        c_ref_t ref;
        SDL_Surface *surface;
        GLuint gl_name;
        float anisotropy;
        int alpha, mipmaps, additive;
};

/* Render modes */
typedef enum {
        R_MODE_NONE,
        R_MODE_2D,
        R_MODE_3D,
} r_mode_t;

/* Structure to wrap OpenGL extensions */
typedef struct r_ext {
        PFNGLBINDBUFFERPROC glBindBuffer;
        PFNGLBUFFERDATAPROC glBufferData;
        PFNGLDELETEBUFFERSPROC glDeleteBuffers;
        PFNGLGENBUFFERSPROC glGenBuffers;
        PFNGLACTIVETEXTUREPROC glActiveTexture;
        GLfloat anisotropy;
        GLint multitexture;
        int point_sprites, vertex_buffers;
} r_ext_t;

/* Wrapper for vertex buffer objects */
typedef struct r_vbo {
        GLuint vertices_name, indices_name;
        void *vertices, *indices;
        int vertices_len, indices_len, init_frame, vertex_size, vertex_format;
} r_vbo_t;

/* r_assets.c */
void R_dealloc_textures(void);
SDL_Surface *R_font_render(r_font_t, const char *);
void R_free_assets(void);
void R_load_assets(void);
void R_realloc_textures(void);
#define R_texture_alloc(w, h, a) R_texture_alloc_full(__FILE__, __LINE__, \
                                                      __func__, w, h, a)
r_texture_t *R_texture_alloc_full(const char *file, int line, const char *func,
                                  int width, int height, int alpha);
#define R_texture_clone(t) R_texture_clone_full(__FILE__, __LINE__, \
                                                __func__, t)
r_texture_t *R_texture_clone_full(const char *file, int line, const char *func,
                                  const r_texture_t *);
void R_texture_render(r_texture_t *, int x, int y);
void R_texture_screenshot(r_texture_t *, int x, int y);
void R_texture_select(r_texture_t *);
void R_texture_upload(const r_texture_t *);
void R_vbo_cleanup(r_vbo_t *);
void R_vbo_init(r_vbo_t *, void *vertices, int vertices_len, int vertex_size,
                int vertex_format, void *indices, int indices_len);
void R_vbo_render(r_vbo_t *);
void R_vbo_update(r_vbo_t *);

extern r_texture_t *r_terrain_tex, *r_white_tex;
extern SDL_PixelFormat r_sdl_format;
extern int r_video_mem, r_video_mem_high;

/* r_camera.c */
void R_init_camera(void);
void R_update_camera(void);

extern GLfloat r_cam_matrix[16];

/* r_globe.c */
void R_cleanup_globe(void);
void R_init_globe(void);

extern c_color_t r_select_color;

/* r_mode.c */
#define R_check_errors() R_check_errors_full(__FILE__, __LINE__, __func__);
void R_check_errors_full(const char *file, int line, const char *func);
void R_gl_disable(GLenum);
void R_gl_enable(GLenum);
void R_gl_restore(void);
void R_pop_mode(void);
void R_push_mode(r_mode_t);
void R_set_mode(r_mode_t);

extern r_ext_t r_ext;
extern r_mode_t r_mode;
extern GLfloat r_proj_matrix[16];
extern int r_init_frame, r_mode_hold;

/* r_prerender.c */
void R_prerender(void);

/* r_solar.c */
void R_cleanup_solar(void);
void R_disable_light(void);
void R_enable_light(void);
void R_finish_atmosphere(void);
void R_generate_halo(void);
void R_init_solar(void);
void R_render_solar(void);
void R_start_atmosphere(void);

extern c_color_t r_fog_color;

/* r_surface.c */
SDL_Surface *R_surface_alloc(int width, int height, int alpha);
void R_surface_flip_v(SDL_Surface *);
c_color_t R_surface_get(const SDL_Surface *, int x, int y);
void R_surface_invert(SDL_Surface *, int rgb, int alpha);
SDL_Surface *R_surface_load_png(const char *filename, int *alpha);
void R_surface_mask(SDL_Surface *dest, SDL_Surface *src);
void R_surface_put(SDL_Surface *, int x, int y, c_color_t);
int R_surface_save(SDL_Surface *, const char *filename);

/* r_test.c */
void R_render_normals(int count, c_vec3_t *co, c_vec3_t *no, int stride);
void R_render_tests(void);

/* r_variables.c */
extern c_var_t r_clear, r_depth_bits, r_globe, r_globe_colors[4],
               r_atmosphere, r_globe_shininess, r_globe_smooth,
               r_globe_transitions, r_gl_errors, r_light, r_moon_atten,
               r_moon_colors[3], r_moon_height, r_solar, r_sun_colors[3],
               r_test_normals, r_test_sprite_num, r_test_sprite, r_test_model,
               r_test_prerender, r_test_text, r_vsync;

