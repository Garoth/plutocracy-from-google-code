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

static c_ref_t *data_root;

/******************************************************************************\
 Finish parsing an object.
\******************************************************************************/
static int finish_object(r_model_data_t *data, int frame, int object,
                          c_array_t *verts, c_array_t *indices)
{
        r_static_mesh_t *mesh;
        int index, index_last;

        if (object < 0 || frame < 0)
                return TRUE;
        if (frame >= data->frames)
                C_error("Invalid frame %d", frame);
        if (object >= data->objects_len)
                C_error("Invalid obejct %d", object);
        mesh = data->matrix + frame * data->objects_len + object;
        mesh->verts_len = verts->len;
        mesh->verts = C_array_steal(verts);
        mesh->indices_len = indices->len;
        mesh->indices = C_array_steal(indices);
        index_last = (frame - 1) * data->objects_len + object;
        index = frame * data->objects_len + object;
        if (frame > 0 && data->matrix[index_last].indices_len !=
                         data->matrix[index].indices_len) {
                C_warning("PLUM file '%s' object '%s' faces mismatch at "
                          "frame %d", data->ref.name,
                          data->objects[object].name, frame);
                return FALSE;
        }
        return TRUE;
}

/******************************************************************************\
 Free resources associated with the model private structure.
\******************************************************************************/
static void model_data_cleanup(r_model_data_t *data)
{
        int i;

        if (!data)
                return;
        if (data->matrix) {
                for (i = 0; i < data->objects_len * data->frames; i++)
                        R_static_mesh_cleanup(data->matrix + i);
                C_free(data->matrix);
        }
        for (i = 0; i < data->objects_len; i++)
                R_texture_free(data->objects[i].texture);
        C_free(data->objects);
        C_free(data->anims);
}

