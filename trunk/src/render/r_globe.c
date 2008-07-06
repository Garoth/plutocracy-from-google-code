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
#define SELECT_FREQ 0.005f
#define SELECT_AMP 0.250f
#define SELECT_MODULATE 0.33f

/* Globe vertex type */
#pragma pack(push, 4)
typedef struct globe_vertex {
        c_vec2_t uv;
        c_vec3_t no;
        c_vec3_t co;
        int next;
} globe_vertex_t;
#pragma pack(pop)

/* Globe radius from the center to sea-level */
float r_globe_radius;

/* Modulates the globe material colors */
float r_globe_light;

/* Maximum zoom distance from globe surface */
float r_zoom_max;

/* Number of tiles on the globe */
int r_tiles;

/* Current selection color */
c_color_t r_select_color;

static r_vertex3_t select_verts[3];
static r_texture_t *select_tex;
static r_vbo_t globe_vbo;
static c_color_t globe_colors[4];
static c_vec3_t normals[R_TILES_MAX];
static globe_vertex_t vertices[R_TILES_MAX * 3];
static int flip_limit, selected_tile;

/******************************************************************************\
 Initialize globe data.
\******************************************************************************/
void R_init_globe(void)
{
        int i;

        /* Load select triangle texture */
        select_tex = R_texture_load("models/globe/select.png", TRUE);
        select_tex->additive = TRUE;

        /* Setup globe material properties */
        for (i = 0; i < 4; i++)
                C_var_update_data(r_globe_colors + i, C_color_update,
                                  globe_colors + i);
}

/******************************************************************************\
 Cleanup globe data.
\******************************************************************************/
void R_cleanup_globe(void)
{
        R_texture_free(select_tex);
        R_vbo_cleanup(&globe_vbo);
}

/******************************************************************************\
 Space out the vertices at even distance from the sphere.
\******************************************************************************/
static void sphericize(void)
{
        c_vec3_t origin;
        int i;

        origin = C_vec3(0.f, 0.f, 0.f);
        for (i = 0; i < r_tiles * 3; i++)
                vertices[i].co = C_vec3_scalef(vertices[i].co, r_globe_radius /
                                               C_vec3_len(vertices[i].co));
}

/******************************************************************************\
 Subdivide each globe tile into four tiles.

 Partioned tile vertices are numbered in the following manner:

          3
         / \
        4---5

     6  2---1  9
    / \  \ /  / \
   7---8  0  10-11

 A vertex's neighbor is the vertex of the tile that shares the next counter-
 clockwise edge of the tile from that vertex.
\******************************************************************************/
static void subdivide4(void)
{
        c_vec3_t mid_0_1, mid_0_2, mid_1_2;
        globe_vertex_t *verts;
        int i, i_flip, j, n[3], n_flip[3];

        for (i = r_tiles - 1; i >= 0; i--) {
                verts = vertices + 12 * i;

                /* Determine which faces are flipped (over 0 vertex) */
                i_flip = i < flip_limit;
                for (j = 0; j < 3; j++) {
                        n[j] = vertices[3 * i + j].next / 3;
                        n_flip[j] = (n[j] < flip_limit) != i_flip;
                }

                /* Compute mid-point coordinates */
                mid_0_1 = C_vec3_add(vertices[3 * i].co,
                                     vertices[3 * i + 1].co);
                mid_0_1 = C_vec3_divf(mid_0_1, 2.f);
                mid_0_2 = C_vec3_add(vertices[3 * i].co,
                                     vertices[3 * i + 2].co);
                mid_0_2 = C_vec3_divf(mid_0_2, 2.f);
                mid_1_2 = C_vec3_add(vertices[3 * i + 1].co,
                                     vertices[3 * i + 2].co);
                mid_1_2 = C_vec3_divf(mid_1_2, 2.f);

                /* Bottom-right triangle */
                verts[9].co = mid_0_2;
                verts[9].next = 12 * i + 1;
                verts[10].co = mid_1_2;
                verts[10].next = 12 * n[1] + 8;
                verts[11].co = vertices[3 * i + 2].co;
                verts[11].next = 12 * n[2] + (n_flip[2] ? 7 : 3);

                /* Bottom-left triangle */
                verts[6].co = mid_0_1;
                verts[6].next = 12 * n[0] + (n_flip[0] ? 9 : 4);
                verts[7].co = vertices[3 * i + 1].co;
                verts[7].next = 12 * n[1] + 11;
                verts[8].co = mid_1_2;
                verts[8].next = 12 * i;

                /* Top triangle */
                verts[3].co = vertices[3 * i].co;
                verts[3].next = 12 * n[0] + (n_flip[0] ? 3 : 7);
                verts[4].co = mid_0_1;
                verts[4].next = 12 * i + 2;
                verts[5].co = mid_0_2;
                verts[5].next = 12 * n[2] + (n_flip[2] ? 4 : 9);

                /* Center triangle */
                verts[0].co = mid_1_2;
                verts[0].next = 12 * i + 10;
                verts[1].co = mid_0_2;
                verts[1].next = 12 * i + 5;
                verts[2].co = mid_0_1;
                verts[2].next = 12 * i + 6;
        }
        flip_limit *= 4;
        r_tiles *= 4;
        r_globe_radius *= 2;
        sphericize();
}

