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

/******************************************************************************\
 Find or insert the given (vert, norm, uv) triplet into the parallel arrays
 given. Returns TRUE on success.
\******************************************************************************/
static unsigned short find_vert(c_array_t *vs, c_vec3_t v, c_vec3_t n,
                                c_vec2_t uv)
{
        r_vertex_t new_vert;
        unsigned short i;

        for (i = 0; i < vs->len; i++) {
                r_vertex_t *vert;

                vert = &C_array_elem(vs, r_vertex_t, i);
                if (C_vec3_eq(v, vert->co) &&
                    C_vec3_eq(n, vert->no) &&
                    C_vec2_eq(uv, vert->uv)) {
                        return i;
                }
        }
        new_vert.co = v;
        new_vert.no = n;
        new_vert.uv = uv;
        C_array_append(vs, &new_vert);
        return i;
}

/******************************************************************************\
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


/******************************************************************************\
 Loads a static mesh from a .OBJ text file.
   FIXME: Need to get rid of fgets() to convert to C_file_*
\******************************************************************************/
r_static_mesh_t* R_static_mesh_load(const char* filename)
{
        r_static_mesh_t* result = NULL;

        FILE* fp = fopen(filename, "r");
        if(!fp) {
                C_debug("can't open file %s", filename);
                return NULL;
        }

        c_array_t verts, norms, sts;
        C_array_init(&verts, c_vec3_t, 512);
        C_array_init(&norms, c_vec3_t, 512);
        C_array_init(&sts, c_vec2_t, 512);

        c_array_t real_verts, inds;
        C_array_init(&real_verts, r_vertex_t, 512);
        C_array_init(&inds, unsigned short, 512);

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
                                v = C_vec3_norm(v);
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

        C_debug("Loaded '%s' (%d entries, %d indices)",
                filename, real_verts.len, inds.len);

        result = C_malloc(sizeof(r_static_mesh_t));
        result->verts_len = verts.len;
        result->indices_len = inds.len;

        /* Shrink arrays and keep them in result structure. */
        result->verts = C_array_steal(&real_verts);
        result->indices = C_array_steal(&inds);

        /* Cleanup temporary arrays. */
        C_array_cleanup(&sts);
        C_array_cleanup(&norms);
        C_array_cleanup(&verts);
        fclose(fp);

        return result;
}

/******************************************************************************\
 Render a mesh.
\******************************************************************************/
void R_static_mesh_render(r_static_mesh_t* mesh)
{
        glInterleavedArrays(R_VERTEX_FORMAT, 0, mesh->verts);
        glDrawElements(GL_TRIANGLES, mesh->indices_len,
                       GL_UNSIGNED_SHORT, mesh->indices);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
}

/******************************************************************************\
 Release the resources for a mesh.
\******************************************************************************/
void R_static_mesh_free(r_static_mesh_t* mesh)
{
        if (!mesh)
                return;
        C_free(mesh->verts);
        C_free(mesh->indices);
        C_free(mesh);
}
