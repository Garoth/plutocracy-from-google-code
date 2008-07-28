/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Implements rendering functions for ship effects */

#include "r_common.h"

/* The size of half of a quad edge */
#define QUAD_HALF 0.75f

/* Vertical offset for the status plane */
#define QUAD_Z 0.05f

/* Modulation for background bars */
#define BAR_BACKGROUND 0.33f

static r_vertex3_t vertices[7];
static r_texture_t *quad_tex, *quad_other_tex, *select_tex, *bars_tex;

/******************************************************************************\
 Initialize ship resources.
\******************************************************************************/
void R_init_ships(void)
{
        c_vec3_t up;

        /* Load textures */
        quad_tex = R_texture_load("models/ship/status_circle.png", TRUE);
        quad_tex->additive = TRUE;
        quad_other_tex = R_texture_load("models/ship/status_other.png", TRUE);
        quad_other_tex->additive = TRUE;
        bars_tex = R_texture_load("models/ship/status_bars.png", TRUE);
        bars_tex->additive = TRUE;
        select_tex = R_texture_load("models/ship/status_select.png", TRUE);
        select_tex->additive = TRUE;

        /* Quad vertices never change */
        up = C_vec3(0.f, 1.f, 0.f);
        vertices[0].co = C_vec3(-QUAD_HALF, QUAD_Z, QUAD_HALF);
        vertices[0].uv = C_vec2(1.f, 0.f);
        vertices[0].no = up;
        vertices[1].co = C_vec3(QUAD_HALF, QUAD_Z, QUAD_HALF);
        vertices[1].uv = C_vec2(0.f, 0.f);
        vertices[1].no = up;
        vertices[2].co = C_vec3(-QUAD_HALF, QUAD_Z, -QUAD_HALF);
        vertices[2].uv = C_vec2(1.f, 1.f);
        vertices[2].no = up;
        vertices[3].co = C_vec3(QUAD_HALF, QUAD_Z, -QUAD_HALF);
        vertices[3].uv = C_vec2(0.f, 1.f);
        vertices[3].no = up;

        /* The center vertex of the bars also don't change */
        vertices[4].co = C_vec3(0.f, QUAD_Z, 0.f);
        vertices[4].uv = C_vec2(0.5f, 0.5f);
        vertices[4].no = up;

        /* Only the position and uv changes for the dynamic corners */
        vertices[5].no = up;
        vertices[6].no = up;
}

/******************************************************************************\
 Cleanup ship resources.
\******************************************************************************/
void R_cleanup_ships(void)
{
        R_texture_free(quad_tex);
        R_texture_free(quad_other_tex);
        R_texture_free(select_tex);
        R_texture_free(bars_tex);
}

/******************************************************************************\
 Render a world-space textured quad.
\******************************************************************************/
static void render_quad(const r_texture_t *texture)
{
        R_texture_select(texture);
        glInterleavedArrays(R_VERTEX3_FORMAT, 0, vertices);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

/******************************************************************************\
 Renders the bar display.
\******************************************************************************/
static void render_bars(float left, float right)
{
        const unsigned short indices[6] = {5, 4, 2, 6, 3, 4};

        /* Dynamically position the top left and right vertices */
        left = cosf(left * C_PI / 2.f + C_PI / 4.f) / C_COS_45;
        right = cosf(right * C_PI / 2.f + C_PI / 4.f) / C_COS_45;
        vertices[6].co = C_vec3(QUAD_HALF, QUAD_Z, -QUAD_HALF * left);
        vertices[6].uv = C_vec2(0.f, 0.5f + 0.5f * left);
        vertices[5].co = C_vec3(-QUAD_HALF, QUAD_Z, -QUAD_HALF * right);
        vertices[5].uv = C_vec2(1.f, 0.5f + 0.5f * right);

        R_texture_select(bars_tex);
        glInterleavedArrays(R_VERTEX3_FORMAT, 0, vertices);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
}

/******************************************************************************\
 Render ship status plane. This assumes that the model has already been
 rendered once this frame so we can just take the cached matrix without doing
 any extra computations.
\******************************************************************************/
void R_render_ship_status(const r_model_t *model, float left, float left_max,
                          float right, float right_max, c_color_t modulate,
                          bool selected, bool own)
{
        R_push_mode(R_MODE_3D);
        R_gl_disable(GL_LIGHTING);
        modulate = C_color_scale(modulate, model->modulate);
        glColor4f(modulate.r, modulate.g, modulate.b, modulate.a);
        glMultMatrixf(model->matrix);
        glDepthMask(GL_FALSE);
        render_quad(own ? quad_tex : quad_other_tex);
        if (selected) {
                glColor4f(r_select_color.r, r_select_color.g, r_select_color.b,
                          r_fog_color.a);
                render_quad(select_tex);
        }
        if (r_multisample.value.n)
                R_gl_enable(GL_MULTISAMPLE);
        glColor4f(modulate.r, modulate.g, modulate.b,
                  modulate.a * BAR_BACKGROUND);
        render_bars(left_max, right_max);
        glColor4f(modulate.r, modulate.g, modulate.b,
                  modulate.a * (1.f - BAR_BACKGROUND));
        render_bars(left, right);
        glDepthMask(GL_TRUE);

        /* Make sure these are off after the interleaved array calls */
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);

        R_gl_restore();
        R_pop_mode();
}

