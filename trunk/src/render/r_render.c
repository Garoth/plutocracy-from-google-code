/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "r_common.h"

/* Height limit of the clipping stack */
#define CLIP_STACK 32

extern c_var_t r_clear, r_gl_errors,
               r_test_model_path, r_test_sprite_num, r_test_sprite_path,
               r_test_text;

/* Keep track of how many faces we render each frame */
c_count_t r_count_faces;

/* Current OpenGL settings */
r_mode_t r_mode;
int r_width_2d, r_height_2d;

/* Camera rotation and zoom */
c_vec3_t r_camera;

/* Testing assets */
static r_model_t test_model;
static r_sprite_t *test_sprites;
static r_text_t test_text;

/* Clipping stack */
static int clip_stack;
static float clip_values[CLIP_STACK * 4];

/******************************************************************************\
 Sets the video mode. SDL creates a window in windowed mode.
\******************************************************************************/
static int set_video_mode(void)
{
        SDL_Surface *screen;
        int flags;

        C_var_unlatch(&r_vsync);
        C_var_unlatch(&r_depth_bits);
        C_var_unlatch(&r_color_bits);
        C_var_unlatch(&r_windowed);
        C_var_unlatch(&r_width);
        C_var_unlatch(&r_height);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, r_depth_bits.value.n);
        SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, r_vsync.value.n);
        if (r_color_bits.value.n <= 16) {
                SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
                SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
                SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
        } else {
                SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
                SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
                SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        }
        flags = SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE |
                SDL_HWSURFACE;
        if (!r_windowed.value.n)
                flags |= SDL_FULLSCREEN;
        screen = SDL_SetVideoMode(r_width.value.n, r_height.value.n, 0, flags);
        if (!screen) {
                C_warning("Failed to set video mode: %s", SDL_GetError());
                return FALSE;
        }

        /* The 2D dimensions are scaled */
        C_var_unlatch(&r_pixel_scale);
        if (r_pixel_scale.value.f < R_PIXEL_SCALE_MIN)
                r_pixel_scale.value.f = R_PIXEL_SCALE_MIN;
        if (r_pixel_scale.value.f > R_PIXEL_SCALE_MAX)
                r_pixel_scale.value.f = R_PIXEL_SCALE_MAX;
        r_width_2d = r_width.value.n / r_pixel_scale.value.f + 0.5f;
        r_height_2d = r_height.value.n / r_pixel_scale.value.f + 0.5f;
        C_debug("2D area %dx%d", r_width_2d, r_height_2d);

        /* Set screen view */
        glViewport(0, 0, r_width.value.n, r_height.value.n);
        R_check_errors();

        return TRUE;
}

/******************************************************************************\
 Outputs debug strings and checks supported extensions.
 TODO: Use some extensions.
\******************************************************************************/
static void check_gl_extensions(void)
{
}

/******************************************************************************\
 Sets the initial OpenGL state.
\******************************************************************************/
static void set_gl_state(void)
{
        c_color_t clear_color;

        glEnable(GL_TEXTURE_2D);
        glEnable(GL_COLOR_MATERIAL);

        /* We use lines to do 2D edge anti-aliasing although there is probably
           a better way so we need to always smooth lines (requires alpha
           blending to be on to work) */
        glEnable(GL_LINE_SMOOTH);

        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

        glAlphaFunc(GL_GREATER, 0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);

        /* Set the background color */
        C_var_unlatch(&r_clear);
        clear_color = C_color_string(r_clear.value.s);
        glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0);

        /* Setting the color parameter will modulate the texture color */
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColor3f(1.0, 1.0, 1.0);

        /* Only rasterize polygons that are facing you. Blender seems to export
           its polygons in counter-clockwise order. */
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        /* GL_SMOOTH here means faces are shaded taking each vertex into
           account rather than one solid color per face */
        glShadeModel(GL_SMOOTH);

        /* Set the OpenGL gamma ramp using SDL */
        C_var_unlatch(&r_gamma);
        SDL_SetGamma(r_gamma.value.f, r_gamma.value.f, r_gamma.value.f);

        /* Not configured to a render mode intially */
        r_mode = R_MODE_NONE;

        R_check_errors();
}

/******************************************************************************\
 Loads all the render testing assets.
\******************************************************************************/
static void load_test_assets(void)
{
        /* Test model */
        C_var_unlatch(&r_test_model_path);
        if (*r_test_model_path.value.s)
                R_model_init(&test_model, r_test_model_path.value.s);

        /* Spinning sprites */
        C_var_unlatch(&r_test_sprite_num);
        C_var_unlatch(&r_test_sprite_path);
        if (r_test_sprite_num.value.n && r_test_sprite_path.value.s[0]) {
                int i;

                C_rand_seed(time(NULL));
                test_sprites = C_malloc(r_test_sprite_num.value.n *
                                        sizeof (*test_sprites));
                R_sprite_init(test_sprites, r_test_sprite_path.value.s);
                test_sprites[0].origin = C_vec2(32, 32);
                for (i = 1; i < r_test_sprite_num.value.n; i++) {
                        R_sprite_init(test_sprites + i,
                                      r_test_sprite_path.value.s);
                        test_sprites[i].origin = C_vec2(r_width_2d * C_rand(),
                                                        r_height_2d * C_rand());
                        test_sprites[i].angle = C_rand();
                }
        }

        /* Spinning text */
        C_var_unlatch(&r_test_text);
        R_text_init(&test_text);
        if (r_test_text.value.s[0]) {
                R_text_configure(&test_text, R_FONT_CONSOLE, 100.f, 1.f,
                                 TRUE, r_test_text.value.s);
                test_text.sprite.origin = C_vec2(r_width_2d / 2,
                                                 r_height_2d / 2);
        }
}

