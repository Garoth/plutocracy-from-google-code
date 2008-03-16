/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Devin Papineau

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "r_common.h"
#include "../game/g_shared.h"
#include <time.h>

extern c_var_t r_clear, r_gl_errors,
               r_test_globe, r_test_globe_seed, r_test_model_path,
               r_test_mesh_path, r_test_sprite_num, r_test_sprite_path,
               r_test_text;

/* Keep track of how many faces we render each frame */
c_count_t r_count_faces;

/* Current OpenGL settings */
r_mode_t r_mode;
int r_width_2d, r_height_2d;

/* Testing assets */
static r_static_mesh_t *test_mesh;
static r_model_t test_model;
static r_sprite_t *test_sprites;
static r_text_t test_text;
static g_globe_t *test_globe;

/******************************************************************************\
 Sets the video mode. SDL creates a window in windowed mode.
\******************************************************************************/
static int set_video_mode(void)
{
        SDL_Surface *screen;
        int flags;

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

        //glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

        glAlphaFunc(GL_GREATER, 0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);

        /* Set the background color */
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
        if (r_test_globe.value.n) {
                unsigned int seed;

                seed = r_test_globe_seed.value.n ? r_test_globe_seed.value.n :
                                                   time(NULL);
                C_debug("Test globe seed %u", seed);
                test_globe = G_globe_alloc(5, seed, 0.1);
        } else if (*r_test_model_path.value.s)
                R_model_init(&test_model, r_test_model_path.value.s);
        /*else if (*r_test_mesh_path.value.s)
                test_mesh = R_static_mesh_load(r_test_mesh_path.value.s);*/
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

        R_load_assets();
        load_test_assets();
        return TRUE;
}

/******************************************************************************\
 Cleanup the OpenGL mess.
\******************************************************************************/
void R_cleanup(void)
{
        R_free_assets();

        /* Cleanup render testing */
        R_static_mesh_free(test_mesh);
        R_model_cleanup(&test_model);
        G_globe_free(test_globe);
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
static int render_test_model(void)
{
        float left[] = { -1.0, 0.0, 0.0, 0.0 };

        if (!test_model.data)
                return FALSE;

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

        return TRUE;
}

/******************************************************************************\
 Render the test mesh.
\******************************************************************************/
static int render_test_mesh(void)
{
        static float x_rot, y_rot;
        float left[] = { -1.0, 0.0, 0.0, 0.0 };

        if (!test_mesh)
                return FALSE;

        R_set_mode(R_MODE_3D);

        /* Setup a white light to the left */
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, left);

        /* Render the test mesh */
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(0.0, 0.0, -2000.0);
        glRotatef(x_rot, 1.0, 0.0, 0.0);
        glRotatef(y_rot, 0.0, 1.0, 0.0);
        glTranslatef(0.0, -10.0, 0.0);
        R_static_mesh_render(test_mesh, NULL);
        glPopMatrix();
        R_check_errors();

        x_rot += 8 * c_frame_sec;
        y_rot += 20 * c_frame_sec;

        return TRUE;
}

/******************************************************************************\
 Render the test globe.
\******************************************************************************/
int render_test_globe()
{
        static float x_rot, y_rot;
        float left[] = { -1.0, 0.0, 0.0, 0.0 };

        static int island_to_show = 1;
        static int since_island_change = 0;
        int i;

        if (!r_test_globe.value.n)
                return FALSE;

        R_set_mode(R_MODE_3D);

        /* Globe renders without texturing atm */
        glDisable(GL_TEXTURE_2D);

        /* Have a light from the left */
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, left);

        glPushMatrix();
        glLoadIdentity();

        glTranslatef(0.0, 0.0, -2000.0);
        glRotatef(x_rot, 1.0, 0.0, 0.0);
        glRotatef(y_rot, 0.0, 1.0, 0.0);

        /* And render. I'll just shove stuff in here right now.
           I'm actually worried about the generation of the thing. */
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);

        /* Water */
        /*glColorMask(FALSE, FALSE, FALSE, FALSE);*/
        glColor3f(0.0, 0.5, 1.0);
        glVertexPointer(3, GL_FLOAT, 0, test_globe->water_verts);
        glNormalPointer(GL_FLOAT, 0, test_globe->water_verts);
        glEnable(GL_NORMALIZE); /* So I can be lazy */
        glDrawElements(GL_TRIANGLES, test_globe->ninds,
                       GL_UNSIGNED_SHORT, test_globe->inds);
        glDisable(GL_NORMALIZE);

        /* Lines - scratch that - Solid land overtop */
        /*glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);*/
        /*glColorMask(TRUE, TRUE, TRUE, TRUE);*/
        glColor3f(0.435294117647059,
                  0.741176470588235,
                  0.298039215686275);
        glVertexPointer(3, GL_FLOAT, 0, test_globe->verts);
        glNormalPointer(GL_FLOAT, 0, test_globe->norms);
        glDrawElements(GL_TRIANGLES, test_globe->ninds,
                       GL_UNSIGNED_SHORT, test_globe->inds);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);

        /* Show all points on island_to_show */
        glPointSize(5.0);
        glColor3f(1.0, 0.0, 0.0);
        glBegin(GL_POINTS);

        for (i = 0; i < test_globe->nverts; i++)
                if (test_globe->island_ids[i] == island_to_show) {
                        c_vec3_t v;

                        v = C_vec3_norm(test_globe->water_verts[i]);
                        glNormal3f(v.x, v.y, v.z);
                        v = test_globe->verts[i];
                        glVertex3f(v.x, v.y, v.z);
                }

        glEnd();

        glPopMatrix();

        /* Reset polygon mode and texturing */
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glEnable(GL_TEXTURE_2D);

        R_check_errors();

        x_rot += 2 * c_frame_sec;
        y_rot += 20 * c_frame_sec;

        if (250 < (since_island_change += c_frame_msec)) {
                island_to_show %= test_globe->n_islands;
                island_to_show++;
                since_island_change = 0;
        }

        return TRUE;
}

