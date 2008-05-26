/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Implements model, mesh, and texture loading and management. Textures are
   stored in a linked list and should only be accessed during loading time so
   efficiency here is not important. */

#include "r_common.h"

/* Font configuration variables */
extern c_var_t r_font_console, r_font_console_pt, r_font_gui, r_font_gui_pt,
               r_font_title, r_font_title_pt;

/* Terrain tile sheet */
r_texture_t *r_terrain_tex;

/* Font asset array */
static struct {
        TTF_Font *ttf_font;
        int line_skip, width, height;
} fonts[R_FONTS];

static c_ref_t *root, *root_alloc;
static SDL_PixelFormat sdl_format;
static int ttf_inited;

/******************************************************************************\
 Gets a pixel from an SDL surface. Lock the surface before calling.
\******************************************************************************/
c_color_t R_surface_get(const SDL_Surface *surf, int x, int y)
{
        Uint8 *p, r, g, b, a;
        Uint32 pixel;
        int bpp;

        bpp = surf->format->BytesPerPixel;
        p = (Uint8 *)surf->pixels + y * surf->pitch + x * bpp;
        switch(bpp) {
        case 1: pixel = *p;
                break;
        case 2: pixel = *(Uint16 *)p;
                break;
        case 3: pixel = SDL_BYTEORDER == SDL_BIG_ENDIAN ?
                        p[0] << 16 | p[1] << 8 | p[2] :
                        p[0] | p[1] << 8 | p[2] << 16;
                break;
        case 4: pixel = *(Uint32 *)p;
                break;
        default:
                C_error("Invalid surface format");
        }
        SDL_GetRGBA(pixel, surf->format, &r, &g, &b, &a);
        return C_color_rgba(r, g, b, a);
}

/******************************************************************************\
 Sets the color of a pixel on an SDL surface. Lock the surface before
 calling.
\******************************************************************************/
void R_surface_put(SDL_Surface *surf, int x, int y, c_color_t color)
{
        Uint8 *p;
        Uint32 pixel;
        int bpp;

        bpp = surf->format->BytesPerPixel;
        p = (Uint8 *)surf->pixels + y * surf->pitch + x * bpp;
        pixel = SDL_MapRGBA(surf->format, (Uint8)(255.f * color.r),
                            (Uint8)(255.f * color.g), (Uint8)(255.f * color.b),
                            (Uint8)(255.f * color.a));
        switch(bpp) {
        case 1: *p = pixel;
                break;
        case 2: *(Uint16 *)p = pixel;
                break;
        case 3: if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                        p[0] = (pixel >> 16) & 0xff;
                        p[1] = (pixel >> 8) & 0xff;
                        p[2] = pixel & 0xff;
                } else {
                        p[0] = pixel & 0xff;
                        p[1] = (pixel >> 8) & 0xff;
                        p[2] = (pixel >> 16) & 0xff;
                }
                break;
        case 4: *(Uint32 *)p = pixel;
                break;
        default:
                C_error("Invalid surface format");
        }
}

/******************************************************************************\
 Inverts a surface. Surface should not be locked when this is called.
\******************************************************************************/
void R_surface_invert(SDL_Surface *surf, int rgb, int alpha)
{
        c_color_t color;
        int x, y;

        if (SDL_LockSurface(surf) < 0) {
                C_warning("Failed to lock surface");
                return;
        }
        for (y = 0; y < surf->h; y++)
                for (x = 0; x < surf->w; x++) {
                        color = R_surface_get(surf, x, y);
                        if (rgb) {
                                color.r = 1.f - color.r;
                                color.g = 1.f - color.g;
                                color.b = 1.f - color.b;
                        }
                        if (alpha)
                                color.a = 1.f - color.a;
                        R_surface_put(surf, x, y, color);
                }
        SDL_UnlockSurface(surf);
}

/******************************************************************************\
 Overwrite's [dest]'s alpha channel with [src]'s intensity. If [dest] is
 larger than [src], [src] is tiled. Do not call on locked surfaces.
\******************************************************************************/
void R_surface_mask(SDL_Surface *dest, SDL_Surface *src)
{
        int x, y;

        if (SDL_LockSurface(dest) < 0) {
                C_warning("Failed to lock destination surface");
                return;
        }
        if (SDL_LockSurface(src) < 0) {
                C_warning("Failed to lock source surface");
                return;
        }
        for (y = 0; y < dest->h; y++)
                for (x = 0; x < dest->w; x++) {
                        c_color_t color, mask;

                        color = R_surface_get(dest, x, y);
                        mask = R_surface_get(src, x % src->w, y % src->h);
                        color.a = C_color_luma(mask);
                        R_surface_put(dest, x, y, color);
                }
        SDL_UnlockSurface(dest);
        SDL_UnlockSurface(src);
}

