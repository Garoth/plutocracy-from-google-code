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
c_var_t r_test_model_path, r_test_sprite_num, r_test_sprite_path, r_test_text;

/* Fonts */
c_var_t r_font_console, r_font_console_pt,
        r_font_gui, r_font_gui_pt;

/******************************************************************************\
 Registers the render namespace variables.
\******************************************************************************/
void R_register_variables(void)
{
        /* Video parameters */
        C_register_integer(&r_width, "r_width", 1024);
        C_register_integer(&r_height, "r_height", 768);
        C_register_integer(&r_color_bits, "r_color_bits", 32);
        C_register_integer(&r_depth_bits, "r_depth_bits", 16);
        C_register_integer(&r_windowed, "r_windowed", TRUE);
        C_register_integer(&r_vsync, "r_vsync", TRUE);
        C_register_float(&r_gamma, "r_gamma", 1);
        C_register_float(&r_pixel_scale, "r_pixel_scale", 1);
        C_register_string(&r_clear, "r_clear", "black");
        C_register_integer(&r_gl_errors, "r_gl_errors", 0);
        r_gl_errors.edit = C_VE_ANYTIME;

        /* Render testing */
        C_register_string(&r_test_model_path, "r_test_model", "");
        r_test_model_path.archive = FALSE;
        C_register_integer(&r_test_sprite_num, "r_test_sprites", 0);
        r_test_sprite_num.archive = FALSE;
        C_register_string(&r_test_sprite_path, "r_test_sprite", "");
        r_test_sprite_path.archive = FALSE;
        C_register_string(&r_test_text, "r_test_text", "");
        r_test_text.archive = FALSE;

        /* Fonts */
        C_register_string(&r_font_console, "r_font_console",
                          "gui/fonts/VeraMoBd.ttf");
        r_font_console.archive = FALSE;
        C_register_integer(&r_font_console_pt, "r_font_console_pt", 12);
        r_font_console_pt.archive = FALSE;
        C_register_string(&r_font_gui, "r_font_gui",
                          "gui/fonts/BLKCHCRY.TTF");
        r_font_gui.archive = FALSE;
        C_register_integer(&r_font_gui_pt, "r_font_gui_pt", 19);
        r_font_gui_pt.archive = FALSE;
}

