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

#define ITERATIONS_MAX 5

/* Globe vertex type */
#pragma pack(push, 4)
typedef struct globe_vertex {
        c_vec2_t uv;
        c_color_t color[3];
        c_vec3_t no;
        c_vec3_t co;
        int next;
} globe_vertex_t;
#pragma pack(pop)

int r_tiles;

static globe_vertex_t vertices[R_TILES_MAX * 3];
static r_texture_t *texture;
static float radius;
static int flip_limit;
static unsigned short indices[R_TILES_MAX * 3];

/******************************************************************************\
 Cleanup globe assets.
\******************************************************************************/
void R_cleanup_globe(void)
{
        R_texture_free(texture);
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
                vertices[i].co = C_vec3_scalef(vertices[i].co, radius /
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
        radius *= 2;
        sphericize();
}

/******************************************************************************\
 Finds vertex neighbors by iteration. Runs in O(n^2) time.
\******************************************************************************/
static void find_neighbors(void)
{
        int i, i_next, j, j_next;

        for (i = 0; i < r_tiles * 3; i++) {
                i_next = 3 * (i / 3) + (i + 1) % 3;
                for (j = 0; ; j++) {
                        if (j == i)
                                continue;
                        if (C_vec3_eq(vertices[i].co, vertices[j].co)) {
                                j_next = 3 * (j / 3) + (3 + j - 1) % 3;
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
        radius = sqrtf(C_TAU + 2);

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
void R_generate_globe(int seed, int subdiv4)
{
        int i;

        if (subdiv4 < 0)
                subdiv4 = 0;
        else if (subdiv4 > ITERATIONS_MAX) {
                subdiv4 = ITERATIONS_MAX;
                C_warning("Too many subdivisions requested");
        }

        C_debug("Generating globe with seed 0x%8x", seed);
        C_rand_seed(seed);
        memset(vertices, 0, sizeof (vertices));
        generate_icosahedron();

        C_debug("Subdividing globe %d times", subdiv4);
        for (i = 0; i < subdiv4; i++)
                subdivide4();

        /* Set indices and normals */
        for (i = 0; i < r_tiles * 3; i++) {
                indices[i] = i;
                vertices[i].no = C_vec3_norm(vertices[i].co);
        }

        /* Load the terrain texture */
        R_texture_free(texture);
        texture = R_texture_load("models/globe/terrain.png", TRUE);

        r_cam_dist = radius + R_ZOOM_MIN;
}

/******************************************************************************\
 Renders the entire globe.
\******************************************************************************/
void R_render_globe(void)
{
        float left[] = { -1.0, 0.0, 0.0, 0.0 };

        R_set_mode(R_MODE_3D);
        R_texture_select(texture);

        /* Have a light from the left */
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, left);

        /* Setup arrays and render the globe mesh */
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_NORMAL_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, sizeof (*vertices), &vertices[0].uv);
        glVertexPointer(3, GL_FLOAT, sizeof (*vertices), &vertices[0].co);
        glNormalPointer(GL_FLOAT, sizeof (*vertices), &vertices[0].no);
        glDrawElements(GL_TRIANGLES, 3 * r_tiles, GL_UNSIGNED_SHORT, indices);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);

        R_check_errors();
        C_count_add(&r_count_faces, r_tiles);
}

/******************************************************************************\
 Converts screen pixel distance to globe radians.
 FIXME: Hacky and inaccurate.
\******************************************************************************/
float R_screen_to_globe(int pixels)
{
        return 0.05f * r_pixel_scale.value.f * pixels /
               (r_cam_dist + 5.f * (R_ZOOM_MAX - R_ZOOM_MIN - r_cam_zoom));
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
 Adjusts globe verices to show the tile's height.
\******************************************************************************/
void R_configure_globe(r_tile_t *tiles)
{
        float bottom;
        int i, tx, ty, flip;

        C_debug("Configuring globe");
        for (i = 0; i < r_tiles; i++) {
                set_tile_height(i, tiles[i].height);

                /* Tile terrain texture */
                ty = tiles[i].terrain / 4;
                tx = tiles[i].terrain - ty * 4;
                bottom = (ty + 1) * 0.25f - 0.0351f;
                vertices[3 * i].uv = C_vec2(tx * 0.25f + 0.125f, ty * 0.25f);
                flip = i < flip_limit;
                vertices[3 * i + 1].uv = C_vec2((tx + flip) * 0.25f, bottom);
                vertices[3 * i + 2].uv = C_vec2((tx + !flip) * 0.25f, bottom);
        }
}

