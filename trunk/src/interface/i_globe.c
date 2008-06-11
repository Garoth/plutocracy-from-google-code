/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Interfaces for interacting with the globe */

#include "i_common.h"

static c_vec3_t grab_normal;
static float grab_angle;
static int grabbing, grab_rolling;

/******************************************************************************\
 Performs ray-sphere intersection between an arbitrary ray and the globe. The
 ray is specified using origin [o] and normalized direction [d]. If the ray
 intersects the globe, returns TRUE and sets [point] to the location of the
 intersection.

 The original optimized code taken from:
 http://www.devmaster.net/wiki/Ray-sphere_intersection

 The basic idea is to setup the problem algerbraically and solve using the
 quadratic formula. The resulting calculations are greatly simplified by the
 fact that [d] is normalized and that the sphere is centered on the origin.
\******************************************************************************/
static int intersect_ray_globe(c_vec3_t o, c_vec3_t d, c_vec3_t *point)
{
        float B, C, D;

        /* The naming convention here corresponds to the quadratic formula,
           with [D] as the discriminant */
        B = C_vec3_dot(o, d);
        C = C_vec3_dot(o, o) - r_globe_radius * r_globe_radius;
        D = B * B - C;
        if (D < 0.f)
                return FALSE;
        *point = C_vec3_add(o, C_vec3_scalef(d, -B - sqrtf(D)));
        return TRUE;
}

/******************************************************************************\
 Converts a screen coordinate to a globe [normal]. Returns TRUE if the globe
 was clicked on and [normal] was set. Otherwise, returns FALSe and sets
 [angle] to the roll angle.
\******************************************************************************/
static int screen_to_normal(int x, int y, c_vec3_t *normal, float *angle)
{
        c_vec3_t direction, forward, point;

        /* We need to create a vector that points out of the camera into the
           ray that is cast into the direction of the clicked pixel */
        direction.x = x - r_width_2d / 2.f;
        direction.y = r_height_2d / 2.f - y;
        direction.z = -0.5f * r_height_2d / R_FOV_HALF_TAN;
        direction = C_vec3_norm(direction);
        forward = R_rotate_to_cam(direction);

        /* Test the direction ray */
        if (i_test_globe.value.n) {
                c_vec3_t a, b;

                a = C_vec3_add(r_cam_origin, r_cam_forward);
                b = C_vec3_add(r_cam_origin, C_vec3_scalef(forward, 10.f));
                R_render_test_line(a, b, C_color(1.f, 0.75f, 0.f, 1.f));
        }

        /* Find the intersection point and normalize it to get a normal pointing
           from the globe center out to the clicked on point */
        if (intersect_ray_globe(r_cam_origin, forward, &point)) {
                *normal = C_vec3_norm(point);
                return TRUE;
        }

        /* If there was no intersection, find the roll angle of the mouse */
        *angle = atan2f(direction.y, direction.x);
        return FALSE;
}

/******************************************************************************\
 Grab and begin rotating the globe. [x] and [y] are in screen coordinates.
\******************************************************************************/
void I_grab_globe(int x, int y)
{
        if (grabbing)
                return;
        grabbing = TRUE;
        if (screen_to_normal(x, y, &grab_normal, &grab_angle)) {
                grab_rolling = FALSE;
                grab_normal = R_rotate_from_cam(grab_normal);
        } else
                grab_rolling = TRUE;
}

/******************************************************************************\
 Release the globe from a rotation grab.
\******************************************************************************/
void I_release_globe(void)
{
        grabbing = FALSE;
}

/******************************************************************************\
 Rotate the globe during a grab or do nothing. [x] and [y] are in screen
 coordinates.
\******************************************************************************/
void I_rotate_globe(int x, int y)
{
        c_vec3_t normal, angles;
        float angle;

        if (!grabbing)
                return;

        /* Rotation without roll */
        if (screen_to_normal(x, y, &normal, &angle)) {
                normal = R_rotate_from_cam(normal);

                /* A transition loses the rotation for the frame */
                if (grab_rolling) {
                        grab_normal = normal;
                        grab_rolling = FALSE;
                        return;
                }

                angles.y = acosf(grab_normal.x) - acosf(normal.x);
                angles.x = asinf(grab_normal.y) - asinf(normal.y);
                angles.z = 0.f;
                grab_normal = normal;
        }

        /* Roll-only rotation */
        else {

                /* A transition loses the rotation for the frame */
                if (!grab_rolling) {
                        grab_angle = angle;
                        grab_rolling = TRUE;
                        return;
                }

                angles.x = 0.f;
                angles.y = 0.f;
                angles.z = angle - grab_angle;
                grab_angle = angle;
        }

        R_rotate_cam_by(angles);
}

/******************************************************************************\
 Runs mouse click detection functions for testing.
\******************************************************************************/
void I_test_globe(void)
{
        c_vec3_t normal;
        float angle;

        if (!i_test_globe.value.n ||
            !screen_to_normal(i_mouse_x, i_mouse_y, &normal, &angle))
                return;
        R_render_test_line(C_vec3_scalef(normal, r_globe_radius),
                           C_vec3_scalef(normal, r_globe_radius + 1.f),
                           C_color(0.f, 1.f, 1.f, 1.f));
}

