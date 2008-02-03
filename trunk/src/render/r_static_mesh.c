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

/******************************************************************************\
 Resizing arrays for fun and profit. This is OH GOD SO UNTYPESAFE.
\******************************************************************************/
typedef struct r_ary {
        size_t capacity;
        size_t len;
        size_t item_size;
        void *elems;
} r_ary_t;

#define R_ary_elems(pary, type) ((type*)(pary)->elems)

/******************************************************************************\
 Initialize an array. Returns TRUE on success.
\******************************************************************************/
#define R_ary_init(ary, type, cap) \
        R_ary_init_real(ary, sizeof(type), cap)

static int R_ary_init_real(r_ary_t *ary, size_t item_size, size_t cap)
{
        ary->item_size = item_size;
        ary->len = 0;
        ary->capacity = cap;
        ary->elems = malloc(cap * item_size);
        if(!ary->elems) {
                return FALSE;
        }

        return TRUE;
}

/******************************************************************************\
 Ensure that enough space is allocated for n elems, but not necessarily more.
 Returns TRUE on success.
\******************************************************************************/
static int R_ary_reserve(r_ary_t *ary, size_t n)
{
        void* new_elems = realloc(ary->elems, ary->item_size * n);
        if(!new_elems) {
                return FALSE;
        }

        ary->elems = new_elems;
        ary->capacity = n;
        return TRUE;
}

/******************************************************************************\
 Append something to an array. item points to something of size item_size.
 Returns TRUE on success.
\******************************************************************************/
static int R_ary_append(r_ary_t *ary, void* item)
{
        if(ary->len >= ary->capacity &&
           !R_ary_reserve(ary, ary->capacity * 2)) {
                return FALSE;
        }

        memcpy((char*)ary->elems + ary->len * ary->item_size,
               item,
               ary->item_size);

        ary->len++;

        return TRUE;
}

/******************************************************************************\
 Clean up after the array.
\******************************************************************************/
static void R_ary_deinit(r_ary_t *ary)
{
        free(ary->elems);
        memset(ary, '\0', sizeof(*ary));
}

/****************************************************************************** \
 Find or insert the given (vert,norm,st) triplet into the parallel arrays
 given. Returns TRUE on success.
\******************************************************************************/
static int R_find_vert(r_ary_t *vs,
                       r_ary_t *ns,
                       r_ary_t *sts,
                       c_pt3_t v,
                       c_pt3_t n,
                       c_pt2_t st,
                       unsigned short* i)
{
        for(*i = 0; *i < vs->len; (*i)++) {
                if(C_pt3_eq(v, R_ary_elems(vs, c_pt3_t)[*i]) &&
                   C_pt3_eq(n, R_ary_elems(ns, c_pt3_t)[*i]) &&
                   C_pt2_eq(st, R_ary_elems(sts, c_pt2_t)[*i])) {
                        return TRUE;
                }
        }

        /* not found */

        if(!R_ary_append(vs, &v)) { return FALSE; }
        if(!R_ary_append(ns, &n)) {
                vs->len--;
                return FALSE;
        }
        if(!R_ary_append(sts, &st)) {
                vs->len--;
                ns->len--;
                return FALSE;
        }

        return TRUE;
}

/****************************************************************************** \
 Parse a v/vt/vn triplet as you would find in a face line in .OBJ format.
\******************************************************************************/
static char* R_parse_face(char* s,
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
                goto errfp;
        }

        r_ary_t verts, norms, sts;

        if(!R_ary_init(&verts, c_pt3_t, 1)) { goto errvs; }
        if(!R_ary_init(&norms, c_pt3_t, 1)) { goto errns; }
        if(!R_ary_init(&sts, c_pt2_t, 1)) { goto errsts; }

        r_ary_t real_verts, real_norms, real_sts, inds;
        if(!R_ary_init(&real_verts, c_pt3_t, 1)) { goto errrv; }
        if(!R_ary_init(&real_norms, c_pt3_t, 1)) { goto errrn; }
        if(!R_ary_init(&real_sts, c_pt2_t, 1)) { goto errrst; }
        if(!R_ary_init(&inds, unsigned short, 1)) { goto errind; }

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
                        r_ary_t* ary = (keyword[1] == 'n' ? &norms : &verts);
                        c_pt3_t v;
                        int i;

                        for(i = 0; i < 3; i++) {
                                char* endptr;
                                ((float*)&v)[i] = strtod(args, &endptr);
                                args = endptr;
                        }

                        if(ary == &norms) {
                                v = C_pt3_normalize(v);
                        }

                        R_ary_append(ary, &v);
                } else if(strcmp(keyword, "vt") == 0) {
                        /* texture coordinate */
                        c_pt2_t st;
                        int i;

                        for(i = 0; i < 2; i++) {
                                args = C_skip_spaces(args);
                                ((float*)&st)[i] = atof(args);
                        }

                        R_ary_append(&sts, &st);
                } else if(strcmp(keyword, "f") == 0) {
                        /* face */
                        char *s = args;
                        unsigned short first_ind, have_first = FALSE;
                        unsigned short last_ind, have_last = FALSE;
                        unsigned short ivert, inorm, ist;
                        while((s = R_parse_face(s, &ivert, &ist, &inorm))) {
                                unsigned short ind;
                                int find_res = R_find_vert(
                                        &real_verts,
                                        &real_norms,
                                        &real_sts,
                                        R_ary_elems(&verts, c_pt3_t)[ivert],
                                        R_ary_elems(&norms, c_pt3_t)[inorm],
                                        R_ary_elems(&sts, c_pt2_t)[ist],
                                        &ind);

                                if(!find_res) {
                                        goto errres; /* cleanup, it's over */
                                }

                                if(!have_first) {
                                        first_ind = ind;
                                        have_first = TRUE;
                                } else if(have_last) {
                                        R_ary_append(&inds, &first_ind);
                                        R_ary_append(&inds, &last_ind);
                                        R_ary_append(&inds, &ind);
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

        result = malloc(sizeof(r_static_mesh_t));
        if(!result) { goto errres; }

        result->nverts = verts.len;
        result->ninds = inds.len;

        /* Shrink arrays and keep them in result structure. */

        /* vertices */
        result->verts = realloc(real_verts.elems,
                                real_verts.len * sizeof(c_pt3_t));
        real_verts.elems = NULL;

        /* normals */
        result->norms = realloc(real_norms.elems,
                                real_verts.len * sizeof(c_pt3_t));
        real_norms.elems = NULL;

        /* texcoords */
        result->sts = realloc(real_sts.elems,
                              real_verts.len * sizeof(c_pt2_t));
        real_sts.elems = NULL;

        /* indices */
        result->inds = realloc(inds.elems, inds.len * sizeof(unsigned short));
        inds.elems = NULL;

errres: R_ary_deinit(&inds);
errind: R_ary_deinit(&real_sts);
errrst: R_ary_deinit(&real_norms);
errrn:  R_ary_deinit(&real_verts);
errrv:  R_ary_deinit(&sts);
errsts: R_ary_deinit(&norms);
errns:  R_ary_deinit(&verts);
errvs:  fclose(fp);
errfp:  return result;
}