/******************************************************************************\
 Allocate memory for and load model data and its textures. Data is cached so
 calling this function again will return and reference the cached data.
\******************************************************************************/
static r_model_data_t *model_data_load(const char *filename)
{
        c_token_file_t token_file;
        c_array_t anims, objects, verts, indices;
        r_model_data_t *data;
        const char *token;
        int found, quoted, object, frame;

        data = C_ref_alloc(sizeof (*data), &data_root,
                           (c_ref_cleanup_f)model_data_cleanup,
                           filename, &found);
        if (found)
                return data;

        /* Start parsing the file */
        C_zero(&verts);
        C_zero(&indices);
        if (!C_token_file_init(&token_file, filename)) {
                C_warning("Failed to open model '%s'", filename);
                goto error;
        }

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
                        anim.from = atoi(C_token_file_read(&token_file)) - 1;
                        anim.to = atoi(C_token_file_read(&token_file)) - 1;
                        if (anim.from < 0 || anim.to < 0) {
                                C_warning("PLUM file '%s' contains invalid "
                                          "animation frame indices", filename);
                                goto error;
                        }
                        if (anim.from >= data->frames)
                                data->frames = anim.from + 1;
                        if (anim.to >= data->frames)
                                data->frames = anim.to + 1;
                        anim.delay = 1000 /
                                     atoi(C_token_file_read(&token_file));
                        if (anim.delay < 1)
                                anim.delay = 1;
                        token = C_token_file_read(&token_file);
                        C_strncpy(anim.end_anim, token, sizeof (anim.end_anim));
                        C_array_append(&anims, &anim);
                }
                data->anims_len = anims.len;
                data->anims = C_array_steal(&anims);
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
        data->objects_len = objects.len;
        data->objects = C_array_steal(&objects);

        /* Load frames into matrix */
        data->matrix = C_calloc(data->frames * data->objects_len *
                                sizeof (r_static_mesh_t));
        for (frame = -1, object = -1; token[0] || quoted;
             token = C_token_file_read_full(&token_file, &quoted)) {
                int verts_parsed;

                /* Each frame starts with 'frame #' where # is the frame
                   index numbered from 1 */
                if (!strcmp(token, "frame")) {
                        if (!finish_object(data, frame, object,
                                           &verts, &indices))
                                goto error;
                        object = -1;
                        if (++frame >= data->frames)
                                break;
                        token = C_token_file_read(&token_file);
                        if (atoi(token) != frame + 1) {
                                C_warning("PLUM file '%s' missing frames",
                                          filename);
                                goto error;
                        }
                        continue;
                }
                if (frame < 0) {
                        C_warning("PLUM file '%s' has '%s' instead of 'frame'",
                                  filename, token);
                        goto error;
                }

                /* Objects start with an 'o' and are listed in order */
                if (!strcmp(token, "o")) {
                        if (!finish_object(data, frame, object++,
                                           &verts, &indices))
                                goto error;
                        C_array_init(&verts, r_vertex_t, 512);
                        C_array_init(&indices, unsigned short, 512);
                        verts_parsed = 0;
                        if (object > data->objects_len) {
                                C_warning("PLUM file '%s' frame %d has too"
                                          "many objects", filename, frame);
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
        if (!finish_object(data, frame, object, &verts, &indices))
                goto error;
        if (frame < data->frames - 1) {
                C_warning("PLUM file '%s' lacks %d frame(s)",
                          filename, data->frames - frame + 1);
                goto error;
        }

        C_token_file_cleanup(&token_file);
        C_debug("Loaded '%s' (%d frames, %d objects, %d animations)",
                filename, data->frames, data->objects_len, data->anims_len);
        return data;

error:  C_token_file_cleanup(&token_file);
        C_array_cleanup(&verts);
        C_array_cleanup(&indices);
        C_ref_down(&data->ref);
        return NULL;
}

/******************************************************************************\
 Initialize a model instance. Model data is loaded if it is not already in
 memory. Returns FALSE and invalidates the model instance if the model data
 failed to load.
\******************************************************************************/
int R_model_init(r_model_t *model, const char *filename)
{
        if (!model)
                return FALSE;
        C_zero(model);
        model->data = model_data_load(filename);
        model->scale = 1.f;
        model->time_left = -1;

        /* Start playing the first animation */
        if (model->data && model->data->anims_len)
                R_model_play(model, model->data->anims[0].name);

        /* Allocate memory for interpolated object meshes */
        if (model->data && model->data->frames)
                model->lerp_meshes = C_calloc(model->data->objects_len *
                                              sizeof (*model->lerp_meshes));

        return model->data != NULL;
}

/******************************************************************************\
 Decrease reference counts to resources used by a model and free those
 resources if they have no references left.
\******************************************************************************/
void R_model_cleanup(r_model_t *model)
{
        int i;

        if (!model)
                return;
        if (model->lerp_meshes) {
                for (i = 0; i < model->data->objects_len; i++)
                        R_static_mesh_cleanup(model->lerp_meshes + i);
                C_free(model->lerp_meshes);
        }
        C_ref_down(&model->data->ref);
        C_zero(model);
}

/******************************************************************************\
 Interpolate between two frames. If the target mesh has vertex and index
 arrays, it is assumed that there is sufficient space allocated in those
 arrays to hold the new interpolated arrays otherwise these arrays are
 allocated with enough room to hold the largest possible interpolated frame.
 Textures are not interpolated.
\******************************************************************************/
static void interpolate_mesh(r_static_mesh_t *dest, float lerp,
                             const r_static_mesh_t *from,
                             const r_static_mesh_t *to)
{
        int j;

        /* Allocate memory for index and vertex arrays */
        if (!dest->indices)
                dest->indices = C_malloc(to->indices_len *
                                         sizeof (*to->indices));
        if (!dest->verts)
                dest->verts = C_malloc(to->indices_len * sizeof (*to->verts));

        /* Interpolate each vertex */
        dest->verts_len = 0;
        dest->indices_len = to->indices_len;
        for (j = 0; j < to->indices_len; j++) {
                r_vertex_t *to_vert, *from_vert, vert;
                unsigned short index;

                to_vert = to->verts + to->indices[j];
                from_vert = from->verts + from->indices[j];
                vert.co = C_vec3_lerp(from_vert->co, lerp, to_vert->co);
                vert.no = C_vec3_lerp(from_vert->no, lerp, to_vert->no);
                vert.uv = C_vec2_lerp(from_vert->uv, lerp, to_vert->uv);
                index = R_static_mesh_find_vert(dest, &vert);
                if (index >= dest->verts_len)
                        dest->verts[dest->verts_len++] = vert;
                dest->indices[j] = index;
        }
}

/******************************************************************************\
 Updates animation progress and frame. Interpolates between key-frame meshes
 when necessary to smooth the animation.
\******************************************************************************/
static void update_animation(r_model_t *model)
{
        r_model_anim_t *anim;
        float lerp;
        int i;

        anim = model->data->anims + model->anim;
        model->time_left -= c_frame_msec;

        /* Advance a frame forward or backward */
        if (model->time_left <= 0) {
                model->last_frame = model->frame;
                if (anim->to > anim->from) {
                        model->frame++;
                        if (model->frame > anim->to)
                                R_model_play(model, anim->end_anim);
                } else {
                        model->frame--;
                        if (model->frame < anim->to)
                                R_model_play(model, anim->end_anim);
                }
                model->time_left = anim->delay;
                model->last_frame_time = c_time_msec;
                model->use_lerp_meshes = FALSE;
                return;
        }

        /* Generate interpolated meshes if we need a new frame now */
        if (c_time_msec - model->last_frame_time < 1000 / R_MODEL_ANIM_FPS)
                return;
        model->last_frame_time = c_time_msec;
        model->use_lerp_meshes = TRUE;
        if (anim->delay < 1)
                C_error("Invalid animation structure");
        lerp = 1.f - (float)model->time_left / anim->delay;
        for (i = 0; i < model->data->objects_len; i++)
                interpolate_mesh(model->lerp_meshes + i, lerp,
                                 model->data->matrix + i +
                                 model->last_frame * model->data->objects_len,
                                 model->data->matrix + i +
                                 model->frame * model->data->objects_len);
}

/******************************************************************************\
 Render and advance the animation of a model. Applies the model's translation,
 rotation, and scale.
\******************************************************************************/
void R_model_render(r_model_t *model)
{
        r_static_mesh_t *meshes;
        int i;

        if (!model || !model->data)
                return;
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glScalef(model->scale, model->scale, model->scale);
        glTranslatef(model->origin.x, model->origin.y, model->origin.z);
        glRotatef(C_rad_to_deg(model->angles.x), 1.0, 0.0, 0.0);
        glRotatef(C_rad_to_deg(model->angles.y), 0.0, 1.0, 0.0);
        glRotatef(C_rad_to_deg(model->angles.z), 0.0, 0.0, 1.0);
        if (model->time_left >= 0)
                update_animation(model);
        if (model->use_lerp_meshes)
                meshes = model->lerp_meshes;
        else
                meshes = model->data->matrix +
                         model->data->objects_len * model->last_frame;
        for (i = 0; i < model->data->objects_len; i++)
                R_static_mesh_render(meshes + i,
                                     model->data->objects[i].texture);
        glPopMatrix();
}

/******************************************************************************\
 Stop the model from playing animations.
\******************************************************************************/
static void model_stop(r_model_t *model)
{
        model->anim = 0;
        model->frame = 0;
        model->last_frame = 0;
        model->time_left = -1;
        model->use_lerp_meshes = FALSE;
}

/******************************************************************************\
 Set a model animation to play.
\******************************************************************************/
void R_model_play(r_model_t *model, const char *name)
{
        int i;

        if (!model || !model->data)
                return;
        if (!name || !name[0]) {
                model_stop(model);
                return;
        }
        for (i = 0; i < model->data->anims_len; i++)
                if (!strcasecmp(model->data->anims[i].name, name)) {
                        model->anim = i;
                        model->frame = model->data->anims[i].from;
                        model->time_left = model->data->anims[i].delay;
                        model->last_frame_time = c_time_msec;
                        model->use_lerp_meshes = FALSE;
                        return;
                }
        model_stop(model);
        C_warning("Model '%s' lacks anim '%s'", model->data->ref.name, name);
}

