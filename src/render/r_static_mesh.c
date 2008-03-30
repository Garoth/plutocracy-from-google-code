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

/******************************************************************************\
 Find a vertex in a static mesh vertex array. Returns the length of the array
 if a matching vertex is not found.
\******************************************************************************/
unsigned short R_static_mesh_find_vert(const r_static_mesh_t *mesh,
                                       const r_vertex3_t *vert)
{
        int i;

        for (i = 0; i < mesh->verts_len; i++)
                if (C_vec3_eq(vert->co, mesh->verts[i].co) &&
                    C_vec3_eq(vert->no, mesh->verts[i].no) &&
                    C_vec2_eq(vert->uv, mesh->verts[i].uv))
                        break;
        return (unsigned short)i;
}

/******************************************************************************\
 Render a mesh.
\******************************************************************************/
void R_static_mesh_render(r_static_mesh_t *mesh, r_texture_t *texture)
{
        R_set_mode(R_MODE_3D);
        R_texture_select(texture);
        C_count_add(&r_count_faces, mesh->indices_len / 3);
        glInterleavedArrays(R_VERTEX3_FORMAT, 0, mesh->verts);
        glDrawElements(GL_TRIANGLES, mesh->indices_len,
                       GL_UNSIGNED_SHORT, mesh->indices);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        R_check_errors();
}

/******************************************************************************\
 Release the resources for a mesh.
\******************************************************************************/
void R_static_mesh_cleanup(r_static_mesh_t *mesh)
{
        if (!mesh)
                return;
        C_free(mesh->verts);
        C_free(mesh->indices);
}

/******************************************************************************\
 Release the resources for and free an allocated mesh.
\******************************************************************************/
void R_static_mesh_free(r_static_mesh_t *mesh)
{
        R_static_mesh_cleanup(mesh);
        C_free(mesh);
}