/******************************************************************************\
 Vertically flip [surf]'s pixels. Do not call on a locked surface.
\******************************************************************************/
void R_surface_flip_v(SDL_Surface *surf)
{
        int x, y;

        if (SDL_LockSurface(surf) < 0) {
                C_warning("Failed to lock surface");
                return;
        }
        for (y = 0; y < surf->h / 2; y++)
                for (x = 0; x < surf->w; x++) {
                        c_color_t color_top, color_bottom;

                        color_top = R_surface_get(surf, x, y);
                        color_bottom = R_surface_get(surf, x, surf->h - y - 1);
                        R_surface_put(surf, x, y, color_bottom);
                        R_surface_put(surf, x, surf->h - y - 1, color_top);
                }
        SDL_UnlockSurface(surf);
}

/******************************************************************************\
 Frees memory associated with a texture.
\******************************************************************************/
static void texture_cleanup(r_texture_t *pt)
{
        SDL_FreeSurface(pt->surface);
        glDeleteTextures(1, &pt->gl_name);
        R_check_errors();
}

/******************************************************************************\
 Just allocate a texture object and cleared [width] by [height] SDL surface.
 Does not load anything or add the texture to the texture linked list. The
 texture needs to be uploaded after calling this function.
\******************************************************************************/
r_texture_t *R_texture_alloc_full(const char *file, int line, const char *func,
                                  int width, int height, int alpha)
{
        static int count;
        SDL_Rect rect;
        r_texture_t *pt;
        int flags;

        if (width < 1 || height < 1)
                C_error_full(file, line, func,
                             "Invalid texture dimensions: %dx%d",
                             width, height);
        if (!C_is_pow2(width) || !C_is_pow2(height))
                C_warning_full(file, line, func,
                               "Texture dimensions not power-of-two: %dx%d",
                               width, height);
        pt = C_recalloc_full(file, line, func, NULL, sizeof (*pt));
        pt->ref.refs = 1;
        pt->ref.cleanup_func = (c_ref_cleanup_f)texture_cleanup;
        if (root_alloc) {
                pt->ref.next = root_alloc;
                root_alloc->prev = &pt->ref;
        }
        pt->ref.root = &root_alloc;
        root_alloc = &pt->ref;
        if (c_mem_check.value.n)
                C_strncpy_buf(pt->ref.name,
                              C_va("Texture #%d allocated by %s()",
                                   ++count, func));
        pt->alpha = alpha;
        flags = SDL_HWSURFACE;
        if (alpha)
                flags |= SDL_SRCALPHA;
        pt->surface = SDL_CreateRGBSurface(flags, width, height,
                                           sdl_format.BitsPerPixel,
                                           sdl_format.Rmask, sdl_format.Gmask,
                                           sdl_format.Bmask, sdl_format.Amask);
        SDL_SetAlpha(pt->surface, SDL_SRCALPHA | SDL_RLEACCEL,
                     SDL_ALPHA_OPAQUE);
        rect.x = 0;
        rect.y = 0;
        rect.w = width;
        rect.h = height;
        SDL_FillRect(pt->surface, &rect, 0);
        glGenTextures(1, &pt->gl_name);
        R_check_errors();
        if (c_mem_check.value.n)
                C_trace_full(file, line, func, "Allocated texture #%d", count);
        return pt;
}

/******************************************************************************\
 Allocates a new texture and copies the contents of another texture to it.
 The texture must be uploaded after calling this function.
\******************************************************************************/
r_texture_t *R_texture_clone_full(const char *file, int line, const char *func,
                                  const r_texture_t *src)
{
        r_texture_t *dest;

        if (!src)
                return NULL;
        dest = R_texture_alloc_full(file, line, func, src->surface->w,
                                    src->surface->h, src->alpha);
        if (!dest)
                return NULL;
        if (SDL_BlitSurface(src->surface, NULL, dest->surface, NULL) < 0)
                C_warning("Failed to clone texture '%s': %s",
                          src->ref.name, SDL_GetError());
        dest->mipmaps = src->mipmaps;
        return dest;
}

