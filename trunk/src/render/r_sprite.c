/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Implements 2D sprites and text rendering with the SDL TTF library. This
   should be the only file that calls functions from the library. */

#include "r_common.h"

static struct {
        TTF_Font *ttf_font;
        float scale;
} fonts[R_FONTS];

/******************************************************************************\
 Initialize a sprite structure.
\******************************************************************************/
void R_sprite_init(r_sprite_t *sprite, const char *filename)
{
        if (!sprite)
                return;
        C_zero(sprite);
        sprite->red = 1.f;
        sprite->green = 1.f;
        sprite->blue = 1.f;
        sprite->alpha = 1.f;
        if (!filename || !filename[0])
                return;
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
   FIXME: There is a slight overlap between the antialiased edge line and the
          polygon which should probably be fixed if possible.
\******************************************************************************/
void R_sprite_render(const r_sprite_t *sprite)
{
        r_vertex2_t verts[4];
        unsigned short indices[] = { 0, 1, 2, 3, 0 };
        float dw, dh;

        if (!sprite || !sprite->texture)
                return;
        R_set_mode(R_MODE_2D);
        R_texture_select(sprite->texture);

        /* Modulate color */
        glColor4f(sprite->red, sprite->green, sprite->blue, sprite->alpha);
        if (sprite->alpha < 1.f)
                glEnable(GL_BLEND);

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
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(sprite->origin.x, sprite->origin.y, 0.f);
        glRotatef(C_rad_to_deg(sprite->angle), 0.0, 0.0, 1.0);
        glInterleavedArrays(R_VERTEX2_FORMAT, 0, verts);
        glDrawElements(GL_QUADS, 4, GL_UNSIGNED_SHORT, indices);

        /* Draw the edge lines to anti-alias non-alpha quads */
        if (!sprite->texture->alpha && sprite->angle != 0.f &&
            sprite->alpha == 1.f) {
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glEnable(GL_BLEND);
                glDrawElements(GL_LINE_STRIP, 5, GL_UNSIGNED_SHORT, indices);
        }

        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
        glPopMatrix();
        R_check_errors();
}

/******************************************************************************\
 Loads the font, properly scaled and generates an error message if something
 goes wrong.
\******************************************************************************/
static void load_font(r_font_t font, const char *path, int size)
{
        int points;

        points = ceilf(size * r_pixel_scale.value.f);
        fonts[font].scale = (float)size / points;
        if (points < R_FONT_SIZE_MIN)
                points = R_FONT_SIZE_MIN;
        fonts[font].ttf_font = TTF_OpenFont(path, points);
        if (!fonts[font].ttf_font)
                C_error("Failed to load font '%s' (%d)", path, size);
}

/******************************************************************************\
 Initialize the SDL TTF library and generate font structures.
\******************************************************************************/
void R_init_fonts(void)
{
        SDL_version compiled;
        const SDL_version *linked;
        int i;

        TTF_VERSION(&compiled);
        C_debug("Compiled with SDL_ttf %d.%d.%d",
                compiled.major, compiled.minor, compiled.patch);
        linked = TTF_Linked_Version();
        if (!linked)
                C_error("Failed to get SDL_ttf linked version");
        C_debug("Linked with SDL_ttf %d.%d.%d",
                linked->major, linked->minor, linked->patch);
        if (TTF_Init() == -1)
                C_error("Failed to initialize SDL TTF library: %s",
                        TTF_GetError());

        /* Load font files */
        load_font(R_FONT_CONSOLE, "gui/fonts/VeraMoBd.ttf", 12);
        for (i = 0; i < R_FONTS; i++)
                if (!fonts[i].ttf_font)
                        C_error("Forgot to load font %d", i);
}

/******************************************************************************\
 Cleanup the SDL TTF library.
\******************************************************************************/
void R_cleanup_fonts(void)
{
        int i;

        for (i = 0; i < R_FONTS; i++)
                TTF_CloseFont(fonts[i].ttf_font);
        TTF_Quit();
}

/******************************************************************************\
 If the text or font has changed, this function will clean up old resources
 and generate a new SDL surface and texture. This function will setup the
 sprite member of the text structure for rendering. Note that sprite size will
 be reset if it is regenerated.
   TODO: To implement text wrapping we need to make a new surface and manually
         paste the text on there line by line. This will let us use our
         texture format though. See:
         http://www.gamedev.net/reference/articles/article1960.asp
\******************************************************************************/
void R_text_set_text(r_text_t *text, const char *string, r_font_t font,
                     float wrap)
{
        SDL_Color white = { 255, 255, 255, 255 };
        r_texture_t *tex;

        if (font < 0 || font >= R_FONTS)
                C_error("Invalid font index %d", font);
        if (!text || (font == text->font && !strcmp(string, text->string)))
                return;
        C_strncpy_buf(text->string, string);
        R_sprite_cleanup(&text->sprite);

        /* SDL_ttf will generate a 32-bit RGBA surface with the text printed
           on it in white (we can modulate the color later) */
        tex = R_texture_alloc();
        tex->surface = TTF_RenderUTF8_Blended(fonts[font].ttf_font,
                                              string, white);
        tex->alpha = TRUE;
        tex->gl_format = GL_RGBA;
        tex->gl_type = GL_UNSIGNED_INT_8_8_8_8;
        R_texture_upload(tex);

        R_sprite_init(&text->sprite, NULL);
        text->sprite.texture = tex;
        text->sprite.size.x = tex->surface->w * fonts[font].scale;
        text->sprite.size.y = tex->surface->h * fonts[font].scale;
        text->font = font;
}

