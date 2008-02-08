/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* This file implements the PLUM (Plutocracy Model) model loading and rendering
   functions. */

#include "r_common.h"

/******************************************************************************\
 Finish parsing an object.
\******************************************************************************/
static void finish_object(r_model_t *model, int object,
                          c_array_t *verts, c_array_t *indices)
{
        r_static_mesh_t *mesh;

        if (object < 0 )
                return;
        mesh = model->matrix + model->frame * model->frames + object;
        R_texture_ref(mesh->texture = model->objects[object].texture);
        mesh->verts_len = verts->len;
        mesh->verts = C_array_steal(verts);
        mesh->indices_len = indices->len;
        mesh->indices = C_array_steal(indices);
}

/******************************************************************************\
 Allocate memory for and load a model and its textures.
\******************************************************************************/
r_model_t *R_model_load(const char *filename)
{
        c_token_file_t token_file;
        c_array_t anims, objects, verts, indices;
        r_model_t *model;
        const char *token;
        int quoted, object;

        if (!C_token_file_init(&token_file, filename)) {
                C_warning("Failed to open model '%s'", filename);
                return NULL;
        }
        memset(&verts, 0, sizeof (verts));
        memset(&indices, 0, sizeof (indices));
        model = C_calloc(sizeof (*model));
        C_strncpy(model->filename, filename, sizeof (model->filename));

        /* Load 'anims:' block */
        token = C_token_file_read(&token_file);
        if (!strcmp(token, "anims:")) {
                C_array_init(&anims, r_model_anim_t, 8);
                for (;;) {
                        r_model_anim_t anim;

                        /* Syntax: [name] [from] [to] [fps] [end-anim] */
                        token = C_token_file_read_full(&token_file, &quoted);
                        if (!quoted && !strcmp(token, "end"))
                                break;
                        C_strncpy(anim.name, token, sizeof (anim.name));
                        anim.from = atoi(C_token_file_read(&token_file));
                        anim.to = atoi(C_token_file_read(&token_file));
                        if (anim.to > model->frames)
                                model->frames = anim.to;
                        anim.delay = 1000 /
                                     atoi(C_token_file_read(&token_file));
                        token = C_token_file_read(&token_file);
                        C_strncpy(anim.end_anim, token, sizeof (anim.end_anim));
                        C_array_append(&anims, &anim);
                }
                model->anims_len = anims.len;
                model->anims = C_array_steal(&anims);
        } else {
                C_warning("PLUM file '%s' lacks anims block", filename);
                goto error;
        }

        /* Define objects */
        C_array_init(&objects, r_model_object_t, 8);
        for (;;) {
                r_model_object_t obj;

                token = C_token_file_read_full(&token_file, &quoted);
                if (strcmp(token, "object"))
                        break;
                token = C_token_file_read(&token_file);
                C_strncpy(obj.name, token, sizeof (obj.name));
                token = C_token_file_read(&token_file);
                obj.texture = R_texture_load(token);
                C_array_append(&objects, &obj);
        }
        model->objects_len = objects.len;
        model->objects = C_array_steal(&objects);

        /* Load frames into matrix */
        model->matrix = C_calloc(model->frames * model->objects_len *
                                 sizeof (r_static_mesh_t));
        C_array_init(&verts, r_vertex_t, 512);
        for (model->frame = -1, object = -1; token[0] || quoted;
             token = C_token_file_read_full(&token_file, &quoted)) {
                int verts_parsed;

                /* Each frame starts with 'frame #' where # is the frame
                   index numbered from 1 */
                if (!strcmp(token, "frame")) {
                        finish_object(model, object, &verts, &indices);
                        object = -1;
                        if (++model->frame > model->frames)
                                break;
                        token = C_token_file_read(&token_file);
                        if (atoi(token) != model->frame + 1) {
                                C_warning("PLUM file '%s' missing frames",
                                          filename);
                                goto error;
                        }
                        continue;
                }
                if (model->frame < 0) {
                        C_warning("PLUM file '%s' has '%s' instead of 'frame'",
                                  filename, token);
                        goto error;
                }

                /* Objects start with an 'o' and are listed in order */
                if (!strcmp(token, "o")) {
                        finish_object(model, object++, &verts, &indices);
                        C_array_init(&verts, r_vertex_t, 512);
                        C_array_init(&indices, unsigned short, 512);
                        verts_parsed = 0;
                        if (object > model->objects_len) {
                                C_warning("PLUM file '%s' frame %d has too"
                                          "many objects", filename,
                                          model->frame);
                                goto error;
                        }
                        continue;
                }
                if (object < 0) {
                        C_warning("PLUM file '%s' has '%s' instead of 'o'",
                                  filename, token);
                        goto error;
                }

                /* A 'v' means we need to add a new vertex */
                if (!strcmp(token, "v")) {
                        r_vertex_t vert;

                        /* Syntax: v [coords x y z] [normal x y z] [uv x y] */
                        verts_parsed++;
                        vert.co.x = (float)atof(C_token_file_read(&token_file));
                        vert.co.y = (float)atof(C_token_file_read(&token_file));
                        vert.co.z = (float)atof(C_token_file_read(&token_file));
                        vert.no.x = (float)atof(C_token_file_read(&token_file));
                        vert.no.y = (float)atof(C_token_file_read(&token_file));
                        vert.no.z = (float)atof(C_token_file_read(&token_file));
                        vert.uv.x = (float)atof(C_token_file_read(&token_file));
                        vert.uv.y = (float)atof(C_token_file_read(&token_file));
                        C_array_append(&verts, &vert);

                        /* Parsing three new vertices automatically adds a
                           new face containing them */
                        if (verts_parsed >= 3) {
                                int i;
                                unsigned short index;

                                for (i = 0; i < 3; i++) {
                                        index = (unsigned short)
                                                (verts.len - 3 + i);
                                        C_array_append(&indices, &index);
                                }
                                verts_parsed = 0;
                        }
                        continue;
                }

                /* An 'i' means we can construct a new face using three
                   existing vertices */
                if (!strcmp(token, "i")) {
                        int i;
                        unsigned short index;

                        for (i = 0; i < 3; i++) {
                                token = C_token_file_read(&token_file);
                                index = (unsigned short)atoi(token);
                                if (index > verts.len) {
                                        C_warning("PLUM file '%s' contains "
                                                  "invalid index", filename);
                                        goto error;
                                }
                                C_array_append(&indices, &index);
                        }
                        verts_parsed = 0;
                        continue;
                }

                C_warning("PLUM file '%s' contains unrecognized token '%s'",
                          filename, token);
                goto error;
        }
        finish_object(model, object, &verts, &indices);

        C_token_file_cleanup(&token_file);
        C_debug("Loaded '%s' (%d frames, %d objects, %d animations)",
                filename, model->frames, model->objects_len, model->anims_len);
        return model;

error:  C_token_file_cleanup(&token_file);
        C_array_cleanup(&verts);
        C_array_cleanup(&indices);
        R_model_free(model);
        return NULL;
}

/******************************************************************************\
 Free memory used by a model and decrease the reference count of its textures.
\******************************************************************************/
void R_model_free(r_model_t *model)
{
        int i;

        if (!model)
                return;
        if (model->matrix) {
                for (i = 0; i < model->objects_len * model->frames; i++)
                        R_static_mesh_cleanup(model->matrix + i);
                C_free(model->matrix);
        }
        for (i = 0; i < model->objects_len; i++)
                R_texture_free(model->objects[i].texture);
        C_free(model->objects);
        C_free(model->anims);
        C_debug("Free'd '%s'", model->filename);
        C_free(model);
}

/******************************************************************************\
 Render and advance the animation of a model.
\******************************************************************************/
void R_model_render(r_model_t *model)
{
}

/******************************************************************************\
 Set a model animation to play.
\******************************************************************************/
void R_model_play(r_model_t *model, const char *name)
{
}

