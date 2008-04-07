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
        int next, neighbor;
} globe_vertex_t;
#pragma pack(pop)

int r_tiles;

static globe_vertex_t vertices[R_TILES_MAX * 3];
static r_texture_t *texture;
static float radius;
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
\******************************************************************************/
static void subdivide4(void)
{
        c_vec3_t mid_0_1, mid_0_2, mid_1_2;
        globe_vertex_t *verts;
        int j;

        for (j = 0; j < r_tiles; j++) {
                mid_0_1 = C_vec3_add(vertices[3 * j].co,
                                     vertices[3 * j + 1].co);
                mid_0_1 = C_vec3_divf(mid_0_1, 2.f);
                mid_0_2 = C_vec3_add(vertices[3 * j].co,
                                     vertices[3 * j + 2].co);
                mid_0_2 = C_vec3_divf(mid_0_2, 2.f);
                mid_1_2 = C_vec3_add(vertices[3 * j + 1].co,
                                     vertices[3 * j + 2].co);
                mid_1_2 = C_vec3_divf(mid_1_2, 2.f);

                /* Top triangle (0) */
                verts = vertices + 3 * r_tiles + 9 * j;
                verts[0].co = vertices[3 * j].co;
                verts[1].co = mid_0_1;
                verts[2].co = mid_0_2;

                /* Bottom-left triangle (1) */
                verts[3].co = mid_0_1;
                verts[4].co = vertices[3 * j + 1].co;
                verts[5].co = mid_1_2;

                /* Bottom-right triangle (2) */
                verts[6].co = mid_0_2;
                verts[7].co = mid_1_2;
                verts[8].co = vertices[3 * j + 2].co;

                /* Center triangle (original) */
                vertices[3 * j].co = mid_1_2;
                vertices[3 * j + 1].co = mid_0_2;
                vertices[3 * j + 2].co = mid_0_1;
        }
        r_tiles *= 4;
        radius *= 2;
        sphericize();
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
        vertices[0].co  = C_vec3(0, 1, C_TAU);
        vertices[1].co  = C_vec3(1, C_TAU, 0);
        vertices[2].co  = C_vec3(-1, C_TAU, 0);
        vertices[3]     = vertices[ 0];
        vertices[4]     = vertices[ 2];
        vertices[5].co  = C_vec3(-C_TAU, 0, 1);
        vertices[6]     = vertices[ 0];
        vertices[7]     = vertices[ 5];
        vertices[8].co  = C_vec3(0, -1, C_TAU);
        vertices[9].co  = C_vec3(-1, -C_TAU, 0);
        vertices[10]    = vertices[ 8];
        vertices[11]    = vertices[ 5];
        vertices[12]    = vertices[ 8];
        vertices[13]    = vertices[ 9];
        vertices[14].co = C_vec3(1, -C_TAU, 0);
        vertices[15]    = vertices[ 8];
        vertices[16]    = vertices[14];
        vertices[17].co = C_vec3(C_TAU, 0, 1);
        vertices[18]    = vertices[ 0];
        vertices[19]    = vertices[ 8];
        vertices[20]    = vertices[17];
        vertices[21]    = vertices[ 0];
        vertices[22]    = vertices[17];
        vertices[23]    = vertices[ 1];
        vertices[24]    = vertices[ 1];
        vertices[25]    = vertices[17];
        vertices[26].co = C_vec3(C_TAU, 0, -1);
        vertices[27]    = vertices[26];
        vertices[28]    = vertices[17];
        vertices[29]    = vertices[14];
        vertices[30].co = C_vec3(0, -1, -C_TAU);
        vertices[31]    = vertices[26];
        vertices[32]    = vertices[14];
        vertices[33]    = vertices[30];
        vertices[34]    = vertices[14];
        vertices[35]    = vertices[ 9];
        vertices[36]    = vertices[30];
        vertices[37]    = vertices[ 9];
        vertices[38].co = C_vec3(-C_TAU, 0, -1);
        vertices[39]    = vertices[ 5];
        vertices[40]    = vertices[38];
        vertices[41]    = vertices[ 9];
        vertices[42]    = vertices[38];
        vertices[43]    = vertices[ 5];
        vertices[44]    = vertices[ 2];
        vertices[45].co = C_vec3(0, 1, -C_TAU);
        vertices[46]    = vertices[38];
        vertices[47]    = vertices[ 2];
        vertices[48]    = vertices[45];
        vertices[49]    = vertices[ 2];
        vertices[50]    = vertices[ 1];
        vertices[51]    = vertices[45];
        vertices[52]    = vertices[ 1];
        vertices[53]    = vertices[26];
        vertices[54]    = vertices[45];
        vertices[55]    = vertices[26];
        vertices[56]    = vertices[30];
        vertices[57]    = vertices[45];
        vertices[58]    = vertices[30];
        vertices[59]    = vertices[38];
        r_tiles = 20;
        radius = sqrtf(C_TAU + 2);
}

