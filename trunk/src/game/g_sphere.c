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
 Generates the sphere to be used as the map for the game by tesselating a
 tetrahedron.
\******************************************************************************/
g_sphere_t *G_sphere_alloc(int subdiv_levels) {
        g_sphere_t *result;
        c_array_t verts, tris1, tris2, edges1, edges2;
        c_array_t *cur_tris, *other_tris, *cur_edges, *other_edges, *atmp;
        c_vec3_t vtmp;
        tri_t ttmp;
        edge_t etmp;
        c_vec3_t origin;
        float sphere_r;
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

        /* Initialize tetrahedron */

        /* Vertices */
        vtmp = C_vec3(-1, -1, 1);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(1, -1, -1);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(-1, 1, -1);
        C_array_append(&verts, &vtmp);
        vtmp = C_vec3(1, 1, 1);
        C_array_append(&verts, &vtmp);

        /* Re-center on origin (which we'll hope is the average of the points). */
        origin = C_vec3(0, 0, 0);
        for (loop = 0; loop < 4; loop++)
                origin = C_vec3_add(origin, C_array_elem(&verts, c_vec3_t, loop));
        origin = C_vec3_invscalef(origin, 4);

        for (loop = 0; loop < 4; loop++) {
                c_vec3_t *v;

                v = &C_array_elem(&verts, c_vec3_t, loop);
                *v = C_vec3_sub(*v, origin);
        }

        /* Remember radius */
        sphere_r = C_vec3_len(C_array_elem(&verts, c_vec3_t, 0));

        /* Triangles */
        ttmp.n = 0;
        ttmp.orig[0] = 0;
        ttmp.orig[1] = 1;
        ttmp.orig[2] = 3;
        C_array_append(cur_tris, &ttmp);
        ttmp.orig[0] = 2;
        ttmp.orig[1] = 0;
        C_array_append(cur_tris, &ttmp);
        ttmp.orig[0] = 1;
        ttmp.orig[1] = 2;
        C_array_append(cur_tris, &ttmp);
        ttmp.orig[1] = 0;
        ttmp.orig[2] = 2;
        C_array_append(cur_tris, &ttmp);

        /* Edges */
        etmp.v[0] = 0;
        etmp.v[1] = 3;
        etmp.t[0] = 0;
        etmp.t[1] = 1;
        C_array_append(cur_edges, &etmp);
        etmp.v[1] = 1;
        etmp.t[1] = 3;
        C_array_append(cur_edges, &etmp);
        etmp.v[0] = 1;
        etmp.v[1] = 3;
        etmp.t[1] = 2;
        C_array_append(cur_edges, &etmp);
        etmp.v[0] = 2;
        etmp.t[0] = 1;
        C_array_append(cur_edges, &etmp);
        etmp.v[0] = 1;
        etmp.v[1] = 2;
        etmp.t[0] = 3;
        C_array_append(cur_edges, &etmp);
        etmp.v[0] = 0;
        etmp.t[1] = 1;
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

                        /* Scale to radius of sphere */
                        //vtmp = C_vec3_scalef(vtmp, sphere_r / C_vec3_len(vtmp));

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
                        int j;

                        tri = &C_array_elem(cur_tris, tri_t, i);

                        /* ttmp.n is zero through all of this. */
                        ttmp.n = 0;

                        /* Loop through original vertices for the triangle.
                           Create the sub-triangle that owns that vertex. */
                        for (j = 0; j < 3; j++) {
                                ushort_t vert_ind;
                                edge_t *edge1, *edge2;
                                int k;

                                vert_ind= tri->orig[j];
                                edge1 = NULL;
                                edge2 = NULL;

                                /* Two of the edges will share this vertex */
                                for (k = 0; k < 3; k++) {
                                        edge_t *e;

                                        e = &C_array_elem(cur_edges, edge_t,
                                                          tri->new[k]);
                                        if (e->v[0] == vert_ind ||
                                            e->v[1] == vert_ind) {
                                                if (!edge1)
                                                        edge1 = e;
                                                else if (!edge2)
                                                        edge2 = e;
                                                else
                                                        C_error("error 2");
                                        }
                                }

                                /* sanity check */
                                if (other_tris->len != i * 4 + j)
                                        C_error("error 3 - length %d", other_tris->len);

                                /* Create the new edge that bridges these */
                                etmp.v[0] = edge1->i;
                                etmp.v[1] = edge2->i;
                                etmp.t[0] = i * 4 + j; /* this triangle */
                                etmp.t[1] = i * 4 + 3; /* center triangle */
                                C_array_append(other_edges, &etmp);

                                /* Create this triangle */
                                ttmp.orig[0] = vert_ind;
                                ttmp.orig[1] = edge1->i;
                                ttmp.orig[2] = edge2->i;

                                /* new[] can have garbage in it. */
                                C_array_append(other_tris, &ttmp);
                        }

                        /* Create the center sub-triangle (number 3) */
                        for (j = 0; j < 3; j++) {
                                edge_t *e;

                                e = &C_array_elem(cur_edges, edge_t, tri->new[j]);
                                ttmp.orig[j] = e->i;
                        }

                        C_array_append(other_tris, &ttmp);
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

        /* Put the sphere together. */
        result = C_malloc(sizeof(g_sphere_t));
        result->nverts = verts.len;
        result->neighbors_lists = C_malloc(verts.len * sizeof(g_vert_neighbors_t));
        result->verts = C_array_steal(&verts);

        /* Scale vertices to be on the sphere. */
        for (loop = 0; loop < result->nverts; loop++) {
                c_vec3_t *v;

                v = &result->verts[loop];
                *v = C_vec3_scalef(*v, sphere_r / C_vec3_len(*v));
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

        return result;
}

/******************************************************************************\
 Free the sphere structure.
\******************************************************************************/
void G_sphere_free(g_sphere_t *s)
{
        C_free(s->verts);
        C_free(s->neighbors_lists);
        C_free(s->inds);
        C_free(s);
}
