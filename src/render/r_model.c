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

/* Non-animated mesh */
typedef struct r_mesh {
        r_vertex3_t *verts;
        GLuint verts_vbo, indices_vbo;
        unsigned short *indices;
        int verts_len, indices_len;
} mesh_t;

/* Model animation type */
typedef struct model_anim {
        int from, to, delay;
        char name[64], end_anim[64];
} model_anim_t;

/* Model object type */
typedef struct model_object {
        r_texture_t *texture;
        char name[64];
} model_object_t;

/* Animated, textured, multi-mesh model. The matrix contains enough room to
   store every object's static mesh for every frame, it is indexed by frame
   then by object. */
typedef struct r_model_data {
        c_ref_t ref;
        mesh_t *matrix;
        model_anim_t *anims;
        model_object_t *objects;
        int anims_len, objects_len, frames;
} model_data_t;

/* Linked list of loaded model data */
static c_ref_t *data_root;

/******************************************************************************\
 Find a vertex in a static mesh vertex array. Returns the length of the array
 if a matching vertex is not found.
\******************************************************************************/
static unsigned short mesh_find_vert(const mesh_t *mesh,
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
static void mesh_render(mesh_t *mesh, r_texture_t *texture)
{
        R_texture_select(texture);
        C_count_add(&r_count_faces, mesh->indices_len / 3);

        /* Use the Vertex Buffer Object if available */
        if (mesh->verts_vbo && mesh->indices_vbo) {
                r_ext.glBindBuffer(GL_ARRAY_BUFFER, mesh->verts_vbo);
                r_ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indices_vbo);
                glInterleavedArrays(R_VERTEX3_FORMAT, 0, NULL);
                glDrawElements(GL_TRIANGLES, mesh->indices_len,
                               GL_UNSIGNED_SHORT, NULL);
                r_ext.glBindBuffer(GL_ARRAY_BUFFER, 0);
                r_ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        }

        /* Rendering by uploading the data every time is slower */
        else {
                glInterleavedArrays(R_VERTEX3_FORMAT, 0, mesh->verts);
                glDrawElements(GL_TRIANGLES, mesh->indices_len,
                               GL_UNSIGNED_SHORT, mesh->indices);
        }

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_NORMAL_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        R_check_errors();

        /* Render the mesh normals for testing */
        R_render_normals(mesh->verts_len, &mesh->verts[0].co,
                         &mesh->verts[0].no, sizeof (*mesh->verts));
}

/******************************************************************************\
 Release the resources for a mesh.
\******************************************************************************/
static void mesh_cleanup(mesh_t *mesh)
{
        if (!mesh)
                return;
        C_free(mesh->verts);
        C_free(mesh->indices);
        if (mesh->verts_vbo)
                r_ext.glDeleteBuffers(1, &mesh->verts_vbo);
        if (mesh->indices_vbo)
                r_ext.glDeleteBuffers(1, &mesh->indices_vbo);
}

/******************************************************************************\
 Finish parsing an object and creates the mesh object. If Vertex Buffer Objects
 are supported, a new buffer is allocated and the vertex data is uploaded.
\******************************************************************************/
static int finish_object(model_data_t *data, int frame, int object,
                         c_array_t *verts, c_array_t *indices)
{
        mesh_t *mesh;
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

        /* If Vertex Buffer Objects are available, upload the data now */
        if (r_ext.vertex_buffers) {

                /* First upload the vertex data */
                r_ext.glGenBuffers(1, &mesh->verts_vbo);
                r_ext.glBindBuffer(GL_ARRAY_BUFFER, mesh->verts_vbo);
                r_ext.glBufferData(GL_ARRAY_BUFFER,
                                   mesh->verts_len * sizeof (*mesh->verts),
                                   mesh->verts, GL_STATIC_DRAW);
                r_ext.glBindBuffer(GL_ARRAY_BUFFER, 0);

                /* Now upload the indices */
                r_ext.glGenBuffers(1, &mesh->indices_vbo);
                r_ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->indices_vbo);
                r_ext.glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                   mesh->indices_len * sizeof (*mesh->indices),
                                   mesh->indices, GL_STATIC_DRAW);
                r_ext.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

                R_check_errors();
        } else {
                mesh->verts_vbo = 0;
                mesh->indices_vbo = 0;
        }

        return TRUE;
}