/******************************************************************************\
 If the texture's SDL surface has changed, the image must be reloaded into
 OpenGL. This function will do this. It is assumed that the texture surface
 format has not changed since the texture was created. Note that mipmaps will
 make UI textures look blurry so do not use them for anything that will be
 rendered in 2D mode.
\******************************************************************************/
void R_texture_upload(const r_texture_t *pt)
{
        int flags, gl_internal;

        flags = SDL_HWSURFACE;
        if (pt->alpha) {
                gl_internal = GL_RGBA;
                if (r_color_bits.value.n == 16)
                        gl_internal = GL_RGBA4;
                else if (r_color_bits.value.n == 32)
                        gl_internal = GL_RGBA8;
                flags |= SDL_SRCALPHA;
        } else {
                gl_internal = GL_RGB;
                if (r_color_bits.value.n == 16)
                        gl_internal = GL_RGB5;
                else if (r_color_bits.value.n == 32)
                        gl_internal = GL_RGB8;
        }
        glBindTexture(GL_TEXTURE_2D, pt->gl_name);
        if (pt->mipmaps)
                gluBuild2DMipmaps(GL_TEXTURE_2D, gl_internal,
                                  pt->surface->w, pt->surface->h,
                                  GL_RGBA, GL_UNSIGNED_BYTE,
                                  pt->surface->pixels);
        else
                glTexImage2D(GL_TEXTURE_2D, 0, gl_internal,
                             pt->surface->w, pt->surface->h, 0,
                             GL_RGBA, GL_UNSIGNED_BYTE, pt->surface->pixels);
        R_check_errors();
}

/******************************************************************************\
 Unload all textures from OpenGL.
\******************************************************************************/
void R_dealloc_textures(void)
{
        r_texture_t *tex;

        C_debug("Deallocting loaded textures");
        tex = (r_texture_t *)root;
        while (tex) {
                glDeleteTextures(1, &tex->gl_name);
                tex = (r_texture_t *)tex->ref.next;
        }

        C_debug("Deallocating allocated textures");
        tex = (r_texture_t *)root_alloc;
        while (tex) {
                glDeleteTextures(1, &tex->gl_name);
                tex = (r_texture_t *)tex->ref.next;
        }
}

/******************************************************************************\
 Reuploads all textures in the linked list to OpenGL.
\******************************************************************************/
void R_realloc_textures(void)
{
        r_texture_t *tex;

        C_debug("Uploading loaded textures");
        tex = (r_texture_t *)root;
        while (tex) {
                glGenTextures(1, &tex->gl_name);
                R_texture_upload(tex);
                tex = (r_texture_t *)tex->ref.next;
        }

        C_debug("Uploading allocated textures");
        tex = (r_texture_t *)root_alloc;
        while (tex) {
                glGenTextures(1, &tex->gl_name);
                R_texture_upload(tex);
                tex = (r_texture_t *)tex->ref.next;
        }
}

/******************************************************************************\
 Low-level callbacks for the PNG library.
\******************************************************************************/
static void user_png_read(png_structp png_ptr, png_bytep data,
                          png_size_t length)
{
        C_file_read((c_file_t *)png_get_io_ptr(png_ptr), (char *)data, 
                    (int)length);
}

static void user_png_write(png_structp png_ptr, png_bytep data,
                          png_size_t length)
{
        C_file_write((c_file_t *)png_get_io_ptr(png_ptr), (char *)data, 
                     (int)length);
}

static void user_png_flush(png_structp png_ptr)
{
        C_file_flush((c_file_t *)png_get_io_ptr(png_ptr));
}

