/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Implements controls for the camera */

#include "r_common.h"

/* Camera location and orientation */
c_vec3_t r_cam_origin, r_cam_forward, r_cam_normal;

/* The distance the camera is from the globe surface */
float r_cam_zoom;

/* The full camera matrix */
GLfloat r_cam_matrix[16];

static c_vec3_t cam_rot_diff;
static GLfloat cam_rotation[16];
static float cam_zoom_diff;

/******************************************************************************\
 Initialize camera structures.
\******************************************************************************/
void R_init_camera(void)
{
        memset(cam_rotation, 0, sizeof (cam_rotation));
        cam_rotation[0] = 1.f;
        cam_rotation[5] = 1.f;
        cam_rotation[10] = 1.f;
        cam_rotation[15] = 1.f;
}

/******************************************************************************\
 Transforms a vector using camera rotation. Does not apply zoom translation.
\******************************************************************************/
c_vec3_t R_rotate_from_cam(c_vec3_t v)
{
        c_vec3_t vt;

        vt.x = v.x * cam_rotation[0] + v.y * cam_rotation[4] +
               v.z * cam_rotation[8];
        vt.y = v.x * cam_rotation[1] + v.y * cam_rotation[5] +
               v.z * cam_rotation[9];
        vt.z = v.x * cam_rotation[2] + v.y * cam_rotation[6] +
               v.z * cam_rotation[10];
        return vt;
}

/******************************************************************************\
 Transforms a vector using the inverse of the camera rotation. Does not apply
 zoom translation.
\******************************************************************************/
c_vec3_t R_rotate_to_cam(c_vec3_t v)
{
        c_vec3_t vt;

        vt.x = v.x * cam_rotation[0] + v.y * cam_rotation[1] +
               v.z * cam_rotation[2];
        vt.y = v.x * cam_rotation[4] + v.y * cam_rotation[5] +
               v.z * cam_rotation[6];
        vt.z = v.x * cam_rotation[8] + v.y * cam_rotation[9] +
               v.z * cam_rotation[10];
        return vt;
}

/******************************************************************************\
 Calculates a new camera rotation matrix and reloads the modelview matrix.
\******************************************************************************/
void R_update_camera(void)
{
        c_vec3_t x_axis, y_axis, z_axis;

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
        z_axis = C_vec3(cam_rotation[2], cam_rotation[6], cam_rotation[10]);
        glRotatef(C_rad_to_deg(cam_rot_diff.x), x_axis.x, x_axis.y, x_axis.z);
        glRotatef(C_rad_to_deg(cam_rot_diff.y), y_axis.x, y_axis.y, y_axis.z);
        glRotatef(C_rad_to_deg(cam_rot_diff.z), z_axis.x, z_axis.y, z_axis.z);
        cam_rot_diff = C_vec3(0.f, 0.f, 0.f);
        glGetFloatv(GL_MODELVIEW_MATRIX, cam_rotation);

        /* Recreate the full camera matrix with the new rotation */
        glLoadIdentity();
        glTranslatef(0, 0, -r_globe_radius - r_cam_zoom);
        glMultMatrixf(cam_rotation);
        glGetFloatv(GL_MODELVIEW_MATRIX, r_cam_matrix);

        /* Extract the camera location from the matrix for use by other parts
           of the program. We cannot replace these new vectors with the axes
           above because they are changed by the rotations. */
        r_cam_forward = C_vec3(-cam_rotation[2], -cam_rotation[6],
                               -cam_rotation[10]);
        r_cam_normal = C_vec3(cam_rotation[1], cam_rotation[5],
                              cam_rotation[9]);
        r_cam_origin = C_vec3_scalef(r_cam_forward,
                                     -r_globe_radius - r_cam_zoom);

        R_pop_mode();
}

/******************************************************************************\
 Rotate the camera incrementally relative to the current orientation. The
 rotation is specified using distances to move in the camera's local x- and
 y-axes.
\******************************************************************************/
void R_move_cam_by(c_vec2_t distance)
{
        cam_rot_diff.x += distance.y / (r_globe_radius * C_PI);
        cam_rot_diff.y += distance.x / (r_globe_radius * C_PI);
}

/******************************************************************************\
 Rotate the camera incrementally relative to the current orientation. Angles
 represent rotation around their axes (i.e. [x] value around the x-axis).
\******************************************************************************/
void R_rotate_cam_by(c_vec3_t angle)
{
        cam_rot_diff = C_vec3_add(cam_rot_diff, angle);
}

/******************************************************************************\
 Zoom the camera incrementally.
\******************************************************************************/
void R_zoom_cam_by(float f)
{
        cam_zoom_diff += f;
}

/******************************************************************************\
 Project a point using a given OpenGL matrix.
\******************************************************************************/
static c_vec3_t project_by_matrix(c_vec3_t co, GLfloat matrix[16])
{
        co = C_vec3_tfm(co, matrix);
        co = C_vec3_tfm(co, r_proj_matrix);
        co.x = (co.x + 1.f) * r_width_2d / 2.f;
        co.y = (1.f - co.y) * r_height_2d / 2.f;
        co.z = (co.z + 1.f) / -2.f;
        return co;
}

/******************************************************************************\
 Project a point using the current modelview matrix.
\******************************************************************************/
c_vec3_t R_project(c_vec3_t co)
{
        GLfloat model_view[16];

        /* Get the model-view matrix from 3D mode */
        R_push_mode(R_MODE_3D);
        glGetFloatv(GL_MODELVIEW_MATRIX, model_view);
        R_pop_mode();

        return project_by_matrix(co, model_view);
}

/******************************************************************************\
 Project a point using the camera matrix.
\******************************************************************************/
c_vec3_t R_project_by_cam(c_vec3_t co)
{
        return project_by_matrix(co, r_cam_matrix);
}