/******************************************************************************\
 Creates the client window. Initializes OpenGL settings such view matrices,
 culling, and depth testing. Returns TRUE on success.
\******************************************************************************/
int R_init(void)
{
        C_status("Opening window");
        C_count_reset(&r_count_faces);

        /* NVidia drivers respect this environment variable for vsync
           FIXME: Doesn't work! */
        SDL_putenv(r_vsync.value.n ? "__GL_SYNC_TO_VBLANK=1" :
                                     "__GL_SYNC_TO_VBLANK=0");

        /* Need to do these initialization steps before loading any assets */
        if (!set_video_mode())
                return FALSE;
        check_gl_extensions();
        set_gl_state();
        R_clip_disable();

        R_load_assets();
        load_test_assets();
        R_generate_globe(0, 4);
        return TRUE;
}

/******************************************************************************\
 Cleanup the OpenGL mess.
\******************************************************************************/
void R_cleanup(void)
{
        R_free_assets();

        /* Cleanup render testing */
        R_model_cleanup(&test_model);
        if (test_sprites) {
                int i;

                for (i = 0; i < r_test_sprite_num.value.n; i++)
                        R_sprite_cleanup(test_sprites + i);
                C_free(test_sprites);
        }
        R_text_cleanup(&test_text);
}

/******************************************************************************\
 Render the test model.
\******************************************************************************/
static void render_test_model(void)
{
        float left[] = { -1.0, 0.0, 0.0, 0.0 };

        if (!test_model.data)
                return;
        glClear(GL_DEPTH_BUFFER_BIT);

        R_set_mode(R_MODE_3D);

        /* Setup a white light to the left */
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, left);
        R_check_errors();

        /* Render the test mesh */
        test_model.origin.z = -7;
        R_model_render(&test_model);

        /* Spin the model around a bit */
        test_model.angles.x += 0.05 * c_frame_sec;
        test_model.angles.y += 0.30 * c_frame_sec;
}

/******************************************************************************\
 Render test sprites.
\******************************************************************************/
void render_test_sprites(void)
{
        int i;

        if (!r_test_sprite_path.value.s[0] || r_test_sprite_num.value.n < 1)
                return;
        for (i = 0; i < r_test_sprite_num.value.n; i++) {
                R_sprite_render(test_sprites + i);
                test_sprites[i].angle += i * c_frame_sec /
                                         r_test_sprite_num.value.n;
        }
}

/******************************************************************************\
 Render test text.
\******************************************************************************/
void render_test_text(void)
{
        if (!r_test_text.value.s[0])
                return;
        R_text_render(&test_text);
        test_text.sprite.angle += 0.5f * c_frame_sec;
}

/******************************************************************************\
 See if there were any OpenGL errors.
\******************************************************************************/
void R_check_errors_full(const char *file, int line, const char *func)
{
        int gl_error;

        if (r_gl_errors.value.n < 1)
                return;
        gl_error = glGetError();
        if (gl_error == GL_NO_ERROR)
                return;
        if (r_gl_errors.value.n > 1)
                C_error_full(file, line, func, "OpenGL error %d: %s",
                             gl_error, gluErrorString(gl_error));
        C_warning(file, line, func, "OpenGL error %d: %s",
                  gl_error, gluErrorString(gl_error));
}

/******************************************************************************\
 Sets up the camera in 3D mode.
\******************************************************************************/
static void set_camera(void)
{
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0, 0, -r_camera.z);
        glRotatef(r_camera.x, 1.0, 0.0, 0.0);
        glRotatef(r_camera.y, 0.0, 1.0, 0.0);
}

/******************************************************************************\
 Sets up OpenGL to render 2D or 3D polygons.
\******************************************************************************/
void R_set_mode(r_mode_t mode)
{
        if (mode == r_mode)
                return;
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        if (mode == R_MODE_2D) {
                glOrtho(0.f, r_width_2d, r_height_2d, 0.f, -1.f, 1.f);
                glDisable(GL_LIGHTING);
                glDisable(GL_CULL_FACE);
                glDisable(GL_DEPTH_TEST);
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
        } else if (mode == R_MODE_3D) {
                gluPerspective(90.0, (float)r_width.value.n / r_height.value.n,
                               1.f, 10000.f);
                glEnable(GL_LIGHTING);
                glEnable(GL_CULL_FACE);
                glEnable(GL_DEPTH_TEST);
                glDisable(GL_CLIP_PLANE0);
                glDisable(GL_CLIP_PLANE1);
                glDisable(GL_CLIP_PLANE2);
                glDisable(GL_CLIP_PLANE3);
                set_camera();
        } else
                C_error("Unknown render mode %d", mode);
        R_check_errors();
        r_mode = mode;
}

