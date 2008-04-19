/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Sunlight, moonlight, and stars */

#include "r_common.h"

/* The rotation speed of the sun around the globe has to be fixed so that there
   are no synchronization errors between players. This sets the day length to
   be one day every three minutes. */
#define ROTATION_SPEED 0.0175f

#define NUM_STARS 200

static r_billboard_t moon, sun, stars[NUM_STARS];
static c_color_t moon_colors[3], sun_colors[3];
static float rotation;

/******************************************************************************\
 Initializes light-related variables.
\******************************************************************************/
void R_init_solar(void)
{
        float dist, scale;
        int i;

        /* Update light colors */
        for (i = 0; i < 3; i++) {
                C_var_update_data(r_sun_colors + i, C_color_update,
                                  sun_colors + i);
                C_var_update_data(r_moon_colors + i, C_color_update,
                                  moon_colors + i);
        }

        /* Load solar object sprites */
        R_billboard_init(&moon, "models/solar/moon.png");
        R_billboard_init(&sun, "models/solar/sun.png");
        sun.world_origin.x = 4900.f;
        moon.world_origin.x = -4900.f;
        moon.unscaled = sun.unscaled = TRUE;

        /* Create stars */
        for (i = 0; i < NUM_STARS; i++) {
                R_billboard_init(stars + i, "models/solar/star.png");
                stars[i].world_origin = C_vec3(C_rand_real() - 0.5f,
                                               C_rand_real() - 0.5f,
                                               C_rand_real() - 0.5f);
                dist = C_vec3_len(stars[i].world_origin);
                stars[i].world_origin = C_vec3_scalef(stars[i].world_origin,
                                                      5000.f / dist);
                scale = 0.25f + 0.75f * C_rand_real();
                stars[i].sprite.size.x *= scale;
                stars[i].sprite.size.y *= scale;
        }
}

/******************************************************************************\
 Cleanup light-related variables.
\******************************************************************************/
void R_cleanup_solar(void)
{
        int i;

        R_billboard_cleanup(&moon);
        R_billboard_cleanup(&sun);
        for (i = 0; i < NUM_STARS; i++)
                R_billboard_cleanup(stars + i);
}

/******************************************************************************\
 Sets up the moon and sun lights.
\******************************************************************************/
void R_enable_light(void)
{
        float sun_pos[4], moon_pos[4];

        if (!r_light.value.n)
                return;
        glEnable(GL_LIGHTING);
        glPushMatrix();
        glRotatef(C_rad_to_deg(rotation), 0.f, 1.f, 0.f);

        /* Sunlight */
        sun_pos[0] = r_globe_radius + r_moon_height.value.f;
        sun_pos[1] = sun_pos[2] = sun_pos[3] = 0.f;
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, C_ARRAYF(sun_pos));
        glLightfv(GL_LIGHT0, GL_AMBIENT, (float *)sun_colors);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, (float *)(sun_colors + 1));
        glLightfv(GL_LIGHT0, GL_SPECULAR, (float *)(sun_colors + 2));

        /* Moonlight */
        moon_pos[0] = -sun_pos[0];
        moon_pos[1] = moon_pos[2] = 0.f;
        moon_pos[3] = 1.f;
        glEnable(GL_LIGHT1);
        glLightfv(GL_LIGHT1, GL_POSITION, C_ARRAYF(moon_pos));
        glLightfv(GL_LIGHT1, GL_AMBIENT, (float *)moon_colors);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, (float *)(moon_colors + 1));
        glLightfv(GL_LIGHT1, GL_SPECULAR, (float *)(moon_colors + 2));
        glLightf(GL_LIGHT1, GL_QUADRATIC_ATTENUATION, r_moon_atten.value.f);
        glLightf(GL_LIGHT1, GL_LINEAR_ATTENUATION, 0.f);
        glLightf(GL_LIGHT1, GL_CONSTANT_ATTENUATION, 0.f);

        glPopMatrix();
}

/******************************************************************************\
 Turns out the lights.
\******************************************************************************/
void R_disable_light(void)
{
        glDisable(GL_LIGHTING);
        glDisable(GL_LIGHT0);
        glDisable(GL_LIGHT1);
}

/******************************************************************************\
 Renders the background behind the globe.
\******************************************************************************/
void R_render_solar(void)
{
        int i;

        rotation -= ROTATION_SPEED * c_frame_sec;
        glPushMatrix();
        glLoadIdentity();
        glRotatef(C_rad_to_deg(rotation), 0.f, 1.f, 0.f);
        glGetFloatv(GL_MODELVIEW_MATRIX, sun.transform);
        glPopMatrix();
        memcpy(moon.transform, sun.transform, sizeof (moon.transform));

        /* Render the stars */
        for (i = 0; i < NUM_STARS; i++) {
                memcpy(stars[i].transform, moon.transform,
                       sizeof (stars[i].transform));
                R_billboard_render(stars + i);
        }

        /* Render the sun and moon point sprites */
        R_billboard_render(&moon);
        R_billboard_render(&sun);
}