/******************************************************************************\
 Free resources associated with the model private structure.
\******************************************************************************/
static void model_data_cleanup(model_data_t *data)
{
        int i;

        if (!data)
                return;
        if (data->matrix) {
                for (i = 0; i < data->objects_len * data->frames; i++)
                        mesh_cleanup(data->matrix + i);
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

 FIXME: Does not account for material properties.
\******************************************************************************/
static model_data_t *model_data_load(const char *filename)
{
        c_token_file_t token_file;
        c_array_t anims, objects, verts, indices;
        model_data_t *data;
        const char *token;
        int found, quoted, object, frame, verts_parsed;

        if (!filename || !filename[0])
                return NULL;
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
                C_array_init(&anims, model_anim_t, 8);
                for (;;) {
                        model_anim_t anim;

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
        C_array_init(&objects, model_object_t, 8);
        for (;;) {
                model_object_t obj;

                token = C_token_file_read_full(&token_file, &quoted);
                if (strcmp(token, "object"))
                        break;
                token = C_token_file_read(&token_file);
                C_strncpy(obj.name, token, sizeof (obj.name));
                token = C_token_file_read(&token_file);
                obj.texture = R_texture_load(token, TRUE);
                C_array_append(&objects, &obj);
        }
        data->objects_len = objects.len;
        data->objects = C_array_steal(&objects);

        /* Load frames into matrix */
        data->matrix = C_calloc(data->frames * data->objects_len *
                                sizeof (mesh_t));
        for (frame = -1, object = -1, verts_parsed = 0; token[0] || quoted;
             token = C_token_file_read_full(&token_file, &quoted)) {

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
                        C_array_init(&verts, r_vertex3_t, 512);
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
                        r_vertex3_t vert;

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
        C_debug("Loaded '%s' (%d frm, %d obj, %d anim)",
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
        model->normal = C_vec3(0.f, 1.f, 0.f);
        model->forward = C_vec3(0.f, 0.f, 1.f);

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
                        mesh_cleanup(model->lerp_meshes + i);
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
static void interpolate_mesh(mesh_t *dest, float lerp,
                             const mesh_t *from,
                             const mesh_t *to)
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
                r_vertex3_t *to_vert, *from_vert, vert;
                unsigned short index;

                to_vert = to->verts + to->indices[j];
                from_vert = from->verts + from->indices[j];
                vert.co = C_vec3_lerp(from_vert->co, lerp, to_vert->co);
                vert.no = C_vec3_lerp(from_vert->no, lerp, to_vert->no);
                vert.uv = C_vec2_lerp(from_vert->uv, lerp, to_vert->uv);
                index = mesh_find_vert(dest, &vert);
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
        model_anim_t *anim;
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
        if (model->frame == model->last_frame)
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

 This URL is helpful in demystifying the matrix operations:
 http://www.gamedev.net/reference/articles/article695.asp
\******************************************************************************/
void R_model_render(r_model_t *model)
{
        c_vec3_t side;
        mesh_t *meshes;
        GLfloat m[16];
        int i;

        if (!model || !model->data)
                return;
        R_push_mode(R_MODE_3D);

        /* Calculate the right-pointing vector. The forward and normal
           vectors had better be correct and normalized! */
        side = C_vec3_cross(model->normal, model->forward);

        /* X */
        m[0] = side.x * model->scale;
        m[4] = model->normal.x * model->scale;
        m[8] = model->forward.x * model->scale;
        m[12] = model->origin.x;

        /* Y */
        m[1] = side.y * model->scale;
        m[5] = model->normal.y * model->scale;
        m[9] = model->forward.y * model->scale;
        m[13] = model->origin.y;

        /* Z */
        m[2] = side.z * model->scale;
        m[6] = model->normal.z * model->scale;
        m[10] = model->forward.z * model->scale;
        m[14] = model->origin.z;

        /* W */
        m[3] = 0.f;
        m[7] = 0.f;
        m[11] = 0.f;
        m[15] = 1.f;

        glMultMatrixf(m);
        R_check_errors();

        /* Animate meshes. Interpolating here between frames is slow, it is best
           if models contain enough keyframes to maintain desired framerate. */
        if (model->time_left >= 0)
                update_animation(model);
        if (model->use_lerp_meshes)
                meshes = model->lerp_meshes;
        else
                meshes = model->data->matrix +
                         model->data->objects_len * model->last_frame;

        /* We want to multisample models */
        if (r_multisample.value.n)
                glEnable(GL_MULTISAMPLE);

        /* Each object in the model needs a rendering call */
        for (i = 0; i < model->data->objects_len; i++)
                mesh_render(meshes + i, model->data->objects[i].texture);

        glDisable(GL_MULTISAMPLE);
        R_pop_mode();
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