/******************************************************************************\
 Returns the [n]th vertex in the face, clockwise if positive or counter-
 clockwise if negative.
\******************************************************************************/
static int face_next(int vertex, int n)
{
        return 3 * (vertex / 3) + (3 + vertex + n) % 3;
}

/******************************************************************************\
 Finds vertex neighbors by iteration. Runs in O(n^2) time.
\******************************************************************************/
static void find_neighbors(void)
{
        int i, i_next, j, j_next;

        for (i = 0; i < r_tiles * 3; i++) {
                i_next = face_next(i, 1);
                for (j = 0; ; j++) {
                        if (j == i)
                                continue;
                        if (C_vec3_eq(vertices[i].co, vertices[j].co)) {
                                j_next = face_next(j, -1);
                                if (C_vec3_eq(vertices[i_next].co,
                                              vertices[j_next].co)) {
                                        vertices[i].next = j;
                                        break;
                                }
                        }
                        if (j >= r_tiles * 3)
                                C_error("Failed to find next vertex for "
                                        "vertex %d", i);
                }
        }
}

/******************************************************************************\
 Sets up a plain icosahedron.

 The icosahedron has 12 vertices: (0, ±1, ±φ) (±1, ±φ, 0) (±φ, 0, ±1)
 http://en.wikipedia.org/wiki/Icosahedron#Cartesian_coordinates

 We need to have duplicates however because we keep three vertices for each
 face, regardless of unique position because their UV coordinates will probably
 be different.
\******************************************************************************/
static void generate_icosahedron(void)
{
        int i, regular_faces[] = {

                /* Front faces */
                7, 5, 4,        5, 7, 0,        0, 2, 5,
                3, 5, 2,        2, 10, 3,       10, 2, 1,

                /* Rear faces */
                1, 11, 10,      11, 1, 6,       6, 8, 11,
                9, 11, 8,       8, 4, 9,        4, 8, 7,

                /* Top/bottom faces */
                0, 6, 1,        6, 0, 7,        9, 3, 10,       3, 9, 4,
        };

        flip_limit = 4;
        r_tiles = 20;
        r_globe_radius = sqrtf(C_TAU + 2);

        /* Flipped (over 0 vertex) face vertices */
        vertices[0].co = C_vec3(0, C_TAU, 1);
        vertices[1].co = C_vec3(-C_TAU, 1, 0);
        vertices[2].co = C_vec3(-1, 0, C_TAU);
        vertices[3].co = C_vec3(0, -C_TAU, 1);
        vertices[4].co = C_vec3(C_TAU, -1, 0);
        vertices[5].co = C_vec3(1, 0, C_TAU);
        vertices[6].co = C_vec3(0, C_TAU, -1);
        vertices[7].co = C_vec3(C_TAU, 1, 0);
        vertices[8].co = C_vec3(1, 0, -C_TAU);
        vertices[9].co = C_vec3(0, -C_TAU, -1);
        vertices[10].co = C_vec3(-C_TAU, -1, 0);
        vertices[11].co = C_vec3(-1, 0, -C_TAU);

        /* Regular face vertices */
        for (i = 12; i < r_tiles * 3; i++)
                vertices[i].co = vertices[regular_faces[i - 12]].co;

        find_neighbors();
}

