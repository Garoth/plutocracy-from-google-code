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

/* Keep track of how many faces we render each frame */
c_count_t r_count_faces;

/* Current OpenGL settings */
r_mode_t r_mode;
c_color_t clear_color;
int r_width_2d, r_height_2d;

/* Supported extensions */
int r_extensions[R_EXTENSIONS];

/* The full camera matrix */
float r_cam_matrix[16], r_proj3_matrix[16];

/* Camera rotation and zoom */
float r_cam_dist, r_cam_zoom;
static c_vec2_t cam_diff;
static GLfloat cam_rotation[16];

/* Clipping stack */
static int clip_stack;
static float clip_values[CLIP_STACK * 4];

/******************************************************************************\
 Updates when [r_pixel_scale] changes.
\******************************************************************************/
static int pixel_scale_update(c_var_t *var, c_var_value_t value)
{
        if (value.f < R_PIXEL_SCALE_MIN)
                value.f = R_PIXEL_SCALE_MIN;
        if (value.f > R_PIXEL_SCALE_MAX)
                value.f = R_PIXEL_SCALE_MAX;
        r_width_2d = (int)(r_width.value.n / value.f + 0.5f);
        r_height_2d = (int)(r_height.value.n / value.f + 0.5f);
        r_pixel_scale.value = value;
        R_free_fonts();
        R_load_fonts();
        C_debug("2D area %dx%d", r_width_2d, r_height_2d);
        return TRUE;
}

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

        /* Ensure a minimum render size or pre-rendering will crash */
        if (r_width.value.n < 640)
                r_width.value.n = 640;
        if (r_height.value.n < 480)
                r_height.value.n = 480;

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

        /* Update 2D area */
        C_var_update(&r_pixel_scale, pixel_scale_update);

        /* Set screen view */
        glViewport(0, 0, r_width.value.n, r_height.value.n);
        R_check_errors();

        return TRUE;
}

/******************************************************************************\
 Outputs debug strings and checks supported extensions. The supported
 extensions array (r_extensions) is filled out accordingly.
\******************************************************************************/
static void check_gl_extensions(void)
{
        GLint gl_int;

        C_zero(r_extensions);

        /* Multitexture */
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &gl_int);
        r_extensions[R_EXT_MULTITEXTURE] = gl_int;
        C_debug("%d texture units supported", gl_int);
}

/******************************************************************************\
 Sets the initial OpenGL state.
\******************************************************************************/
static void set_gl_state(void)
{
        c_color_t color;

        glAlphaFunc(GL_GREATER, 0);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

        /* We never render untextured vertices */
        glEnable(GL_TEXTURE_2D);

        /* We use lines to do 2D edge anti-aliasing although there is probably
           a better way so we need to always smooth lines (requires alpha
           blending to be on to work) */
        glEnable(GL_LINE_SMOOTH);
        glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

        /* Only rasterize polygons that are facing you. Blender seems to export
           its polygons in counter-clockwise order. */
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        /* GL_SMOOTH here means faces are shaded taking each vertex into
           account rather than one solid color per face */
        glShadeModel(GL_SMOOTH);

        /* Not configured to a render mode intially */
        r_mode = R_MODE_NONE;

        /* Initialize camera to identity */
        memset(cam_rotation, 0, sizeof (cam_rotation));
        cam_rotation[0] = 1.f;
        cam_rotation[5] = 1.f;
        cam_rotation[10] = 1.f;
        cam_rotation[15] = 1.f;

        /* Setup the lighting model */
        color = C_color(0.f, 0.f, 0.f, 1.f);
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, C_ARRAYF(color));

        R_check_errors();
}

/******************************************************************************\
 Updates the gamma ramp when [r_gamma] changes.
\******************************************************************************/
static int gamma_update(c_var_t *var, c_var_value_t value)
{
        return SDL_SetGamma(value.f, value.f, value.f) >= 0;
}

/******************************************************************************\
 Updates the clear color when [r_clear] changes.
\******************************************************************************/
static int clear_update(c_var_t *var, c_var_value_t value)
{
        clear_color = C_color_string(value.s);
        glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.f);
        return TRUE;
}

/******************************************************************************\
 Creates the client window. Initializes OpenGL settings such view matrices,
 culling, and depth testing. Returns TRUE on success.

 TODO: Try backup "safe" video modes when user requested mode fails.
\******************************************************************************/
void R_init(void)
{
        C_status("Opening window");
        C_count_reset(&r_count_faces);
        if (!set_video_mode())
                C_error("No available video modes");
        check_gl_extensions();
        set_gl_state();
        r_cam_zoom = (R_ZOOM_MAX - R_ZOOM_MIN) / 2.f;
        R_clip_disable();
        R_load_assets();
        R_init_solar();

        /* Set updatable variables */
        C_var_update(&r_clear, clear_update);
        C_var_update(&r_gamma, gamma_update);
        r_pixel_scale.edit = C_VE_FUNCTION;
        r_pixel_scale.update = pixel_scale_update;
}