/******************************************************************************\
 Generates the globe by subdividing an icosahedron and spacing the vertices
 out at the sphere's surface.
\******************************************************************************/
void R_generate_globe(int seed, int subdiv4)
{
        int i, j;

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

        C_debug("Subdividng globe (%dx)", subdiv4);
        for (i = 0; i < subdiv4; i++)
                subdivide4();

        C_debug("Processing vertices");
        for (i = 0; i < r_tiles * 3; i++) {
                indices[i] = i;
                vertices[i].next = i;
                vertices[i].no = C_vec3_norm(vertices[i].co);

                /* Add this vertex to a circular linked list of co-located
                   vertices. This greatly speeds up finding tile neighbors. */
                for (j = 0; j < i; j++)
                        if (C_vec3_eq(vertices[j].co, vertices[i].co)) {
                                vertices[i].next = vertices[j].next;
                                vertices[j].next = i;
                                break;
                        }
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
                verts[i] = pos;
                pos = vertices[pos].next;
        }
        return i;
}

/******************************************************************************\
 Adjusts globe verices to show the tile's height.
\******************************************************************************/
void R_configure_globe(r_tile_t *tiles)
{
        float dist, height, bottom;
        int i, j, k, verts[6], verts_len, tx, ty;

        C_debug("Configuring globe");
        for (i = 0; i < r_tiles; i++) {

                /* Set tile height */
                for (j = 0; j < 3; j++) {
                        verts_len = vertex_indices(3 * i + j, verts);
                        height = tiles[i].height / verts_len;
                        for (k = 0; k < verts_len; k++) {
                                dist = C_vec3_len(vertices[verts[k]].co);
                                vertices[verts[k]].co =
                                        C_vec3_scalef(vertices[verts[k]].co,
                                                      (dist + height) / dist);
                        }
                }

                /* Tile texture */
                ty = tiles[i].terrain / 4;
                tx = tiles[i].terrain - ty * 4;
                bottom = (ty + 1) * 0.25f - 0.06f;
                vertices[3 * i].uv = C_vec2(tx * 0.25f + 0.125f, ty * 0.25f);
                vertices[3 * i + 1].uv = C_vec2(tx * 0.25f, bottom);
                vertices[3 * i + 2].uv = C_vec2((tx + 1) * 0.25f, bottom);
        }
}

/******************************************************************************\
 Finds all tiles that share an edge with this tile.
\******************************************************************************/
void R_find_tile_neighbors(int index, int neighbors[3])
{
        int i, j, neighbors_len, tile, tiles[12];
        unsigned short original, pos;

        neighbors_len = 0;
        tiles[0] = -1;
        for (i = 0; i < 3; i++) {
                original = 3 * index + i;
                pos = vertices[original].next;
                while (pos != original) {
                        tile = pos / 3;
                        for (j = 0; tiles[j] >= 0; j++)
                                if (tiles[j] == tile) {
                                        neighbors[neighbors_len++] = tile;
                                        if (neighbors_len >= 3)
                                                return;
                                        goto next_tile;
                                }
                        tiles[j] = tile;
                        tiles[j + 1] = -1;
next_tile:              pos = vertices[pos].next;
                }
        }
        C_error("Tile %d does not have three neighbors", index);
}