/******************************************************************************\
 Generates the globe by subdividing an icosahedron and spacing the vertices
 out at the sphere's surface.
\******************************************************************************/
void R_generate_globe(int subdiv4)
{
        int i;

        if (subdiv4 < 0)
                subdiv4 = 0;
        else if (subdiv4 > R_SUBDIV4_MAX) {
                subdiv4 = R_SUBDIV4_MAX;
                C_warning("Too many subdivisions requested");
        }
        C_debug("Generating globe with %d subdivisions", subdiv4);
        memset(vertices, 0, sizeof (vertices));
        generate_icosahedron();
        for (i = 0; i < subdiv4; i++)
                subdivide4();

        /* Set normals */
        for (i = 0; i < r_tiles * 3; i++)
                vertices[i].no = C_vec3_norm(vertices[i].co);

        /* Delete any old vertex buffers */
        R_vbo_cleanup(&globe_vbo);

        /* Maximum zoom distance is a function of the globe radius */
        r_zoom_max = r_globe_radius * R_ZOOM_MAX_SCALE;

        selected_tile = -1;
        R_generate_halo();
}

/******************************************************************************\
 Returns the modulated globe color.
\******************************************************************************/
static GLfloat *modulate_globe_color(int i)
{
        static c_color_t color;

        color = C_color_scalef(globe_colors[i], r_globe_light);
        color.a = globe_colors[i].a;
        return (GLfloat *)&color;
}