/******************************************************************************\
 Cleanup the namespace.
\******************************************************************************/
void R_cleanup(void)
{
        R_cleanup_solar();
        R_free_assets();
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
 Calculates a new camera rotation matrix and reloads the modelview matrix.
\******************************************************************************/
static void update_camera(void)
{
        c_vec3_t x_axis, y_axis;

        R_set_mode(R_MODE_3D);
        glMatrixMode(GL_MODELVIEW);

        /* Apply the rotation differences from last frame to the rotation
           matrix to get view-oriented scrolling */
        glLoadMatrixf(cam_rotation);
        x_axis = C_vec3(cam_rotation[0], cam_rotation[4], cam_rotation[8]);
        y_axis = C_vec3(cam_rotation[1], cam_rotation[5], cam_rotation[9]);
        glRotatef(C_rad_to_deg(cam_diff.y), x_axis.x, x_axis.y, x_axis.z);
        glRotatef(C_rad_to_deg(cam_diff.x), y_axis.x, y_axis.y, y_axis.z);
        cam_diff = C_vec2(0.f, 0.f);
        glGetFloatv(GL_MODELVIEW_MATRIX, cam_rotation);

        /* Recreate the full camera matrix with the new rotation */
        glLoadIdentity();
        glTranslatef(0, 0, -r_cam_dist - r_cam_zoom);
        glMultMatrixf(cam_rotation);
        glGetFloatv(GL_MODELVIEW_MATRIX, r_cam_matrix);
}

/******************************************************************************\
 Move the camera incrementally relative to the current orientation.
\******************************************************************************/
void R_move_cam_by(c_vec2_t angle)
{
        cam_diff = C_vec2_add(cam_diff, angle);
}

/******************************************************************************\
 Zoom the camera incrementally.
\******************************************************************************/
void R_zoom_cam_by(float f)
{
        r_cam_zoom += f;
        if (r_cam_zoom < 0)
                r_cam_zoom = 0;
        if (r_cam_zoom > R_ZOOM_MAX - R_ZOOM_MIN)
                r_cam_zoom = R_ZOOM_MAX - R_ZOOM_MIN;
}

/******************************************************************************\
 Sets up OpenGL to render 3D polygons in world space. If mode is set to
 R_MODE_HOLD, the mode will not be changeable until set to R_MODE_NONE.
\******************************************************************************/
void R_set_mode(r_mode_t mode)
{
        if (mode == r_mode || (mode && r_mode == R_MODE_HOLD))
                return;
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        /* 2D mode sets up an orthogonal projection to render sprites */
        if (mode == R_MODE_2D) {
                glOrtho(0.f, r_width_2d, r_height_2d, 0.f, 0.f, 1.f);
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
        } else {
                glDisable(GL_CLIP_PLANE0);
                glDisable(GL_CLIP_PLANE1);
                glDisable(GL_CLIP_PLANE2);
                glDisable(GL_CLIP_PLANE3);
        }

        /* 3D mode sets up perspective projection and camera view for models */
        if (mode == R_MODE_3D) {
                gluPerspective(90.0, (float)r_width.value.n / r_height.value.n,
                               1.f, 512.f);
                glGetFloatv(GL_PROJECTION_MATRIX, r_proj3_matrix);
                glEnable(GL_CULL_FACE);
                glEnable(GL_DEPTH_TEST);
                glMatrixMode(GL_MODELVIEW);
                glLoadMatrixf(r_cam_matrix);
        } else {
                glDisable(GL_CULL_FACE);
                glDisable(GL_DEPTH_TEST);
        }

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
        clip_values[4 * clip_stack + 2] = 100000.f;
        clip_values[4 * clip_stack + 3] = 100000.f;
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
        int clear_flags;

        /* Only clear the screen if r_clear is set */
        clear_flags = GL_DEPTH_BUFFER_BIT;
        if (clear_color.a > 0.f)
                clear_flags |= GL_COLOR_BUFFER_BIT;
        glClear(clear_flags);

        update_camera();
        R_render_globe();
        R_render_solar();
}

/******************************************************************************\
 Finishes rendering the scene and flips the buffer.
\******************************************************************************/
void R_finish_frame(void)
{
        R_render_tests();
        SDL_GL_SwapBuffers();
        R_check_errors();
}

