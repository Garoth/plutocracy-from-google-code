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

static r_model_t test_model;
static r_billboard_t *test_sprites;
static r_text_t test_text;

/******************************************************************************\
 Cleanup test assets.
\******************************************************************************/
void R_free_test_assets(void)
{
        R_model_cleanup(&test_model);
        if (test_sprites) {
                int i;

                for (i = 0; i < r_test_sprite_num.value.n; i++)
                        R_billboard_cleanup(test_sprites + i);
                C_free(test_sprites);
        }
        R_text_cleanup(&test_text);
}

/******************************************************************************\
 Reloads the test model when [r_test_model] changes value.
\******************************************************************************/
static int test_model_update(c_var_t *var, c_var_value_t value)
{
        R_model_cleanup(&test_model);
        if (!value.s[0])
                return TRUE;
        return R_model_init(&test_model, value.s);
}

/******************************************************************************\
 Reloads the testing sprite when [r_test_sprite] or [r_test_sprites] changes.
\******************************************************************************/
static int test_sprite_update(c_var_t *var, c_var_value_t value)
{
        int i;

        /* Cleanup old sprites */
        if (test_sprites) {
                for (i = 0; i < r_test_sprite_num.value.n; i++)
                        R_billboard_cleanup(test_sprites + i);
                C_free(test_sprites);
                test_sprites = NULL;
        }

        /* Create new sprites */
        var->value = value;
        if (r_test_sprite_num.value.n < 1 || !r_test_sprite.value.s[0])
                return TRUE;
        C_rand_seed((unsigned int)time(NULL));
        test_sprites = C_malloc(r_test_sprite_num.value.n *
                                sizeof (*test_sprites));
        for (i = 0; i < r_test_sprite_num.value.n; i++) {
                c_vec3_t origin;

                R_billboard_init(test_sprites + i, r_test_sprite.value.s);
                origin = C_vec3(r_globe_radius * (C_rand_real() - 0.5f),
                                r_globe_radius * (C_rand_real() - 0.5f),
                                r_globe_radius + 3.f);
                test_sprites[i].world_origin = origin;
                test_sprites[i].unscaled = TRUE;
                test_sprites[i].sprite.angle = C_rand_real();
        }

        return TRUE;
}

/******************************************************************************\
 Loads all the render testing assets.
\******************************************************************************/
void R_load_test_assets(void)
{
        /* Test model */
        C_var_unlatch(&r_test_model);
        if (*r_test_model.value.s)
                R_model_init(&test_model, r_test_model.value.s);
        r_test_model.edit = C_VE_FUNCTION;
        r_test_model.update = test_model_update;

        /* Spinning sprites */
        C_var_unlatch(&r_test_sprite);
        r_test_sprite.edit = C_VE_FUNCTION;
        r_test_sprite.update = test_sprite_update;
        C_var_update(&r_test_sprite_num, test_sprite_update);

        /* Spinning text */
        C_var_unlatch(&r_test_text);
        R_text_init(&test_text);
        if (r_test_text.value.s[0]) {
                R_text_configure(&test_text, R_FONT_CONSOLE, 100.f, 1.f,
                                 TRUE, r_test_text.value.s);
                test_text.sprite.origin = C_vec2(r_width_2d / 2.f,
                                                 r_height_2d / 2.f);
        }
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
        R_push_mode(R_MODE_3D);

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
        test_model.angles.x += 0.05f * c_frame_sec;
        test_model.angles.y += 0.30f * c_frame_sec;

        R_pop_mode();
}

/******************************************************************************\
 Render test sprites.
\******************************************************************************/
static void render_test_sprites(void)
{
        int i;

        if (!r_test_sprite.value.s[0] || r_test_sprite_num.value.n < 1)
                return;
        for (i = 0; i < r_test_sprite_num.value.n; i++) {
                R_billboard_render(test_sprites + i);
                test_sprites[i].sprite.angle += i * c_frame_sec /
                                                r_test_sprite_num.value.n;
        }
}

/******************************************************************************\
 Render test text.
\******************************************************************************/
static void render_test_text(void)
{
        if (!r_test_text.value.s[0])
                return;
        R_text_render(&test_text);
        test_text.sprite.angle += 0.5f * c_frame_sec;
}

/******************************************************************************\
 Render testing models, sprites, etc.
\******************************************************************************/
void R_render_tests(void)
{
        render_test_model();
        render_test_sprites();
        render_test_text();
}