/******************************************************************************\
 Render test sprites.
\******************************************************************************/
int render_test_sprites(void)
{
        int i;

        if (!r_test_sprite_path.value.s[0] || r_test_sprite_num.value.n < 1)
                return FALSE;
        for (i = 0; i < r_test_sprite_num.value.n; i++) {
                R_sprite_render(test_sprites + i);
                test_sprites[i].angle += i * c_frame_sec /
                                         r_test_sprite_num.value.n;
        }
        return TRUE;
}

/******************************************************************************\
 Render test text.
\******************************************************************************/
int render_test_text(void)
{
        if (!r_test_text.value.s[0])
                return FALSE;
        R_text_render(&test_text);
        test_text.sprite.angle += 0.5f * c_frame_sec;
        return TRUE;
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
 Sets up OpenGL to render 2D or 3D polygons.
   TODO: Work a camera into the projection matrix.
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
        } else {
                R_clip_disable();
                gluPerspective(90.0, (float)r_width.value.n / r_height.value.n,
                               1.f, 10000.f);
                glEnable(GL_LIGHTING);
                glEnable(GL_CULL_FACE);
                glEnable(GL_DEPTH_TEST);
        }
        glMatrixMode(GL_MODELVIEW);
        R_check_errors();
        r_mode = mode;
}

/******************************************************************************\
 Disables the clipping planes.
\******************************************************************************/
void R_clip_disable(void)
{
        glDisable(GL_CLIP_PLANE0);
        glDisable(GL_CLIP_PLANE1);
        glDisable(GL_CLIP_PLANE2);
        glDisable(GL_CLIP_PLANE3);
}

/******************************************************************************\
 Clip in a specific direction. This only works in 2D mode. OpenGL takes plane
 equations as arguments. Points that are visible satisfy the following
 inequality:
 a * x + b * y + c * z + d >= 0
\******************************************************************************/
void R_clip_left(float dist)
{
        GLdouble eqn[4] = { 1.f, 0.f, 0.f, -1.f };

        R_set_mode(R_MODE_2D);
        glEnable(GL_CLIP_PLANE0);
        eqn[0] /= dist;
        glClipPlane(GL_CLIP_PLANE0, eqn);
}

void R_clip_top(float dist)
{
        GLdouble eqn[4] = { 0.f, 1.f, 0.f, -1.f };

        R_set_mode(R_MODE_2D);
        glEnable(GL_CLIP_PLANE1);
        eqn[1] /= dist;
        glClipPlane(GL_CLIP_PLANE1, eqn);
}

void R_clip_right(float dist)
{
        GLdouble eqn[4] = { -1.f, 0.f, 0.f, 1.f };

        R_set_mode(R_MODE_2D);
        glEnable(GL_CLIP_PLANE2);
        eqn[0] /= dist;
        glClipPlane(GL_CLIP_PLANE2, eqn);
}

void R_clip_bottom(float dist)
{
        GLdouble eqn[4] = { 0.f, -1.f, 0.f, 1.f };

        R_set_mode(R_MODE_2D);
        glEnable(GL_CLIP_PLANE3);
        eqn[1] /= dist;
        glClipPlane(GL_CLIP_PLANE3, eqn);
}

/******************************************************************************\
 Setup a clipping rectangle. This only works in 2D mode.
\******************************************************************************/
void R_clip_rect(c_vec2_t origin, c_vec2_t size)
{
        GLdouble eqn[4];

        R_set_mode(R_MODE_2D);
        glEnable(GL_CLIP_PLANE0);
        glEnable(GL_CLIP_PLANE1);
        glEnable(GL_CLIP_PLANE2);
        glEnable(GL_CLIP_PLANE3);
        eqn[0] = 1.f / origin.x;
        eqn[1] = 0.f;
        eqn[2] = 0.f;
        eqn[3] = -1.f;
        glClipPlane(GL_CLIP_PLANE0, eqn);
        eqn[0] = 0;
        eqn[1] = 1.f / origin.y;
        glClipPlane(GL_CLIP_PLANE1, eqn);
        eqn[0] = -1.f / (origin.x + size.x);
        eqn[1] = 0;
        eqn[3] = 1.f;
        glClipPlane(GL_CLIP_PLANE2, eqn);
        eqn[0] = 0.f;
        eqn[1] = 1.f / (origin.y + size.y);
        glClipPlane(GL_CLIP_PLANE3, eqn);
}

/******************************************************************************\
 Clears the buffer and starts rendering the scene.
\******************************************************************************/
void R_start_frame(void)
{
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (render_test_globe() || render_test_model() || render_test_mesh());
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

