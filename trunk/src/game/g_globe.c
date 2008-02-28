/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Devin Papineau

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "g_common.h"
#include <stdio.h>

/* Define the radius of the globe */
#define G_GLOBE_RADIUS (1000.0)

/* Some local datatypes. */
typedef unsigned short ushort_t;

/* orig are vertex indices for the vertices for the triangle.
   new are edge indices for the edges involved. */
typedef struct tri {
        ushort_t orig[3], new[3];
        unsigned char n; /* Number of new that have been filled. */
} tri_t;

typedef struct edge {
        ushort_t i, v[2], t[2];
} edge_t;

/******************************************************************************\
 Set the indices of a triangle.
\******************************************************************************/
static void set_tri(tri_t *tri, ushort_t a, ushort_t b, ushort_t c)
{
        tri->orig[0] = a;
        tri->orig[1] = b;
        tri->orig[2] = c;
}

/******************************************************************************\
 Fill in an edge. The compiler bitches if I use ushort_t here. Wonder why...?
\******************************************************************************/
static void set_edge(edge_t *edge,
                     unsigned short v0, unsigned short v1,
                     unsigned short t0, unsigned short t1)
{
        edge->v[0] = v0;
        edge->v[1] = v1;
        edge->t[0] = t0;
        edge->t[1] = t1;
}

/******************************************************************************\
 Seed the noise and generate heights based on 3D fractal noise, and offset each
 point according to it. The water vertices are offset as though their height
 value had been [water].
\******************************************************************************/
static void offset_terrain(unsigned int seed, float water, g_globe_t *g)
{
        int i;
        float min_noise = 0.0, max_noise = 0.0;

        C_noise3_seed(seed);
        for (i = 0; i < g->nverts; i++) {
                c_vec3_t *v;
                float noise;
                c_vec3_t offset;

                v = &g->verts[i];
                noise = C_noise3_fractal(8, v->x / 200, v->y / 200, v->z / 200);
                offset = C_vec3_scalef(*v, 0.1 * noise);
                *v = C_vec3_add(*v, offset);

                v = &g->water_verts[i];
                offset = C_vec3_scalef(*v, 0.1 * water);
                *v = C_vec3_add(*v, offset);

                min_noise = min_noise < noise ? min_noise : noise;
                max_noise = max_noise > noise ? max_noise : noise;
        }

        C_debug("minimum noise: %f", min_noise);
        C_debug("maximum noise: %f", max_noise);
}


/******************************************************************************\
 Compute normal values based on geometry.
\******************************************************************************/
static void compute_normals(g_globe_t *g)
{
        int i;

        g->norms = C_malloc(g->nverts * sizeof (c_vec3_t));
        for (i = 0; i < g->nverts; i++)
                g->norms[i] = C_vec3(0, 0, 0);

        for (i = 0; i < g->ninds; i += 3) {
                c_vec3_t a, b, face_norm;
                int j;

                a = C_vec3_sub(g->verts[g->inds[i]], g->verts[g->inds[i+1]]);
                b = C_vec3_sub(g->verts[g->inds[i]], g->verts[g->inds[i+2]]);
                face_norm = C_vec3_norm(C_vec3_cross(a, b));

                for (j = 0; j < 3; j++) {
                        int vind;

                        vind = g->inds[i + j];
                        g->norms[vind] = C_vec3_add(g->norms[vind], face_norm);
                }
        }

        for (i = 0; i < g->nverts; i++)
                g->norms[i] = C_vec3_norm(g->norms[i]);
}

/******************************************************************************\
 Greedily mark all connected land vertices as the specified island.
\******************************************************************************/
static void mark_island(g_globe_t *g, unsigned short vert, unsigned short isle)
{
        int i;

        if (!g->landp[vert] || g->island_ids[vert] != 0)
                return;

        g->island_ids[vert] = isle;
        for (i = 0; i < g->neighbors_lists[vert].count; i++)
                mark_island(g, g->neighbors_lists[vert].indices[i], isle);
}