/******************************************************************************\
 Sets the OpenGL clipping planes to the strictest clipping in each direction.
 This only works in 2D mode. OpenGL takes plane equations as arguments. Points
 that are visible satisfy the following inequality:
 a * x + b * y + c * z + d >= 0
\******************************************************************************/
static void set_clipping(void)
{
        GLdouble eqn[4];
        float left, top, right, bottom;
        int i;

        R_set_mode(R_MODE_2D);

        /* Find the most restrictive clipping values in each direction */
        left = clip_values[0];
        top = clip_values[1];
        right = clip_values[2];
        bottom = clip_values[3];
        for (i = 1; i < clip_stack; i++) {
                if (clip_values[4 * i] > left)
                        left = clip_values[4 * i];
                if (clip_values[4 * i + 1] > top)
                        top = clip_values[4 * i + 1];
                if (clip_values[4 * i + 2] < right)
                        right = clip_values[4 * i + 2];
                if (clip_values[4 * i + 3] < bottom)
                        bottom = clip_values[4 * i + 3];
        }

        /* Clip left */
        eqn[2] = 0.f;
        eqn[3] = -1.f;
        if (left > 0.f) {
                eqn[0] = 1.f / left;
                eqn[1] = 0.f;
                glEnable(GL_CLIP_PLANE0);
                glClipPlane(GL_CLIP_PLANE0, eqn);
        } else
                glDisable(GL_CLIP_PLANE0);

        /* Clip top */
        if (top > 0.f) {
                eqn[0] = 0.f;
                eqn[1] = 1.f / top;
                glEnable(GL_CLIP_PLANE1);
                glClipPlane(GL_CLIP_PLANE1, eqn);
        } else
                glDisable(GL_CLIP_PLANE1);

        /* Clip right */
        eqn[3] = 1.f;
        if (right < r_width_2d - 1) {
                eqn[0] = -1.f / right;
                eqn[1] = 0.f;
                glEnable(GL_CLIP_PLANE2);
                glClipPlane(GL_CLIP_PLANE2, eqn);
        } else
                glDisable(GL_CLIP_PLANE2);

        /* Clip bottom */
        if (bottom < r_height_2d - 1) {
                eqn[0] = 0.f;
                eqn[1] = -1.f / bottom;
                glEnable(GL_CLIP_PLANE3);
                glClipPlane(GL_CLIP_PLANE3, eqn);
        } else
                glDisable(GL_CLIP_PLANE3);
}

/******************************************************************************\
 Disables the clipping at the current stack level.
\******************************************************************************/
void R_clip_disable(void)
{
        clip_values[4 * clip_stack] = 0.f;
        clip_values[4 * clip_stack + 1] = 0.f;
        clip_values[4 * clip_stack + 2] = INFINITY;
        clip_values[4 * clip_stack + 3] = INFINITY;
        set_clipping();
}

/******************************************************************************\
 Adjust the clip stack.
\******************************************************************************/
void R_clip_push(void)
{
        clip_stack++;
        if (clip_stack >= CLIP_STACK)
                C_error("Clip stack overflow");
        clip_values[4 * clip_stack] = 0.f;
        clip_values[4 * clip_stack + 1] = 0.f;
        clip_values[4 * clip_stack + 2] = 0.f;
        clip_values[4 * clip_stack + 3] = 0.f;
}

void R_clip_pop(void)
{
        clip_stack--;
        if (clip_stack < 0)
                C_error("Clip stack underflow");
        set_clipping();
}

/******************************************************************************\
 Clip in a specific direction.
\******************************************************************************/
void R_clip_left(float dist)
{
        clip_values[4 * clip_stack] = dist;
        set_clipping();
}

void R_clip_top(float dist)
{
        clip_values[4 * clip_stack + 1] = dist;
        set_clipping();
}

void R_clip_right(float dist)
{
        clip_values[4 * clip_stack + 2] = dist;
        set_clipping();
}

void R_clip_bottom(float dist)
{
        clip_values[4 * clip_stack + 3] = dist;
        set_clipping();
}

/******************************************************************************\
 Setup a clipping rectangle.
\******************************************************************************/
void R_clip_rect(c_vec2_t origin, c_vec2_t size)
{
        clip_values[4 * clip_stack] = origin.x;
        clip_values[4 * clip_stack + 1] = origin.y;
        clip_values[4 * clip_stack + 2] = origin.x + size.x;
        clip_values[4 * clip_stack + 3] = origin.y + size.y;
        set_clipping();
}

/******************************************************************************\
 Clears the buffer and starts rendering the scene.
\******************************************************************************/
void R_start_frame(void)
{
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        render_test_model();
        R_render_globe();
}

/******************************************************************************\
 Finishes rendering the scene and flips the buffer.
\******************************************************************************/
void R_finish_frame(void)
{
        render_test_sprites();
        render_test_text();
        SDL_GL_SwapBuffers();
        R_check_errors();
}

