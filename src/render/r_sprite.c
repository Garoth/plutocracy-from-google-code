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
 Initialize a sprite structure.
\******************************************************************************/
void R_sprite_init(r_sprite_t *sprite, const char *filename)
{
        if (!sprite)
                return;
        C_zero(sprite);
        sprite->texture = R_texture_load(filename);
        if (sprite->texture) {
                sprite->size.x = sprite->texture->surface->w;
                sprite->size.y = sprite->texture->surface->h;
        }
}

/******************************************************************************\
 Initialize a sprite structure.
\******************************************************************************/
void R_sprite_cleanup(r_sprite_t *sprite)
{
        if (!sprite)
                return;
        R_texture_free(sprite->texture);
        C_zero(sprite);
}

/******************************************************************************\
 Renders a 2D textured quad on screen. If we are rendering a rotated sprite
 that doesn't use alpha, this function will draw an anti-aliased border for it.
   TODO: Disable anti-aliasing here if FSAA is on.
\******************************************************************************/
void R_sprite_render(r_sprite_t *sprite)
{
        r_vertex2_t verts[4];
        unsigned short indices[] = { 0, 1, 2, 3, 0 };
        float dw, dh;

        if (!sprite || !sprite->texture)
                return;
        R_set_mode(R_MODE_2D);
        dw = sprite->size.x / 2;
        dh = sprite->size.y / 2;
        verts[0].co = C_vec3(-dw - 0.5f, dh - 0.5f, 0.f);
        verts[0].uv = C_vec2(0.f, 1.f);
        verts[1].co = C_vec3(-dw - 0.5f, -dh - 0.5f, 0.f);
        verts[1].uv = C_vec2(0.f, 0.f);
        verts[2].co = C_vec3(dw - 0.5f, -dh - 0.5f, 0.f);
        verts[2].uv = C_vec2(1.f, 0.f);
        verts[3].co = C_vec3(dw - 0.5f, dh - 0.5f, 0.f);
        verts[3].uv = C_vec2(1.f, 1.f);
        C_count_add(&r_count_faces, 2);
        R_texture_select(sprite->texture);
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(sprite->origin.x, sprite->origin.y, sprite->origin.z);
        glRotatef(C_rad_to_deg(sprite->angle), 0.0, 0.0, 1.0);
        glInterleavedArrays(R_VERTEX2_FORMAT, 0, verts);
        glDrawElements(GL_QUADS, 4, GL_UNSIGNED_SHORT, indices);
        if (!sprite->texture->alpha && sprite->angle != 0.f) {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_BLEND);
                glDrawElements(GL_LINE_STRIP, 5, GL_UNSIGNED_SHORT, indices);
        }
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glPopMatrix();
        R_check_errors();
}