/******************************************************************************\
 Loads a PNG file and allocates a new SDL surface for it. Returns NULL on
 failure.

 Based on tutorial implementation in the libpng manual:
 http://www.libpng.org/pub/png/libpng-1.2.5-manual.html
\******************************************************************************/
static SDL_Surface *load_png(const char *filename, int *alpha)
{
        SDL_Surface *surface;
        Uint32 flags;
        png_byte png_header[8];
        png_bytep row_pointers[4096];
        png_infop info_ptr;
        png_structp png_ptr;
        png_uint_32 width, height;
        c_file_t file;
        int i, bit_depth, color_type;

        surface = NULL;

        /* We have to read everything ourselves for libpng */
        if (!C_file_init_read(&file, filename)) {
                C_warning("Failed to open '%s' for reading", filename);
                return NULL;
        }

        /* Check the first few bytes of the file to see if it is PNG format */
        C_file_read(&file, (char *)png_header, sizeof (png_header));
        if (png_sig_cmp(png_header, 0, sizeof (png_header))) {
                C_warning("'%s' is not in PNG format", filename);
                C_file_cleanup(&file);
                return NULL;
        }

        /* Allocate the PNG structures */
        png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                         NULL, NULL, NULL);
        if (!png_ptr) {
                C_warning("Failed to allocate PNG struct");
                return NULL;
        }

        /* Tells libpng that we've read a bit of the header already */
        png_set_sig_bytes(png_ptr, sizeof (png_header));

        /* Set read callback before proceeding */
        png_set_read_fn(png_ptr, (voidp)&file, (png_rw_ptr)user_png_read);

        /* If an error occurs in libpng, it will longjmp back here */
        if (setjmp(png_ptr->jmpbuf)) {
		C_warning("Error loading PNG '%s'", filename);
		goto cleanup;
	}

        /* Allocate a PNG info struct */
        info_ptr = png_create_info_struct(png_ptr);
        if (!info_ptr) {
                C_warning("Failed to allocate PNG info struct");
                goto cleanup;
        }

        /* Read the image info */
        png_read_info(png_ptr, info_ptr);
        png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                     &color_type, NULL, NULL, NULL);

        /* If the image has an alpha channel we want to store it properly */
        *alpha = FALSE;
        if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
                *alpha = TRUE;

        /* Convert paletted images to RGB */
        if (color_type == PNG_COLOR_TYPE_PALETTE)
                png_set_palette_to_rgb(png_ptr);

        /* Convert transparent regions to alpha channel */
        if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
                png_set_tRNS_to_alpha(png_ptr);
                *alpha = TRUE;
        }

        /* Convert grayscale images to RGB */
        if (color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
                png_set_expand(png_ptr);
                png_set_gray_to_rgb(png_ptr);
        }

        /* Crop 16-bit image data to 8-bit */
        if (bit_depth == 16)
                png_set_strip_16(png_ptr);

        /* Give opaque images an opaque alpha channel (ARGB) */
        if (!(color_type & PNG_COLOR_MASK_ALPHA))
                png_set_filler(png_ptr, 0xff, PNG_FILLER_BEFORE);

        /* Convert 1-, 2-, and 4-bit samples to 8-bit */
        png_set_packing(png_ptr);

        /* Let libpng handle interlacing */
        png_set_interlace_handling(png_ptr);

        /* Update our image information */
        png_read_update_info(png_ptr, info_ptr);
        png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth,
                     &color_type, NULL, NULL, NULL);

        /* Sanity check */
        if (width < 1 || height < 1) {
                C_warning("Invalid PNG image dimensions %dx%d", width, height);
                goto cleanup;
        }
        if (height > sizeof (row_pointers)) {
                C_warning("PNG image is too tall (%d), cropping", height);
                height = sizeof (row_pointers);
        }

        /* Allocate the SDL surface */
        flags = SDL_HWSURFACE;
        if (*alpha)
                flags |= SDL_SRCALPHA;
        surface = SDL_CreateRGBSurface(flags, width, height,
                                       sdl_format.BitsPerPixel,
                                       sdl_format.Rmask, sdl_format.Gmask,
                                       sdl_format.Bmask, sdl_format.Amask);
        SDL_SetAlpha(surface, SDL_SRCALPHA | SDL_RLEACCEL, SDL_ALPHA_OPAQUE);

        /* Get image data */
        if (SDL_LockSurface(surface) < 0) {
                C_warning("Failed to lock surface");
                goto cleanup;
        }
        for (i = 0; i < (int)height; i++)
                row_pointers[i] = (char *)surface->pixels + surface->pitch * i;
        png_read_image(png_ptr, row_pointers);
        SDL_UnlockSurface(surface);

cleanup:
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        C_file_cleanup(&file);
        return surface;
}

/******************************************************************************\
 Loads a texture from file. If the texture has already been loaded, just ups
 the reference count and returns the loaded surface. If the texture failed to
 load, returns NULL.
\******************************************************************************/
r_texture_t *R_texture_load(const char *filename, int mipmaps)
{
        r_texture_t *pt;
        int found;

        if (!filename || !filename[0])
                return NULL;

        /* Find a place for the texture */
        pt = C_ref_alloc(sizeof (*pt), &root, (c_ref_cleanup_f)texture_cleanup,
                         filename, &found);
        if (found)
                return pt;

        /* Load the image file */
        pt->mipmaps = mipmaps;
        pt->surface = load_png(filename, &pt->alpha);
        if (!pt->surface) {
                C_warning("Failed to load texture '%s'", filename);
                C_ref_down(&pt->ref);
                return NULL;
        }

        /* Load the texture into OpenGL */
        glGenTextures(1, &pt->gl_name);
        R_texture_upload(pt);
        R_check_errors();

        return pt;
}