/******************************************************************************\
 Start rendering the globe.
\******************************************************************************/
void R_start_globe(void)
{
        R_push_mode(R_MODE_3D);

        /* Set globe material properties */
        glMateriali(GL_FRONT, GL_SHININESS, r_globe_shininess.value.n);
        glMaterialfv(GL_FRONT, GL_AMBIENT, modulate_globe_color(0));
        glMaterialfv(GL_FRONT, GL_DIFFUSE, modulate_globe_color(1));
        glMaterialfv(GL_FRONT, GL_SPECULAR, modulate_globe_color(2));
        glMaterialfv(GL_FRONT, GL_EMISSION, modulate_globe_color(3));

        R_start_atmosphere();
        R_enable_light();
        R_texture_select(r_terrain_tex);

        /* Only render if the globe is desired */
        if (!r_globe.value.n)
                return;

        /* Render the globe through a vertex buffer object */
        R_vbo_render(&globe_vbo);

        /* Render selection triangle */
        if (selected_tile >= 0) {
                R_gl_disable(GL_LIGHTING);
                R_texture_select(select_tex);

                /* Sine-wave modulate the selection highlight */
                r_select_color = r_fog_color;
                r_select_color.a = SELECT_MODULATE * (1.f - SELECT_AMP *
                                   (1.f - sinf(SELECT_FREQ * c_time_msec)));
                glColor4f(r_select_color.r, r_select_color.g,
                          r_select_color.b, r_select_color.a);

                glInterleavedArrays(R_VERTEX3_FORMAT, sizeof (*select_verts),
                                    select_verts);
                glDrawArrays(GL_TRIANGLES, 0, 3);
                glColor4f(1.f, 1.f, 1.f, 1.f);
                C_count_add(&r_count_faces, 1);
                R_gl_restore();
        }

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        R_check_errors();
        C_count_add(&r_count_faces, r_tiles);

        /* Render the globe's normals for testing */
        R_render_normals(3 * r_tiles, &vertices[0].co, &vertices[0].no,
                         sizeof (*vertices));
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
 Returns the vertices associated with a specific tile via [verts].
\******************************************************************************/
void R_get_tile_coords(int index, c_vec3_t verts[3])
{
        verts[0] = vertices[3 * index].co;
        verts[1] = vertices[3 * index + 1].co;
        verts[2] = vertices[3 * index + 2].co;
}

/******************************************************************************\
 Returns the tiles this tile shares a face with via [neighbors].
\******************************************************************************/
void R_get_tile_neighbors(int tile, int neighbors[3])
{
        neighbors[0] = vertices[3 * tile].next / 3;
        neighbors[1] = vertices[3 * tile + 1].next / 3;
        neighbors[2] = vertices[3 * tile + 2].next / 3;
}

/******************************************************************************\
 Returns the tiles this tile shares a vertex with via [neighbors]. Returns the
 number of entries used in the array.
\******************************************************************************/
int R_get_tile_region(int tile, int neighbors[12])
{
        int i, j, n, next_tile;

        for (n = i = 0; i < 3; i++) {
                next_tile = vertices[face_next(3 * tile + i, -1)].next / 3;
                for (j = vertices[3 * tile + i].next;
                     j / 3 != next_tile; j = vertices[j].next)
                        neighbors[n++] = j / 3;
        }
        return n;
}

/******************************************************************************\
 Returns the "geocentric" latitude (in radians) of the tile:
 http://en.wikipedia.org/wiki/Latitude
\******************************************************************************/
float R_get_tile_latitude(int tile)
{
        float center_y;

        center_y = (vertices[3 * tile].co.y + vertices[3 * tile + 1].co.y +
                    vertices[3 * tile + 2].co.y) / 3.f;
        return asinf(center_y / r_globe_radius);
}

/******************************************************************************\
 Fills [verts] with pointers to the vertices that are co-located with [vert].
 Returns the number of entries that are used. The first vertex is always
 [vert].
\******************************************************************************/
static int vertex_indices(int vert, int verts[6])
{
        int i, pos;

        verts[0] = vert;
        pos = vertices[vert].next;
        for (i = 1; pos != vert; i++) {
                if (i > 6)
                        C_error("Vertex %d ring overflow", vert);
                verts[i] = pos;
                pos = vertices[pos].next;
        }
        return i;
}

/******************************************************************************\
 Sets the height of one tile.
\******************************************************************************/
static void set_tile_height(int tile, float height)
{
        c_vec3_t co;
        float dist;
        int i, j, verts[6], verts_len;

        for (i = 0; i < 3; i++) {
                verts_len = vertex_indices(3 * tile + i, verts);
                height = height / verts_len;
                for (j = 0; j < verts_len; j++) {
                        co = vertices[verts[j]].co;
                        dist = C_vec3_len(co);
                        co = C_vec3_scalef(co, (dist + height) / dist);
                        vertices[verts[j]].co = co;
                }
        }
}

/******************************************************************************\
 Smooth globe vertex normals.
\******************************************************************************/
static void smooth_normals(void)
{
        c_vec3_t normal;
        int i, j, len;

        C_var_unlatch(&r_globe_smooth);
        if (r_globe_smooth.value.f <= 0.f)
                return;
        if (r_globe_smooth.value.f > 1.f)
                r_globe_smooth.value.f = 1.f;
        for (i = 0; i < r_tiles * 3; i++) {

                /* Compute the average normal for this point */
                normal = C_vec3(0.f, 0.f, 0.f);
                len = 0;
                j = vertices[i].next;
                while (j != i) {
                        normal = C_vec3_add(normal, vertices[j].no);
                        j = vertices[j].next;
                        len++;
                }
                normal = C_vec3_scalef(normal, r_globe_smooth.value.f / len);

                /* Set the normal for all vertices in the ring */
                j = vertices[i].next;
                while (j != i) {
                        vertices[j].no = C_vec3_scalef(normals[j / 3], 1.f -
                                                       r_globe_smooth.value.f);
                        vertices[j].no = C_vec3_add(vertices[j].no, normal);
                        j = vertices[j].next;
                }
        }
}

/******************************************************************************\
 Called when [r_globe_smooth] changes.
\******************************************************************************/
static int globe_smooth_update(c_var_t *var, c_var_value_t value)
{
        int i;

        if (value.f > 0.f) {
                r_globe_smooth.value.f = value.f;
                smooth_normals();
        } else
                for (i = 0; i < r_tiles * 3; i++)
                        vertices[i].no = normals[i / 3];
        R_vbo_update(&globe_vbo);
        return TRUE;
}

/******************************************************************************\
 Returns the base terrain for a terrain variant.
\******************************************************************************/
static int terrain_base(r_terrain_t terrain)
{
        switch (terrain) {
        case R_T_GROUND_HOT:
        case R_T_GROUND_COLD:
                return R_T_GROUND;
        case R_T_WATER:
        case R_T_SHALLOW:
                return R_T_SHALLOW;
        default:
                return terrain;
        }
}

/******************************************************************************\
 Selects a terrain index for a tile depending on its region.
\******************************************************************************/
static int get_tile_terrain(int tile, r_tile_t *tiles)
{
        int i, j, base, base_num, trans, terrain, vert_terrain[3], offset;

        base = terrain_base(tiles[tile].terrain);
        if (base >= R_T_BASES - 1)
                return tiles[tile].terrain;

        /* If we are not using transition tiles, just return the base */
        if (!r_globe_transitions.value.n)
                return base;

        /* Each tile vertex can only have up to two base terrains on it, we
           need to find out if the dominant one is present */
        base_num = 0;
        for (i = 0; i < 3; i++) {
                j = vertices[3 * tile + i].next;
                vert_terrain[i] = base;
                while (j != 3 * tile + i) {
                        terrain = terrain_base(tiles[j / 3].terrain);
                        if (terrain > vert_terrain[i])
                                trans = vert_terrain[i] = terrain;
                        j = vertices[j].next;
                }
                if (vert_terrain[i] == base)
                        base_num++;
        }
        if (base_num > 2)
                return tiles[tile].terrain;

        /* Swap the base and single terrain if the base only appears once */
        offset = 2 * base;
        if (base_num == 1) {
                trans = base;
                offset++;
        }

        /* Find the lone terrain and assign the transition tile */
        for (i = 0; i < 3; i++)
                if (vert_terrain[i] == trans)
                        break;

        /* Flipped tiles need reversing */
        if (tile < flip_limit) {
                if (i == 1)
                        i = 2;
                else if (i == 2)
                        i = 1;
        }

        return R_T_TRANSITION + offset * 3 + i;
}

/******************************************************************************\
 Adjusts globe verices to show the tile's height.
\******************************************************************************/
void R_configure_globe(r_tile_t *tiles)
{
        c_vec2_t tile;
        float left, right, top, bottom, tmp;
        int i, tx, ty, terrain;

        C_debug("Configuring globe");
        C_var_unlatch(&r_globe_transitions);

        /* UV dimensions of tile boundary box */
        tile.x = 2.f * (r_terrain_tex->surface->w / R_TILE_SHEET_W) /
                 r_terrain_tex->surface->w;
        tile.y = 2.f * (int)(C_SIN_60 * r_terrain_tex->surface->h /
                             R_TILE_SHEET_H / 2) / r_terrain_tex->surface->h;

        for (i = 0; i < r_tiles; i++) {
                set_tile_height(i, tiles[i].height);

                /* Tile terrain texture */
                terrain = get_tile_terrain(i, tiles);
                ty = terrain / R_TILE_SHEET_W;
                tx = terrain - ty * R_TILE_SHEET_W;
                left = tx / 2 * tile.x + C_SIN_60 * R_TILE_BORDER;
                right = (tx / 2 + 1) * tile.x - C_SIN_60 * R_TILE_BORDER;
                if (tx & 1) {
                        bottom = ty * tile.y + C_SIN_30 * R_TILE_BORDER;
                        top = (ty + 1.f) * tile.y - C_SIN_60 * R_TILE_BORDER;
                        left += tile.x / 2.f;
                        right += tile.x / 2.f;
                } else {
                        top = ty * tile.y + R_TILE_BORDER;
                        bottom = (ty + 1.f) * tile.y - C_SIN_30 * R_TILE_BORDER;
                }

                /* Flip tiles are mirrored over the middle */
                if (i < flip_limit) {
                        tmp = left;
                        left = right;
                        right = tmp;
                }

                vertices[3 * i].uv = C_vec2((left + right) / 2.f, top);
                vertices[3 * i + 1].uv = C_vec2(left, bottom);
                vertices[3 * i + 2].uv = C_vec2(right, bottom);

                /* Set normal vector */
                normals[i] = C_vec3_cross(C_vec3_sub(vertices[3 * i].co,
                                                     vertices[3 * i + 1].co),
                                          C_vec3_sub(vertices[3 * i].co,
                                                     vertices[3 * i + 2].co));
                normals[i] = C_vec3_norm(normals[i]);
                vertices[3 * i].no = normals[i];
                vertices[3 * i + 1].no = normals[i];
                vertices[3 * i + 2].no = normals[i];
        }
        smooth_normals();

        /* We can update normals dynamically from now on */
        r_globe_smooth.edit = C_VE_FUNCTION;
        r_globe_smooth.update = globe_smooth_update;

        /* If Vertex Buffer Objects are supported, upload the vertices now */
        R_vbo_cleanup(&globe_vbo);
        R_vbo_init(&globe_vbo, vertices, 3 * r_tiles, sizeof (*vertices),
                   GL_T2F_N3F_V3F, NULL, 0);
}

/******************************************************************************\
 Picks a tile on the globe to highlight during rendering.
\******************************************************************************/
void R_select_tile(int tile)
{
        selected_tile = tile;
        if (tile < 0)
                return;

        /* Copy coordinates */
        select_verts[0].co = vertices[3 * tile].co;
        select_verts[1].co = vertices[3 * tile + 1].co;
        select_verts[2].co = vertices[3 * tile + 2].co;

        /* Copy normals */
        select_verts[0].no = vertices[3 * tile].no;
        select_verts[1].no = vertices[3 * tile + 1].no;
        select_verts[2].no = vertices[3 * tile + 2].no;

        /* Set UV */
        select_verts[0].uv = C_vec2(0.5f, 0.0f);
        select_verts[1].uv = C_vec2(0.0f, 1.0f);
        select_verts[2].uv = C_vec2(1.0f, 1.0f);
}

