/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Devin Papineau

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "r_common.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/****************************************************************************** \
 Find or insert the given (vert,norm,st) triplet into the parallel arrays
 given. Returns TRUE on success.
\******************************************************************************/
static unsigned short find_vert(c_array_t *vs,
                                c_array_t *ns,
                                c_array_t *sts,
                                c_vec3_t v,
                                c_vec3_t n,
                                c_vec2_t st)
{
        unsigned short i;

        for(i = 0; i < vs->len; i++) {
                if(c_vec3_eq(v, C_array_elem(vs, c_vec3_t, i)) &&
                   c_vec3_eq(n, C_array_elem(ns, c_vec3_t, i)) &&
                   c_vec2_eq(st, C_array_elem(sts, c_vec2_t, i))) {
                        return i;
                }
        }

        /* not found */

        C_array_append(vs, &v);
        C_array_append(ns, &n);
        C_array_append(sts, &st);

        return i;
}

/****************************************************************************** \
 Parse a v/vt/vn triplet as you would find in a face line in .OBJ format.
\******************************************************************************/
static char* parse_face(char* s,
                        unsigned short* ivert,
                        unsigned short* ist,
                        unsigned short* inorm)
{
        s = C_skip_spaces(s);
        if(!*s) { return NULL; }

        unsigned short* ptrs[] = { ivert, ist, inorm };
        char *endptr;

        int i;
        for(i = 0; i < 3; i++) {
                if(i) {
                        if(*s != '/') {
                                C_debug("invalid face line, treating %c as /",
                                        *s);
                        } else {
                                s++;
                        }
                }

                /* OBJ indices are 1-indexed */
                *ptrs[i] = (unsigned short)strtol(s, &endptr, 0);
                if(*ptrs[i] > 0) { (*ptrs[i])--; }
                s = endptr;
        }

        return s;
}


/****************************************************************************** \
 Loads a static mesh from a .OBJ text file.
\******************************************************************************/
r_static_mesh_t* R_load_static_mesh(const char* filename)
{
        r_static_mesh_t* result = NULL;

        FILE* fp = fopen(filename, "r");
        if(!fp) {
                C_debug("can't open file %s", filename);
                return NULL;
        }

        c_array_t verts, norms, sts;
        C_array_init(&verts, c_vec3_t, 1);
        C_array_init(&norms, c_vec3_t, 1);
        C_array_init(&sts, c_vec2_t, 1);

        c_array_t real_verts, real_norms, real_sts, inds;
        C_array_init(&real_verts, c_vec3_t, 1);
        C_array_init(&real_norms, c_vec3_t, 1);
        C_array_init(&real_sts, c_vec2_t, 1);
        C_array_init(&inds, unsigned short, 1);

        int flag = TRUE;

        char buffer[BUFSIZ];
        while(fgets(buffer, BUFSIZ, fp)) {
                size_t len = strlen(buffer);
                if(buffer[len - 1] == '\n') { buffer[--len] = '\0'; }

                char* keyword = C_skip_spaces(buffer);
                if(!*keyword || *keyword == '#') {
                        /* empty/comment */
                        continue;
                }

                char* args = keyword;
                while(*args && !isspace(*args)) { args++; }
                if(*args) {
                        *args++ = '\0';
                }

                if(strcmp(keyword, "v") == 0 || strcmp(keyword, "vn") == 0) {
                        /* vertex or normal */
                        c_array_t* ary = (keyword[1] == 'n' ? &norms : &verts);
                        c_vec3_t v;
                        int i;

                        for(i = 0; i < 3; i++) {
                                char* endptr;
                                ((float*)&v)[i] = strtod(args, &endptr);
                                args = endptr;
                        }

                        if(ary == &norms) {
                                v = c_vec3_normalize(v);
                        }

                        C_array_append(ary, &v);
                } else if(strcmp(keyword, "vt") == 0) {
                        /* texture coordinate */
                        c_vec2_t st;
                        int i;

                        for(i = 0; i < 2; i++) {
                                args = C_skip_spaces(args);
                                ((float*)&st)[i] = atof(args);
                        }

                        C_array_append(&sts, &st);
                } else if(strcmp(keyword, "f") == 0) {
                        /* face */
                        char *s = args;
                        unsigned short first_ind, have_first = FALSE;
                        unsigned short last_ind, have_last = FALSE;
                        unsigned short ivert, inorm, ist;
                        while((s = parse_face(s, &ivert, &ist, &inorm))) {
                                unsigned short ind;
                                ind = find_vert(
                                        &real_verts,
                                        &real_norms,
                                        &real_sts,
                                        C_array_elem(&verts, c_vec3_t, ivert),
                                        C_array_elem(&norms, c_vec3_t, inorm),
                                        C_array_elem(&sts, c_vec2_t, ist));

                                if(!have_first) {
                                        first_ind = ind;
                                        have_first = TRUE;
                                } else if(have_last) {
                                        C_array_append(&inds, &first_ind);
                                        C_array_append(&inds, &last_ind);
                                        C_array_append(&inds, &ind);
                                } else {
                                        have_last = TRUE;
                                }

                                last_ind = ind;
                        }
                        flag = FALSE;
                }
        }

        C_debug("loaded %s: %d entries, %d indices",
                filename,
                real_verts.len,
                inds.len);

        result = C_malloc(sizeof(r_static_mesh_t));

        result->nverts = verts.len;
        result->ninds = inds.len;

        /* Shrink arrays and keep them in result structure. */

        /* vertices */
        result->verts = C_realloc(real_verts.elems,
                                  real_verts.len * sizeof(c_vec3_t));
        real_verts.elems = NULL;

        /* normals */
        result->norms = C_realloc(real_norms.elems,
                                  real_verts.len * sizeof(c_vec3_t));
        real_norms.elems = NULL;

        /* texcoords */
        result->sts = C_realloc(real_sts.elems,
                                real_verts.len * sizeof(c_vec2_t));
        real_sts.elems = NULL;

        /* indices */
        result->inds = C_realloc(inds.elems,
                                 inds.len * sizeof(unsigned short));
        inds.elems = NULL;

        C_array_deinit(&inds);
        C_array_deinit(&real_sts);
        C_array_deinit(&real_norms);
        C_array_deinit(&real_verts);
        C_array_deinit(&sts);
        C_array_deinit(&norms);
        C_array_deinit(&verts);
        fclose(fp);

        return result;
}