/******************************************************************************\
 Selects (binds) a texture for rendering in OpenGL. Also sets whatever options
 are necessary to get the texture to show up properly.
\******************************************************************************/
void R_texture_select(r_texture_t *texture)
{
        if (!texture) {
                glDisable(GL_TEXTURE_2D);
                glBindTexture(GL_TEXTURE_2D, 0);
                glDisable(GL_BLEND);
                glDisable(GL_ALPHA_TEST);
                return;
        }
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture->gl_name);

        /* Texture repeat */
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        /* Mipmaps */
        if (texture->mipmaps) {
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR_MIPMAP_LINEAR);
                if (texture->mipmaps > 1)
                        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD,
                                        (GLfloat)texture->mipmaps);
        } else
                glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                                GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        /* Anisotropic filtering */
        if (r_extensions[R_EXT_ANISOTROPY] > 1) {
                GLfloat aniso;

                aniso = texture->anisotropy;
                if (aniso > r_extensions[R_EXT_ANISOTROPY])
                        aniso = (GLfloat)r_extensions[R_EXT_ANISOTROPY];
                if (aniso < 1.f)
                        aniso = 1.f;
                glTexParameterf(GL_TEXTURE_2D,
                                GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
        }

        /* Alpha blending */
        if (texture->alpha) {
                glEnable(GL_BLEND);
                glEnable(GL_ALPHA_TEST);
        } else {
                glDisable(GL_BLEND);
                glDisable(GL_ALPHA_TEST);
        }

        R_check_errors();
}

/******************************************************************************\
 Renders a plain quad without any sprite effects. Coordinates refer to the
 upper left-hand corner.
\******************************************************************************/
void R_texture_render(r_texture_t *tex, int x, int y)
{
        r_vertex2_t verts[4];
        unsigned short indices[] = {0, 1, 2, 3};

        verts[0].co = C_vec3(0.f, 0.f, 0.f);
        verts[0].uv = C_vec2(0.f, 0.f);
        verts[1].co = C_vec3(0.f, (float)tex->surface->h, 0.f);
        verts[1].uv = C_vec2(0.f, 1.f);
        verts[2].co = C_vec3((float)tex->surface->w,
                             (float)tex->surface->h, 0.f);
        verts[2].uv = C_vec2(1.f, 1.f);
        verts[3].co = C_vec3((float)tex->surface->w, 0.f, 0.f);
        verts[3].uv = C_vec2(1.f, 0.f);
        R_push_mode(R_MODE_2D);
        R_texture_select(tex);
        glTranslatef((GLfloat)x, (GLfloat)y, 0.f);
        glInterleavedArrays(R_VERTEX2_FORMAT, 0, verts);
        glDrawElements(GL_QUADS, 4, GL_UNSIGNED_SHORT, indices);
        R_check_errors();
        R_pop_mode();
}

/******************************************************************************\
 Returns the height of the tallest glyph in the font.
\******************************************************************************/
int R_font_height(r_font_t font)
{
        if (!fonts[font].ttf_font)
                C_error("Forgot to load fonts");
        return fonts[font].height;
}

/******************************************************************************\
 Returns the width of the widest glyph in the font.
\******************************************************************************/
int R_font_width(r_font_t font)
{
        if (!fonts[font].ttf_font)
                C_error("Forgot to load fonts");
        return fonts[font].width;
}

/******************************************************************************\
 Returns distance from start of one line to the start of the next.
\******************************************************************************/
int R_font_line_skip(r_font_t font)
{
        return fonts[font].line_skip;
}

/******************************************************************************\
 Returns distance from start of one line to the start of the next.
\******************************************************************************/
c_vec2_t R_font_size(r_font_t font, const char *text)
{
        int w, h;

        TTF_SizeUTF8(fonts[font].ttf_font, text, &w, &h);
        return C_vec2((float)w, (float)h);
}

