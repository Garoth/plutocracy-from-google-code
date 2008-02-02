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

/******************************************************************************\
 Renders the scene (let's have a spinning cube for now?)
\******************************************************************************/
void R_render(void)
{
        static float x_rot = 0.0, y_rot = 0.0;
        static float white[] = { 1.0, 1.0, 1.0, 1.0 };

        /* I'm pretty sure w=0 means directional. */
        static float left[] = { -1.0, 0.0, 0.0, 0.0 };

        glClear(GL_COLOR_BUFFER_BIT);
        glLoadIdentity();

        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, left);
        glLightfv(GL_LIGHT0, GL_COLOR, white);

        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

        glPushMatrix();
        glTranslatef(0.0, 0.0, -6.0);
        glRotatef(x_rot, 1.0, 0.0, 0.0);
        glRotatef(y_rot, 0.0, 1.0, 0.0);

        /* This is not how we generally draw things. */
        glBegin(GL_QUADS);
        {
                /* front */
                glColor3f(1.0, 0.0, 0.0);
                glNormal3f(0, 0, 1);
                glVertex3f(-1, -1, 1);
                glVertex3f(1, -1, 1);
                glVertex3f(1, 1, 1);
                glVertex3f(-1, 1, 1);

                /* left */
                glColor3f(0.0, 1.0, 0.0);
                glNormal3f(-1, 0, 0);
                glVertex3f(-1, -1, 1);
                glVertex3f(-1, 1, 1);
                glVertex3f(-1, 1, -1);
                glVertex3f(-1, -1, -1);

                /* back */
                glColor3f(1.0, 0.0, 0.0);
                glNormal3f(0, 0, -1);
                glVertex3f(-1, 1, -1);
                glVertex3f(1, 1, -1);
                glVertex3f(1, -1, -1);
                glVertex3f(-1, -1, -1);

                /* right */
                glColor3f(0.0, 1.0, 0.0);
                glNormal3f(1, 0, 0);
                glVertex3f(1, 1, -1);
                glVertex3f(1, 1, 1);
                glVertex3f(1, -1, 1);
                glVertex3f(1, -1, -1);

                /* top */
                glColor3f(0.0, 0.0, 1.0);
                glNormal3f(-1, 1, 0);
                glVertex3f(1, 1, -1);
                glVertex3f(-1, 1, -1);
                glVertex3f(-1, 1, 1);
                glVertex3f(1, 1, 1);

                /* bottom */
                glColor3f(0.0, 0.0, 1.0);
                glNormal3f(-1, -1, 0);
                glVertex3f(1, -1, 1);
                glVertex3f(-1, -1, 1);
                glVertex3f(-1, -1, -1);
                glVertex3f(1, -1, -1);
        }
        glEnd();

        glPopMatrix();
        x_rot += 0.01;
        y_rot += 0.05;

        SDL_GL_SwapBuffers();
}
