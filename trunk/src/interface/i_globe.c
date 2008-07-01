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
static c_vec2_t globe_motion;
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
 Returns an eye-space ray direction from the camera into the screen pixel.
\******************************************************************************/
static c_vec3_t screen_ray(int x, int y)
{
        return C_vec3_norm(C_vec3(x - r_width_2d / 2.f, r_height_2d / 2.f - y,
                                  -0.5f * r_height_2d / R_FOV_HALF_TAN));
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
        direction = screen_ray(x, y);
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
static void grab_globe(int x, int y)
{
        if (grabbing)
                return;
        grabbing = TRUE;
        if (screen_to_normal(x, y, &grab_normal, &grab_angle)) {
                grab_rolling = FALSE;
                grab_normal = R_rotate_from_cam(grab_normal);
        } else
                grab_rolling = TRUE;
        I_close_ring();
}

/******************************************************************************\
 Release the globe from a rotation grab.
\******************************************************************************/
static void release_globe(void)
{
        c_vec3_t direction;

        grabbing = FALSE;
        if (i_mouse_focus != &i_root)
                return;

        /* After a mouse grab is released, we need to scan for selection */
        direction = screen_ray(i_mouse_x, i_mouse_y);
        direction = R_rotate_to_cam(direction);
        G_mouse_ray(r_cam_origin, direction);
}

/******************************************************************************\
 Rotate the globe during a grab or do nothing. [x] and [y] are in screen
 coordinates.
\******************************************************************************/
static void rotate_globe(int x, int y)
{
        c_vec3_t normal, angles;
        float angle;

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
static void test_globe(void)
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

/******************************************************************************\
 Processes events from the root window that affect the globe.
\******************************************************************************/
void I_globe_event(i_event_t event)
{
        /* No processing here during limbo */
        if (i_limbo)
                return;

        switch (event) {
        case I_EV_KEY_DOWN:
                I_close_ring();
                if (i_key == SDLK_RIGHT && globe_motion.x > -1.f)
                        globe_motion.x = -i_scroll_speed.value.f;
                if (i_key == SDLK_LEFT && globe_motion.x < 1.f)
                        globe_motion.x = i_scroll_speed.value.f;
                if (i_key == SDLK_DOWN && globe_motion.y > -1.f)
                        globe_motion.y = -i_scroll_speed.value.f;
                if (i_key == SDLK_UP && globe_motion.y < 1.f)
                        globe_motion.y = i_scroll_speed.value.f;
                if (i_key == '-')
                        R_zoom_cam_by(i_zoom_speed.value.f);
                if (i_key == '=')
                        R_zoom_cam_by(-i_zoom_speed.value.f);
                break;
        case I_EV_KEY_UP:
                if ((i_key == SDLK_RIGHT && globe_motion.x < 0.f) ||
                    (i_key == SDLK_LEFT && globe_motion.x > 0.f))
                        globe_motion.x = 0.f;
                if ((i_key == SDLK_DOWN && globe_motion.y < 0.f) ||
                    (i_key == SDLK_UP && globe_motion.y > 0.f))
                        globe_motion.y = 0.f;
                break;
        case I_EV_MOUSE_DOWN:
                if (i_mouse == SDL_BUTTON_WHEELDOWN)
                        R_zoom_cam_by(i_zoom_speed.value.f);
                else if (i_mouse == SDL_BUTTON_WHEELUP)
                        R_zoom_cam_by(-i_zoom_speed.value.f);
                else if (i_mouse == SDL_BUTTON_MIDDLE)
                        grab_globe(i_mouse_x, i_mouse_y);
                else
                        G_process_click(i_mouse);
                break;
        case I_EV_MOUSE_UP:
                if (i_mouse == SDL_BUTTON_MIDDLE)
                        release_globe();
                break;
        case I_EV_MOUSE_MOVE:
                if (I_ring_shown())
                        break;
                if (i_mouse_focus != &i_root)
                        G_mouse_ray_miss();
                else if (grabbing)
                        rotate_globe(i_mouse_x, i_mouse_y);
                else {
                        c_vec3_t direction;

                        direction = screen_ray(i_mouse_x, i_mouse_y);
                        direction = R_rotate_to_cam(direction);
                        G_mouse_ray(r_cam_origin, direction);
                }
                break;
        case I_EV_RENDER:
                test_globe();
                R_move_cam_by(C_vec2_scalef(globe_motion, c_frame_sec));
                break;
        default:
                break;
        }
}