/****************************************************************************** \
 Determines which vertices are on which islands, and which water tiles "belong"
 to which islands. Skip water for now.
\******************************************************************************/
static void identify_islands(g_globe_t *g)
{
        int i;
        float water_len_sqr;

        g->n_islands = 0;
        g->landp = C_malloc(g->nverts);
        g->island_ids = C_malloc(g->nverts * sizeof (unsigned short));

        memset(g->landp, '\0', g->nverts);
        memset(g->island_ids, '\0', g->nverts * sizeof (unsigned short));

        water_len_sqr = C_vec3_square_len(g->water_verts[0]);
        for (i = 0; i < g->nverts; i++)
                if (C_vec3_square_len(g->verts[i]) >= water_len_sqr)
                        g->landp[i] = 1;

        C_debug("marked land verts");

        for (i = 0; i < g->nverts; i++)
                if (g->landp[i] && g->island_ids[i] == 0)
                        mark_island(g, i, ++g->n_islands);

        C_debug("%d islands on globe", g->n_islands);
}

/****************************************************************************** \
 Generates the globe to be used as the map for the game by tesselating a
 tetrahedron.
\******************************************************************************/
g_globe_t *G_globe_alloc(int subdiv_levels, unsigned int seed, float water)
{
        g_globe_t *result;
        c_array_t verts, tris1, tris2, edges1, edges2;
        c_array_t *cur_tris, *other_tris, *cur_edges, *other_edges, *atmp;
        c_vec3_t vtmp;
        tri_t ttmp;
        edge_t etmp;
        c_vec3_t origin;
        float tau, phi, one;
        int loop;

        C_array_init(&verts, c_vec3_t, 512);
        C_array_init(&tris1, tri_t, 512);
        C_array_init(&tris2, tri_t, 512);
        C_array_init(&edges1, edge_t, 512);
        C_array_init(&edges2, edge_t, 512);

        cur_tris = &tris1;
        other_tris = &tris2;
        cur_edges = &edges1;
        other_edges = &edges2;

        /* Initialize icosahedron */
        phi = (1 + sqrt(5)) / 2;
        tau = phi / sqrt(1 + phi*phi);
        one = 1 / sqrt(1 + phi*phi);

        /* Vertices */
        vtmp = C_vec3(tau, one, 0.0);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(-tau, one, 0.0);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(-tau, -one, 0.0);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(tau, -one, 0.0);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(one, 0.0, tau);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(one, 0.0, -tau);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(-one, 0.0, -tau);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(-one, 0.0, tau);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(0.0, tau, one);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(0.0, -tau, one);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(0.0, -tau, -one);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(0.0, tau, -one);
        C_array_append(&verts, &vtmp);

        /* Re-center on origin (which we'll hope is the average of the points). */
        origin = C_vec3(0, 0, 0);
        for (loop = 0; loop < verts.len; loop++)
                origin = C_vec3_add(origin, C_array_elem(&verts, c_vec3_t, loop));
        origin = C_vec3_invscalef(origin, verts.len);

        for (loop = 0; loop < verts.len; loop++) {
                c_vec3_t *v;

                v = &C_array_elem(&verts, c_vec3_t, loop);
                *v = C_vec3_sub(*v, origin);
                *v = C_vec3_scalef(*v, G_GLOBE_RADIUS / C_vec3_len(*v));
        }

        /* Triangles */
        ttmp.n = 0;
        set_tri(&ttmp, 4, 8, 7);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 4, 7, 9);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 5, 6, 11);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 5, 10, 6);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 0, 4, 3);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 0, 3, 5);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 2, 7, 1);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 2, 1, 6);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 8, 0, 11);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 8, 11, 1);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 9, 10, 3);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 9, 2, 10);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 8, 4, 0);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 11, 0, 5);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 4, 9, 3);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 5, 3, 10);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 7, 8, 1);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 6, 1, 11);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 7, 2, 9);
        C_array_append(cur_tris, &ttmp);
        set_tri(&ttmp, 6, 10, 2);
        C_array_append(cur_tris, &ttmp);

        /* Edges */
        set_edge(&etmp, 4, 8, 0, 12);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 4, 7, 0, 1);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 7, 8, 0, 16);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 4, 9, 1, 14);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 7, 9, 1, 18);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 5, 6, 2, 3);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 5, 11, 2, 13);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 6, 11, 2, 17);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 5, 10, 3, 15);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 6, 10, 3, 19);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 0, 4, 4, 12);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 0, 3, 4, 5);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 3, 4, 4, 14);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 0, 5, 5, 13);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 3, 5, 5, 15);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 2, 7, 6, 18);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 1, 2, 6, 7);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 1, 7, 6, 16);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 2, 6, 7, 19);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 1, 6, 7, 17);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 0, 8, 8, 12);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 8, 11, 8, 9);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 0, 11, 8, 13);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 1, 11, 9, 17);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 1, 8, 9, 16);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 9, 10, 10, 11);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 3, 9, 10, 14);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 3, 10, 10, 15);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 2, 9, 11, 18);
        C_array_append(cur_edges, &etmp);
        set_edge(&etmp, 2, 10, 11, 19);
        C_array_append(cur_edges, &etmp);

        /* That was gross. Now we do a bunch of iterations of subdivision. */
        for (loop = 0; loop < subdiv_levels; loop++) {
                int i;

                /* Create a point at the middle of each edge. */
                for (i = 0; i < cur_edges->len; i++) {
                        ushort_t vert_ind;
                        c_vec3_t vtmp2;
                        int j;

                        vert_ind = C_array_elem(cur_edges, edge_t, i).v[0];
                        vtmp = C_array_elem(&verts, c_vec3_t, vert_ind);

                        vert_ind = C_array_elem(cur_edges, edge_t, i).v[1];
                        vtmp2 = C_array_elem(&verts, c_vec3_t, vert_ind);

                        vtmp = C_vec3_add(vtmp, vtmp2);
                        vtmp = C_vec3_invscalef(vtmp, 2);

                        C_array_elem(cur_edges, edge_t, i).i = verts.len;
                        C_array_append(&verts, &vtmp);

                        /* Tell each tri with this edge that they have a
                           new vertex. */
                        for (j = 0; j < 2; j++) {
                                int t;
                                tri_t *tri;

                                t = C_array_elem(cur_edges, edge_t, i).t[j];
                                tri = &C_array_elem(cur_tris, tri_t, t);

                                /* edge number. */
                                tri->new[tri->n++] = i;
                        }
                }

                /* For each edge, take the point in the middle and create
                   its edges that won't be added later. */
                for (i = 0; i < cur_edges->len; i++) {
                        edge_t *edge = &C_array_elem(cur_edges, edge_t, i);
                        int j;

                        etmp.v[0] = edge->i;

                        /* Two are to the existing vertices in the edge.
                           We create the in-triangle edges when we actually
                           create the sub-triangles to which they belong. */
                        for (j = 0; j < 2; j++) {
                                int k;

                                etmp.v[1] = edge->v[j];
                                for (k = 0; k < 2; k++) {
                                        tri_t *tri;
                                        int l;

                                        tri = &C_array_elem(cur_tris, tri_t,
                                                            edge->t[k]);

                                        /* Predict triangle numbering. */
                                        for (l = 0; l < 3; l++)
                                                if(tri->orig[l]==etmp.v[1])
                                                        break;

                                        if (l == 3)
                                                C_error("error 1");

                                        etmp.t[k] = edge->t[k] * 4 + l;
                                }

                                C_array_append(other_edges, &etmp);
                        }
                }

                /* For each triangle, actually do the subdivision. */
                for (i = 0; i < cur_tris->len; i++) {
                        tri_t *tri;
                        tri_t center_tri;
                        int j;

                        tri = &C_array_elem(cur_tris, tri_t, i);

                        /* ttmp.n is zero through all of this. */
                        ttmp.n = 0;
                        center_tri.n = 0;

                        /* Loop through original vertices for the triangle.
                           Create the sub-triangle that owns that vertex. */
                        for (j = 0; j < 3; j++) {
                                edge_t *edge1, *edge2;
                                unsigned short vert_ind0, vert_ind1, vert_ind2;
                                int k;

                                vert_ind0 = tri->orig[j];
                                vert_ind1 = tri->orig[(j + 1) % 3];
                                vert_ind2 = tri->orig[(j + 2) % 3];
                                edge1 = NULL;
                                edge2 = NULL;

                                /* Two of the edges will share this vertex.
                                   Our vertices are in CCW order, pick them
                                   that way */
                                for (k = 0; k < 3; k++) {
                                        edge_t *e;

                                        e = &C_array_elem(cur_edges, edge_t,
                                                          tri->new[k]);
                                        if (e->v[0] == vert_ind0) {
                                                if (e->v[1] == vert_ind1)
                                                        edge1 = e;
                                                else if (e->v[1] == vert_ind2)
                                                        edge2 = e;
                                        } else if(e->v[1] == vert_ind0) {
                                                if (e->v[0] == vert_ind1)
                                                        edge1 = e;
                                                else if (e->v[0] == vert_ind2)
                                                        edge2 = e;
                                        }
                                }

                                /* sanity check */
                                if (!edge1 || !edge2)
                                        C_error("error 5");

                                if (other_tris->len != i * 4 + j)
                                        C_error("error 3");

                                /* Create the new edge that bridges these */
                                etmp.v[0] = edge1->i;
                                etmp.v[1] = edge2->i;
                                etmp.t[0] = i * 4 + j; /* this triangle */
                                etmp.t[1] = i * 4 + 3; /* center triangle */
                                C_array_append(other_edges, &etmp);

                                /* Create this triangle */
                                ttmp.orig[0] = vert_ind0;
                                ttmp.orig[1] = edge1->i;
                                ttmp.orig[2] = edge2->i;

                                /* new[] can have garbage in it. */
                                C_array_append(other_tris, &ttmp);

                                /* Center triangle, incrementally */
                                center_tri.orig[j] = edge1->i;
                        }

                        /* Create the center sub-triangle (number 3) */
                        C_array_append(other_tris, &center_tri);
                }

                /* For the next level of subdivision,
                   switch the arrays and the new other_* arrays should start empty.   */
                atmp = cur_tris;
                cur_tris = other_tris;
                other_tris = atmp;

                atmp = cur_edges;
                cur_edges = other_edges;
                other_edges = atmp;

                other_tris->len = 0;
                other_edges->len = 0;
        }

        /* That was even more gross */

        /* Put the globe together. */
        result = C_malloc(sizeof(g_globe_t));
        result->nverts = verts.len;
        result->neighbors_lists = C_malloc(verts.len * sizeof(g_vert_neighbors_t));
        result->verts = C_array_steal(&verts);

        /* Scale vertices to be on the globe. */
        for (loop = 0; loop < result->nverts; loop++) {
                c_vec3_t *v;

                v = &result->verts[loop];
                *v = C_vec3_scalef(*v, G_GLOBE_RADIUS / C_vec3_len(*v));
        }

        /* No neighbours initially */
        for (loop = 0; loop < result->nverts; loop++)
                result->neighbors_lists[loop].count = 0;

        /* Go through edges and fill in neighbors */
        for (loop = 0; loop < cur_edges->len; loop++) {
                edge_t *edge;
                g_vert_neighbors_t *nbs1, *nbs2;

                edge = &C_array_elem(cur_edges, edge_t, loop);
                nbs1 = &result->neighbors_lists[edge->v[0]];
                nbs2 = &result->neighbors_lists[edge->v[1]];

                nbs1->indices[nbs1->count++] = edge->v[1];
                nbs2->indices[nbs2->count++] = edge->v[0];

                /* sanity check */
                if (nbs1->count > 6 || nbs2->count > 6)
                        C_error("error 4");
        }

        /* Create index array for triangles */
        result->ninds = 0;
        result->inds = C_malloc(3 * cur_tris->len * sizeof(ushort_t));
        for (loop = 0; loop < cur_tris->len; loop++) {
                tri_t *tri;
                int i;

                tri = &C_array_elem(cur_tris, tri_t, loop);
                for (i = 0; i < 3; i++)
                        result->inds[result->ninds++] = tri->orig[i];
        }

        C_array_cleanup(&tris1);
        C_array_cleanup(&tris2);
        C_array_cleanup(&edges1);
        C_array_cleanup(&edges2);

        /* Calculate terrain while keeping the originals for water. */
        result->water_verts = C_malloc(result->nverts * sizeof(c_vec3_t));
        memcpy(result->water_verts, result->verts,
               result->nverts * sizeof(c_vec3_t));

        offset_terrain(seed, water, result);

        /* And some more stuff */
        compute_normals(result);
        identify_islands(result);

        return result;
}

/******************************************************************************\
 Free the globe structure.
\******************************************************************************/
void G_globe_free(g_globe_t *s)
{
        if (!s)
                return;
        C_free(s->verts);
        C_free(s->water_verts);
        C_free(s->norms);
        C_free(s->neighbors_lists);
        C_free(s->inds);
        C_free(s->island_ids);
        C_free(s->landp);
        C_free(s);
}
