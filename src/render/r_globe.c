/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Implements the generation and rendering of the world globe */

#include "r_common.h"

/* Sine-wave blending modulation for the tile selection */
#define HOVER_FREQ 0.005f
#define HOVER_AMP 0.250f
#define HOVER_OPACITY 0.25f
#define SELECT_OPACITY 0.5f

/* Current hover and selection colors */
c_color_t r_hover_color, r_select_color;

/* Globe material colors after modulation */
c_color_t r_material[3];

/* Modulates the globe material colors */
float r_globe_light;

/* Maximum zoom distance from globe surface */
float r_zoom_max;

/* Select overlays */
static r_vertex3_t hover_verts[3], select_verts[3], path_verts[R_PATH_MAX * 3];
static r_texture_t *select_tex[R_SELECT_TYPES];
static r_select_type_t hover_type, select_type;
static int hover_tile, selected_tile, path_len;

/* Original globe colors */
static c_color_t material_colors[3];

/******************************************************************************\
 Initialize globe data.
\******************************************************************************/
void R_init_globe(void)
{
        int i;

        /* Load select triangle textures */
        select_tex[R_ST_TILE] = R_texture_load("models/globe/select_tile.png",
                                               TRUE);
        select_tex[R_ST_TILE]->additive = TRUE;
        select_tex[R_ST_GOTO] = R_texture_load("models/globe/select_goto.png",
                                               TRUE);
        select_tex[R_ST_GOTO]->additive = TRUE;
        select_tex[R_ST_PATH] = R_texture_load("models/globe/select_path.png",
                                               TRUE);
        select_tex[R_ST_PATH]->additive = TRUE;
        select_type = R_ST_NONE;

        /* Setup globe material properties */
        for (i = 0; i < 3; i++)
                C_var_update_data(r_globe_colors + i, C_color_update,
                                  material_colors + i);

        /* Clear path */
        path_len = 0;
}

/******************************************************************************\
 Cleanup globe data.
\******************************************************************************/
void R_cleanup_globe(void)
{
        int i;

        for (i = 0; i < R_SELECT_TYPES; i++)
                R_texture_free(select_tex[i]);
        R_vbo_cleanup(&r_globe_vbo);
}

/******************************************************************************\
 Start rendering the globe.
\******************************************************************************/
void R_start_globe(void)
{
        int i;

        R_push_mode(R_MODE_3D);

        /* Set globe material properties */
        glMateriali(GL_FRONT, GL_SHININESS, r_globe_shininess.value.n);
        for (i = 0; i < 3; i++) {
                r_material[i].r = material_colors[i].r * r_globe_light;
                r_material[i].g = material_colors[i].g * r_globe_light;
                r_material[i].b = material_colors[i].b * r_globe_light;
                r_material[i].a = material_colors[i].a;
        }
        glMaterialfv(GL_FRONT, GL_AMBIENT, (GLfloat *)r_material);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, (GLfloat *)(r_material + 1));
        glMaterialfv(GL_FRONT, GL_SPECULAR, (GLfloat *)(r_material + 2));

        R_start_atmosphere();
        R_enable_light();
        R_texture_select(r_terrain_tex);

        /* Only render if the globe is desired */
        if (!r_globe.value.n)
                return;

        /* Render the globe through a vertex buffer object */
        R_vbo_render(&r_globe_vbo);

        /* Base selection color on the fog color */
        r_select_color = r_fog_color;
        r_select_color.a *= SELECT_OPACITY;

        /* Sine-wave modulate the hover highlight. Even though no tile may
           be selected, this color could be used elsewhere. */
        r_hover_color = r_fog_color;
        r_hover_color.a *= HOVER_OPACITY * (1.f - HOVER_AMP *
                           (1.f - sinf(HOVER_FREQ * c_time_msec)));

        /* Render hover triangle */
        if (hover_tile >= 0 && hover_type != R_ST_NONE) {
                R_gl_disable(GL_LIGHTING);
                R_texture_select(select_tex[hover_type]);
                glColor4f(r_hover_color.r, r_hover_color.g,
                          r_hover_color.b, r_hover_color.a);
                glInterleavedArrays(R_VERTEX3_FORMAT, sizeof (*hover_verts),
                                    hover_verts);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                glColor4f(1.f, 1.f, 1.f, 1.f);
                C_count_add(&r_count_faces, 1);
                R_gl_restore();
        }

        /* Render selection triangle */
        if (selected_tile >= 0 && select_type != R_ST_NONE) {
                R_gl_disable(GL_LIGHTING);
                R_texture_select(select_tex[select_type]);
                glColor4f(r_select_color.r, r_select_color.g,
                          r_select_color.b, r_select_color.a);
                glInterleavedArrays(R_VERTEX3_FORMAT, sizeof (*select_verts),
                                    select_verts);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                glColor4f(1.f, 1.f, 1.f, 1.f);
                C_count_add(&r_count_faces, 1);
                R_gl_restore();
        }

        /* If there is a path overlay, render it */
        if (path_len > 0) {
                R_gl_disable(GL_LIGHTING);
                R_texture_select(select_tex[R_ST_PATH]);
                glColor4f(r_fog_color.r, r_fog_color.g,
                          r_fog_color.b, r_fog_color.a);
                glInterleavedArrays(R_VERTEX3_FORMAT, sizeof (*path_verts),
                                    path_verts);
                glDrawArrays(GL_TRIANGLES, 0, path_len * 3);
                glColor4f(1.f, 1.f, 1.f, 1.f);
                C_count_add(&r_count_faces, path_len);
                R_gl_restore();
        }

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        R_check_errors();
        C_count_add(&r_count_faces, r_tiles_max);

        /* Render the globe's normals for testing */
        R_render_normals(3 * r_tiles_max, &r_globe_verts[0].v.co,
                         &r_globe_verts[0].v.no, sizeof (*r_globe_verts));
}

