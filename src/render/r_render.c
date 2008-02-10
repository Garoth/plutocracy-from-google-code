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
#include "../game/g_shared.h"

/* Keep track of how many faces we render each frame */
c_count_t r_count_faces;

/* Render testing */
extern c_var_t r_test_globe, r_test_model_path;
extern r_static_mesh_t *r_test_mesh;
extern r_model_t r_test_model;

/******************************************************************************\
 Creates the client window. Initializes OpenGL settings such view matrices,
 culling, and depth testing.Returns TRUE on success.
\******************************************************************************/
int R_render_init(void)
{
        SDL_Surface *screen;
        int flags;

        C_status("Opening window");

        /* Initialize counters */
        C_count_init(&r_count_faces);

        /* Set OpenGL attributes */
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, r_depth.value.n);
        SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, r_vsync.value.n);
        if (r_colordepth.value.n == 16) {
                SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
                SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
                SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
        } else {
                SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
                SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
                SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
                if (r_colordepth.value.n != 24)
                        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        }

        /* NVidia drivers respect this environment variable for vsync */
        SDL_putenv(r_vsync.value.n ? "__GL_SYNC_TO_VBLANK=1" :
                                     "__GL_SYNC_TO_VBLANK=0");

        /* Set the video mode */
        flags = SDL_OPENGL | SDL_GL_DOUBLEBUFFER | SDL_HWPALETTE |
                SDL_HWSURFACE;
        if (!r_windowed.value.n)
                flags |= SDL_FULLSCREEN;
        screen = SDL_SetVideoMode(r_width.value.n, r_height.value.n, 0, flags);
        if (!screen) {
                C_warning("Failed to set video mode: %s", SDL_GetError());
                return FALSE;
        }

        /* Set screen view */
        glViewport(0, 0, r_width.value.n, r_height.value.n);

        /* The range of z values here is arbitrary and shoud be
           modified to fit ranges that we typically have. */
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(90.0, (float)r_width.value.n / r_height.value.n,
                       1.0, 10000.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        /* Backface culling. Only rasterize polygons that are facing you.
           Blender seems to export its polygons in counter-clockwise order. */
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);

        /* Use a white default material */
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        glColor3f(1.0, 1.0, 1.0);

        /* Clear to black */
        glClearColor(0.0, 0.0, 0.0, 1.0);

        /* Depth testing */
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        /* Textures */
        glEnable(GL_TEXTURE_2D);

        /* Enable smooth shading */
        glShadeModel(GL_SMOOTH);

        /* Gamma */
        SDL_SetGamma(r_gamma.value.f, r_gamma.value.f, r_gamma.value.f);

        /* All set to start loading models and textures */
        R_load_assets();

        return TRUE;
}

/******************************************************************************\
 Cleanup the OpenGL mess.
\******************************************************************************/
void R_render_cleanup(void)
{
        R_free_assets();
}

/******************************************************************************\
 Render the test model.
\******************************************************************************/
static int render_test_model(void)
{
        float white[] = { 1.0, 1.0, 1.0, 1.0 };
        float left[] = { -1.0, 0.0, 0.0, 0.0 };

        if (!r_test_model.data)
                return FALSE;

        /* Setup a white light to the left */
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, left);
        glLightfv(GL_LIGHT0, GL_COLOR, white);

        /* Render the test mesh */
        r_test_model.origin.z = -8;
        R_model_render(&r_test_model);

        /* Spin the model around a bit */
        r_test_model.angles.x += 0.05 * c_frame_sec;
        r_test_model.angles.y += 0.30 * c_frame_sec;

        return TRUE;
}

/******************************************************************************\
 Render the test mesh.
\******************************************************************************/
static int render_test_mesh(void)
{
        static float x_rot, y_rot;
        float white[] = { 1.0, 1.0, 1.0, 1.0 };
        float left[] = { -1.0, 0.0, 0.0, 0.0 };

        if (!r_test_mesh)
                return FALSE;

        /* Setup a white light to the left */
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0, GL_POSITION, left);
        glLightfv(GL_LIGHT0, GL_COLOR, white);

        /* FIXME: Disabling culling */
        glDisable(GL_CULL_FACE);

        /* Render the test mesh */
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(0.0, 0.0, -20.0);
        glRotatef(x_rot, 1.0, 0.0, 0.0);
        glRotatef(y_rot, 0.0, 1.0, 0.0);
        glTranslatef(0.0, -10.0, 0.0);
        R_static_mesh_render(r_test_mesh, NULL);
        glPopMatrix();

        x_rot += 8 * c_frame_sec;
        y_rot += 20 * c_frame_sec;

        return TRUE;
}

/******************************************************************************\
 Render the test globe.
   FIXME: Broken thanks to new vertex type.
\******************************************************************************/
int render_test_globe()
{
        static float x_rot, y_rot;
        static g_globe_t *globe = NULL;
        r_static_mesh_t fake_mesh;

        if (!r_test_globe.value.n)
                return FALSE;

        if (!globe)
                globe = G_globe_alloc(3);

        /* No lighting or culling */
        glDisable(GL_LIGHTING);
        glDisable(GL_CULL_FACE);

        /* Fake a static mesh */
        /*fake_mesh.nverts = globe->nverts;
        fake_mesh.ninds = globe->ninds;
        fake_mesh.verts = globe->verts;
        fake_mesh.inds = globe->inds;
        fake_mesh.norms = NULL;
        fake_mesh.sts = NULL;*/

        glPushMatrix();

        /* And render */
        glTranslatef(0.0, 0.0, -20.0);
        glRotatef(x_rot, 1.0, 0.0, 0.0);
        glRotatef(y_rot, 0.0, 1.0, 0.0);
        glScalef(10, 10, 10);
        glColor3f(0.0, 0.0, 0.0);
        R_static_mesh_render(&fake_mesh, NULL); /* render for depth */
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glColor3f(1.0, 1.0, 1.0);
        R_static_mesh_render(&fake_mesh, NULL); /* render lines over */

        /* See if neighbors works */
        int i;
        c_vec3_t v = globe->verts[31];

        glPointSize(5.0);
        glBegin(GL_POINTS);

        glColor3f(1.0, 0.0, 0.0);
        glVertex3f(v.x, v.y, v.z);
        glColor3f(0.0, 1.0, 1.0);
        for (i = 0; i < globe->neighbors_lists[31].count; i++) {
                v = globe->verts[globe->neighbors_lists[31].indices[i]];
                glVertex3f(v.x, v.y, v.z);
        }
        glEnd();

        glPopMatrix();

        /* Reset polygon mode */
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        x_rot += 8 * c_frame_sec;
        y_rot += 20 * c_frame_sec;

        return TRUE;
}

/****************************************************************************** \
 Renders the scene.
\******************************************************************************/
void R_render(void)
{
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /* Render testing.
             FIXME: Test globe */
        if (render_test_model() || render_test_mesh());

        SDL_GL_SwapBuffers();
}

