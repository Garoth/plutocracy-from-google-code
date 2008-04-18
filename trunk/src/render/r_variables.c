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

/* Video parameters */
c_var_t r_width, r_height, r_color_bits, r_depth_bits, r_windowed, r_vsync,
        r_gamma, r_pixel_scale, r_clear, r_gl_errors;

/* Render testing */
c_var_t r_test_model, r_test_prerender, r_test_sprite, r_test_sprite_num,
        r_test_text;

/* Effects parameters */
c_var_t r_globe_smooth;

/* Fonts */
c_var_t r_font_console, r_font_console_pt,
        r_font_gui, r_font_gui_pt;

/******************************************************************************\
 Registers the render namespace variables.
\******************************************************************************/
void R_register_variables(void)
{
        /* Video parameters */
        C_register_integer(&r_width, "r_width", 1024, "window width in pixels");
        C_register_integer(&r_height, "r_height", 768,
                           "window height in pixels");
        C_register_integer(&r_color_bits, "r_color_bits", 32,
                           "texture color depth: 16 or 32 bits");
        C_register_integer(&r_depth_bits, "r_depth_bits", 16,
                           "depth buffer bits: 16 or 24 bits");
        C_register_integer(&r_windowed, "r_windowed", TRUE,
                           "1 = windowed mode, 0 = fullscreen");
        C_register_integer(&r_vsync, "r_vsync", TRUE, "enable vertical sync");
        C_register_float(&r_gamma, "r_gamma", 1, "brightness gamma correction");
        C_register_float(&r_pixel_scale, "r_pixel_scale", 1,
                         "scales the interface pixel unit");
        C_register_string(&r_clear, "r_clear", "black",
                          "background clear color");
        C_register_integer(&r_gl_errors, "r_gl_errors", 0,
                           "enable to quit if there is a GL error");
        r_gl_errors.edit = C_VE_ANYTIME;

        /* Render testing */
        C_register_string(&r_test_model, "r_test_model", "",
                          "path to PLUM model to test");
        C_register_integer(&r_test_prerender, "r_test_prerender", FALSE,
                           "display pre-rendered textures");
        C_register_string(&r_test_sprite, "r_test_sprite", "",
                          "path to test sprite texture");
        C_register_integer(&r_test_sprite_num, "r_test_sprites", 0,
                           "number of test sprites to show");
        C_register_string(&r_test_text, "r_test_text", "",
                          "text to test rendering");

        /* Visual effects parameters */
        C_register_float(&r_globe_smooth, "r_globe_smooth", 1.f,
                         "amount to smooth globe normals: 0.0-1.0");

        /* Fonts */
        C_register_string(&r_font_console, "r_font_console",
                          "gui/fonts/DejaVuSansMono-Bold.ttf",
                          "path to TTF console font file");
        r_font_console.archive = FALSE;
        C_register_integer(&r_font_console_pt, "r_font_console_pt", 12,
                           "size of console font in points");
        r_font_console_pt.archive = FALSE;
        C_register_string(&r_font_gui, "r_font_gui", "gui/fonts/BLKCHCRY.TTF",
                          "path to GUI font TTF file");
        r_font_gui.archive = FALSE;
        C_register_integer(&r_font_gui_pt, "r_font_gui_pt", 19,
                           "size of GUI font in points");
        r_font_gui_pt.archive = FALSE;
}

