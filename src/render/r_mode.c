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

/* Stack limits */
#define CLIP_STACK 32
#define MODE_STACK 32

/* Keep track of how many faces we render each frame */
c_count_t r_count_faces;

/* Current OpenGL settings */
r_mode_t r_mode;
c_color_t clear_color;
int r_width_2d, r_height_2d, r_mode_hold, r_restart;

/* Restart the video system */
int r_restart;
static int init_frame;

/* Supported extensions */
int r_extensions[R_EXTENSIONS];

/* Camera location */
c_vec3_t r_cam_normal;

/* The full camera matrix */
static float cam_matrix[16];

/* Camera rotation and zoom */
float r_cam_zoom;
static c_vec2_t cam_rot_diff;
static GLfloat cam_rotation[16];
static float cam_zoom_diff;

/* Clipping stack */
static int clip_stack;
static float clip_values[CLIP_STACK * 4];

/* Mode stack */
static int mode_stack;
static r_mode_t mode_values[MODE_STACK];

/* When non-empty will save a PNG screenshot to this filename */
static char screenshot[256];

/******************************************************************************\
 Updates when [r_pixel_scale] changes.
\******************************************************************************/
static void pixel_scale_update(void)
{
        if (r_pixel_scale.value.f < R_PIXEL_SCALE_MIN)
                r_pixel_scale.value.f = R_PIXEL_SCALE_MIN;
        if (r_pixel_scale.value.f > R_PIXEL_SCALE_MAX)
                r_pixel_scale.value.f = R_PIXEL_SCALE_MAX;
        r_width_2d = (int)(r_width.value.n / r_pixel_scale.value.f + 0.5f);
        r_height_2d = (int)(r_height.value.n / r_pixel_scale.value.f + 0.5f);
        R_free_fonts();
        R_load_fonts();
        C_debug("2D area %dx%d", r_width_2d, r_height_2d);
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
        if (r_width.value.n < R_WIDTH_MIN)
                r_width.value.n = R_WIDTH_MIN;
        if (r_height.value.n < R_HEIGHT_MIN)
                r_height.value.n = R_HEIGHT_MIN;

#ifdef WINDOWS
        /* On Windows, before we change the video mode we need to flush out
           any existing textures in case they actually don't get lost or
           deallocated properly on their own */
        R_dealloc_textures();
#endif

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
        screen = SDL_SetVideoMode(r_width.value.n, r_height.value.n,
                                  r_color_bits.value.n, flags);
        if (!screen) {
                C_warning("Failed to set video mode: %s", SDL_GetError());
                return FALSE;
        }

        /* Set screen view */
        glViewport(0, 0, r_width.value.n, r_height.value.n);
        R_check_errors();

        /* Update pixel scale */
        pixel_scale_update();

#ifdef WINDOWS
        /* Under Windows we just lost all of our textures, so we need to
           reload the entire texture linked list */
        R_realloc_textures();
#endif

        return TRUE;
}

/******************************************************************************\
 Checks the extension string to see if an extension is listed.
 FIXME: Will return false-positive for partial match
\******************************************************************************/
static int check_extension(const char *ext)
{
        static const char *ext_str;

        if (!ext_str)
                ext_str = (const char *)glGetString(GL_EXTENSIONS);
        return strstr(ext_str, ext) != NULL;
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

        /* Point sprites */
        if (check_extension("GL_ARB_point_sprite")) {
                r_extensions[R_EXT_POINT_SPRITE] = TRUE;
                C_debug("Hardware point sprites supported");
        } else {
                r_extensions[R_EXT_POINT_SPRITE] = FALSE;
                C_warning("Using software point sprites");
        }

        /* Check for anisotropic filtering */
        if (check_extension("GL_EXT_texture_filter_anisotropic")) {
                glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &gl_int);
                r_extensions[R_EXT_ANISOTROPY] = gl_int;
                C_debug("%d anisotropy levels supported", gl_int);
        } else {
                r_extensions[R_EXT_ANISOTROPY] = 0;
                C_warning("Anisotropic filtering not supported");
        }

        /* Check for vertex buffer objects */
        if (check_extension("GL_ARB_vertex_buffer_object")) {
                r_extensions[R_EXT_VERTEX_BUFFER] = TRUE;
                C_debug("Vertex buffer objects supported");
        } else {
                r_extensions[R_EXT_VERTEX_BUFFER] = FALSE;
                C_warning("Vertex buffer objects not supported");
        }
}