/******************************************************************************\
 Finishes rendering the globe.
\******************************************************************************/
void R_finish_globe(void)
{
        R_finish_atmosphere();
        R_disable_light();
        R_check_errors();
        R_pop_mode();
}

/******************************************************************************\
 Copies a globe tile's vertex data to a 3D vertex array.
\******************************************************************************/
static void copy_tile_vertices(int tile, r_vertex3_t *verts)
{
        int i;

        for (i = 0; i < 3; i++) {
                verts[i].co = r_globe_verts[3 * tile + i].v.co;
                verts[i].no = r_globe_verts[3 * tile + i].v.no;
        }

        /* Set single-texture UV */
        verts[0].uv = C_vec2(0.5f, 0.0f);
        verts[1].uv = C_vec2(0.0f, 1.0f);
        verts[2].uv = C_vec2(1.0f, 1.0f);
}

/******************************************************************************\
 Picks a tile on the globe to highlight during rendering with selection
 modulation over time.
\******************************************************************************/
void R_hover_tile(int tile, r_select_type_t type)
{
        if (hover_tile == tile)
                return;
        if (tile < 0 || type == R_ST_NONE) {
                hover_tile = -1;
                hover_type = R_ST_NONE;
                return;
        }
        hover_type = type;
        hover_tile = tile;
        copy_tile_vertices(tile, hover_verts);
}

/******************************************************************************\
 Picks a tile on the globe to highlight during rendering.
\******************************************************************************/
void R_select_tile(int tile, r_select_type_t type)
{
        if (selected_tile == tile)
                return;
        if (tile < 0 || type == R_ST_NONE) {
                selected_tile = -1;
                select_type = R_ST_NONE;
                return;
        }
        select_type = type;
        selected_tile = tile;
        copy_tile_vertices(tile, select_verts);
}

/******************************************************************************\
 Generates a highlighted path from [tile] to render on the globe.
\******************************************************************************/
void R_select_path(int tile, const char *path)
{
        int index;

        path_len = 0;
        if (!path || path[0] < 1 || tile < 0)
                return;
        for (; path_len < R_PATH_MAX; path_len++) {
                index = path[path_len] - 1;
                if (index < 0 || index > 2)
                        break;
                tile = r_globe_verts[3 * tile + index].next / 3;
                copy_tile_vertices(tile, path_verts + path_len * 3);
        }
}