/******************************************************************************\
 Renders a line of text onto a newly-allocated SDL surface. This surface must
 be freed by the caller.
\******************************************************************************/
SDL_Surface *R_font_render(r_font_t font, const char *text)
{
        SDL_Surface *surf;
        SDL_Color white = { 255, 255, 255, 255 };

        surf = TTF_RenderUTF8_Blended(fonts[font].ttf_font, text, white);
        if (!surf) {
                C_warning("TTF_RenderUTF8_Blended() failed: %s",
                          TTF_GetError());
                return surf;
        }
        return surf;
}

/******************************************************************************\
 Loads the font, properly scaled and generates an error message if something
 goes wrong. For some reason, SDL_ttf returns a line skip that is one pixel shy
 of the image height returned by TTF_RenderUTF8_Blended() so we account for
 that here.
\******************************************************************************/
static void load_font(r_font_t font, const char *path, int size)
{
        int points;

        points = (int)ceilf(size * r_pixel_scale.value.f);
        if (points < R_FONT_SIZE_MIN)
                points = R_FONT_SIZE_MIN;
        C_zero(fonts + font);
        fonts[font].ttf_font = TTF_OpenFont(path, points);
        if (!fonts[font].ttf_font) {
                C_warning("Failed to load font '%s' (%d -> %d pt)",
                          path, size, points);
                return;
        }
        fonts[font].height = TTF_FontHeight(fonts[font].ttf_font);
        fonts[font].line_skip = TTF_FontLineSkip(fonts[font].ttf_font) + 1;

        /* SDL_ttf won't tell us the width of the widest glyph directly so we
           assume it is the width of 'W' */
        fonts[font].width = (int)R_font_size(font, "W").x;
}

/******************************************************************************\
 Initialize the SDL TTF library and generate font structures.
\******************************************************************************/
void R_load_fonts(void)
{
        const char *override;
        int i;

        if (!ttf_inited)
                return;

        /* Get the locale font override, if any */
        override = C_str("r-font-override", "");
        if (override[0]) {
                C_var_set(&r_font_console, override);
                C_var_set(&r_font_gui, override);
                C_var_set(&r_font_title, override);
        }

        /* Console font */
        C_var_unlatch(&r_font_console);
        C_var_unlatch(&r_font_console_pt);
        load_font(R_FONT_CONSOLE, r_font_console.value.s,
                  r_font_console_pt.value.n);

        /* GUI font */
        C_var_unlatch(&r_font_gui);
        C_var_unlatch(&r_font_gui_pt);
        load_font(R_FONT_GUI, r_font_gui.value.s, r_font_gui_pt.value.n);

        /* Title font */
        C_var_unlatch(&r_font_title);
        C_var_unlatch(&r_font_title_pt);
        load_font(R_FONT_TITLE, r_font_title.value.s, r_font_title_pt.value.n);

        for (i = 0; i < R_FONTS; i++)
                if (!fonts[i].ttf_font)
                        C_error("Forgot to load font %d", i + 1);
}

/******************************************************************************\
 Loads render assets.
\******************************************************************************/
void R_load_assets(void)
{
        SDL_version compiled;
        const SDL_version *linked;

        C_status("Loading render assets");

        /* Setup the texture pixel format, RGBA in 32 bits */
        sdl_format.BitsPerPixel = 32;
        sdl_format.BytesPerPixel = 4;
        sdl_format.Amask = 0xff000000;
        sdl_format.Bmask = 0x00ff0000;
        sdl_format.Gmask = 0x0000ff00;
        sdl_format.Rmask = 0x000000ff;
        sdl_format.Ashift = 24;
        sdl_format.Bshift = 16;
        sdl_format.Gshift = 8;
        sdl_format.alpha = 255;

        /* Initialize SDL_ttf library */
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
        ttf_inited = TRUE;
        R_load_fonts();

        /* Generate procedural content */
        r_terrain_tex = R_texture_load("models/globe/terrain.png", TRUE);
        R_prerender();
}

/******************************************************************************\
 Cleans up fonts.
\******************************************************************************/
void R_free_fonts(void)
{
        int i;

        if (!ttf_inited)
                return;
        for (i = 0; i < R_FONTS; i++)
                TTF_CloseFont(fonts[i].ttf_font);
}

/******************************************************************************\
 Cleans up the asset resources. Shuts down the SDL_ttf library.
\******************************************************************************/
void R_free_assets(void)
{
        R_texture_free(r_terrain_tex);
        R_free_fonts();
        TTF_Quit();
}