/******************************************************************************\
 Sets the initial OpenGL state.
\******************************************************************************/
static void set_gl_state(void)
{
        c_color_t color;

        glEnable(GL_TEXTURE_2D);
        glAlphaFunc(GL_GREATER, 1 / 255.f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

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

        /* Not configured to a render mode intially. The stack is initialized
           with no modes on it (index of -1). */
        r_mode = R_MODE_NONE;
        mode_stack = -1;
        mode_values[0] = R_MODE_NONE;

        /* Initialize camera to identity */
        memset(cam_rotation, 0, sizeof (cam_rotation));
        cam_rotation[0] = 1.f;
        cam_rotation[5] = 1.f;
        cam_rotation[10] = 1.f;
        cam_rotation[15] = 1.f;

        /* Setup the lighting model */
        color = C_color(0.f, 0.f, 0.f, 1.f);
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, C_ARRAYF(color));

        /* Point sprites */
        if (r_extensions[R_EXT_POINT_SPRITE]) {
                glEnable(GL_POINT_SPRITE);
                glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE);
        }

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
\******************************************************************************/
void R_init(void)
{
        C_status("Opening window");
        C_count_reset(&r_count_faces);

        /* If we fail to set the video mode, our only hope is to reset the
           "unsafe" variables that the user might have messed up and try
           setting the video mode again */
        if (!set_video_mode()) {
                C_reset_unsafe_vars();
                if (!set_video_mode())
                        C_error("Failed to set video mode");
                C_warning("Video mode set after resetting unsafe variables");
        }

        check_gl_extensions();
        set_gl_state();
        r_cam_zoom = R_ZOOM_MAX;
        R_clip_disable();
        R_load_assets();
        R_init_solar();

        /* Set updatable variables */
        C_var_update(&r_clear, clear_update);
        C_var_update(&r_gamma, gamma_update);
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

        R_push_mode(R_MODE_3D);
        glMatrixMode(GL_MODELVIEW);

        /* Update zoom */
        r_cam_zoom += cam_zoom_diff;
        if (r_cam_zoom < R_ZOOM_MIN)
                r_cam_zoom = R_ZOOM_MIN;
        if (r_cam_zoom > R_ZOOM_MAX)
                r_cam_zoom = R_ZOOM_MAX;
        cam_zoom_diff = 0.f;

        /* Apply the rotation differences from last frame to the rotation
           matrix to get view-oriented scrolling */
        glLoadMatrixf(cam_rotation);
        x_axis = C_vec3(cam_rotation[0], cam_rotation[4], cam_rotation[8]);
        y_axis = C_vec3(cam_rotation[1], cam_rotation[5], cam_rotation[9]);
        glRotatef(C_rad_to_deg(cam_rot_diff.y), x_axis.x, x_axis.y, x_axis.z);
        glRotatef(C_rad_to_deg(cam_rot_diff.x), y_axis.x, y_axis.y, y_axis.z);
        cam_rot_diff = C_vec2(0.f, 0.f);
        glGetFloatv(GL_MODELVIEW_MATRIX, cam_rotation);

        /* Recreate the full camera matrix with the new rotation */
        glLoadIdentity();
        glTranslatef(0, 0, -r_globe_radius - r_cam_zoom);
        glMultMatrixf(cam_rotation);
        glGetFloatv(GL_MODELVIEW_MATRIX, cam_matrix);

        /* Extract the camera location from the matrix for use by other parts
           of the program. We want to where the camera itself is rather than
           where it rotates things so we take the inverse rotation matrix's
           Z axis. */
        r_cam_normal = C_vec3(cam_rotation[2], cam_rotation[6],
                              cam_rotation[10]);

        R_pop_mode();
}

/******************************************************************************\
 Move the camera incrementally relative to the current orientation.
\******************************************************************************/
void R_move_cam_by(c_vec2_t angle)
{
        cam_rot_diff = C_vec2_add(cam_rot_diff, angle);
}

