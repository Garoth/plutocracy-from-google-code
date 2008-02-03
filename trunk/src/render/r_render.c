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
#include "SDL.h"

extern r_static_mesh_t* r_mesh_data;

/******************************************************************************\
 Renders the scene (let's have a spinning cube for now?)
\******************************************************************************/
void R_render(void)
{
        static float x_rot = 0.0, y_rot = 0.0;
        static float white[] = { 1.0, 1.0, 1.0, 1.0 };

        /* I'm pretty sure w=0 means directional. */
        static float left[] = { -1.0, 0.0, 0.0, 0.0 };

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glLoadIdentity();

        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, left);
        glLightfv(GL_LIGHT0, GL_COLOR, white);

        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColor3f(1.0, 1.0, 1.0);

        glDisable(GL_CULL_FACE);

        glPushMatrix();
        glTranslatef(0.0, 0.0, -20.0);
        glRotatef(x_rot, 1.0, 0.0, 0.0);
        glRotatef(y_rot, 0.0, 1.0, 0.0);
        glTranslatef(0.0, -10.0, 0.0);

        /* Vertex arrays! */
        if(r_mesh_data) {
                glEnableClientState(GL_VERTEX_ARRAY);
                glEnableClientState(GL_NORMAL_ARRAY);
                glEnableClientState(GL_TEXTURE_COORD_ARRAY);

                glVertexPointer(3, GL_FLOAT, 0, r_mesh_data->verts);
                glNormalPointer(GL_FLOAT, 0, r_mesh_data->norms);
                glTexCoordPointer(2, GL_FLOAT, 0, r_mesh_data->sts);

                glDrawElements(GL_TRIANGLES,
                               r_mesh_data->ninds,
                               GL_UNSIGNED_SHORT,
                               r_mesh_data->inds);

                glDisableClientState(GL_TEXTURE_COORD_ARRAY);
                glDisableClientState(GL_NORMAL_ARRAY);
                glDisableClientState(GL_VERTEX_ARRAY);
        }

        glPopMatrix();
        x_rot += 0.03;
        y_rot += 0.1;

        SDL_GL_SwapBuffers();
}