/******************************************************************************\
 Zoom the camera incrementally.
\******************************************************************************/
void R_zoom_cam_by(float f)
{
        cam_zoom_diff += f;
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

        if (r_mode != R_MODE_2D)
                return;

        /* Find the most restrictive clipping values in each direction */
        left = clip_values[0];
        top = clip_values[1];
        right = clip_values[2];
        bottom = clip_values[3];
        for (i = 1; i <= clip_stack; i++) {
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
void R_push_clip(void)
{
        if (++clip_stack >= CLIP_STACK)
                C_error("Clip stack overflow");
        R_clip_disable();
}

void R_pop_clip(void)
{
        if (--clip_stack < 0)
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
 Sets up OpenGL to render 3D polygons in world space.
\******************************************************************************/
void R_set_mode(r_mode_t mode)
{
        if (r_mode_hold)
                return;

        /* Reset model-view matrices even if the mode didn't change */
        if (mode == R_MODE_3D)
                glLoadMatrixf(cam_matrix);
        else if (mode == R_MODE_2D)
                glLoadIdentity();

        if (mode == r_mode)
                return;
        r_mode = mode;
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();

        /* 2D mode sets up an orthogonal projection to render sprites */
        if (mode == R_MODE_2D) {
                glOrtho(0.f, r_width_2d, r_height_2d, 0.f, 0.f, 1.f);
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                set_clipping();
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
                glEnable(GL_CULL_FACE);
                glEnable(GL_DEPTH_TEST);
                glMatrixMode(GL_MODELVIEW);
                glLoadMatrixf(cam_matrix);
        } else {
                glDisable(GL_CULL_FACE);
                glDisable(GL_DEPTH_TEST);
        }

        R_check_errors();
}

/******************************************************************************\
 Save the previous mode and set the mode. Will also push an OpenGL model view
 matrix on the stack.
\******************************************************************************/
void R_push_mode(r_mode_t mode)
{
        if (++mode_stack >= 32)
                C_error("Mode stack overflow");
        mode_values[mode_stack] = mode;
        glPushMatrix();
        R_set_mode(mode);
}

/******************************************************************************\
 Restore saved mode. Will also pop an OpenGL model view matrix off the stack.
\******************************************************************************/
void R_pop_mode(void)
{
        if (--mode_stack < -1)
                C_error("Mode stack underflow");
        glPopMatrix();
        if (mode_stack >= 0)
                R_set_mode(mode_values[mode_stack]);
}

/******************************************************************************\
 Clears the buffer and starts rendering the scene.
\******************************************************************************/
void R_start_frame(void)
{
        int clear_flags;

        /* Video can only be restarted at the start of the frame */
        if (r_restart) {
                set_video_mode();
                if (r_color_bits.changed > init_frame)
                        R_realloc_textures();
                init_frame = c_frame;
                r_restart = FALSE;
        }

        /* Pixel scale should only be updated at the start of the frame */
        if (C_var_unlatch(&r_pixel_scale))
                pixel_scale_update();

        /* Only clear the screen if r_clear is set */
        clear_flags = GL_DEPTH_BUFFER_BIT;
        if (clear_color.a > 0.f)
                clear_flags |= GL_COLOR_BUFFER_BIT;
        glClear(clear_flags);

        update_camera();
        R_render_solar();
}

/******************************************************************************\
 Mark this frame for saving a screenshot when the buffer flips.
\******************************************************************************/
void R_save_screenshot(const char *filename)
{
        if (screenshot[0]) {
                C_warning("Can't save '%s', screenshot '%s' queued",
                          filename, screenshot);
                return;
        }
        C_strncpy_buf(screenshot, filename);
}

/******************************************************************************\
 Finishes rendering the scene and flips the buffer.
\******************************************************************************/
void R_finish_frame(void)
{
        R_render_tests();

        /* Before flipping the buffer, save any pending screenshots */
        if (screenshot[0]) {
                r_texture_t *tex;

                C_debug("Saving screenshot '%s'", screenshot);
                tex = R_texture_alloc(r_width.value.n, r_height.value.n, FALSE);
                R_texture_screenshot(tex, 0, 0);
                R_texture_save(tex, screenshot);
                R_texture_free(tex);
                screenshot[0] = NUL;
        }

        SDL_GL_SwapBuffers();
        R_check_errors();
}

