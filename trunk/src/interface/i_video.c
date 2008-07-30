/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "i_common.h"

/* Options implemented */
enum {
        OPT_RESOLUTION,
        OPT_COLOR_BITS,
        OPT_WINDOWED,
        OPT_MULTISAMPLE,
        OPT_GAMMA,
        OPT_PIXEL_SCALE,
        OPT_DRAW_DISTANCE,
        OPTIONS,
        OPTIONS_APPLY = OPT_MULTISAMPLE
};

/* Length of video modes list */
#define VIDEO_MODES 64

static i_label_t label;
static i_button_t apply_button;
static i_select_t options[OPTIONS];
static SDL_Rect **sdl_modes;
static int orig_indices[OPTIONS], mode_indices[VIDEO_MODES];

/******************************************************************************\
 Set the apply button's state depending on if options have changed or not.
\******************************************************************************/
static void set_apply_state(void)
{
        int i;

        for (i = 0; i < OPTIONS_APPLY; i++)
                if (options[i].index != orig_indices[i]) {
                        apply_button.widget.state = I_WS_READY;
                        return;
                }
        apply_button.widget.state = I_WS_DISABLED;
}

/******************************************************************************\
 Apply modified settings.
\******************************************************************************/
static void apply_button_clicked(i_button_t *button)
{
        int i;

        for (i = 0; i < OPTIONS_APPLY; i++)
                orig_indices[i] = options[i].index;
        r_restart = TRUE;

        /* Didn't get an SDL resolution list? */
        if (!sdl_modes)
                return;

        /* Resolutions list is reversed */
        i = mode_indices[options[OPT_RESOLUTION].index];
        C_var_set(&r_width, C_va("%d", sdl_modes[i]->w));
        C_var_set(&r_height, C_va("%d", sdl_modes[i]->h));
        set_apply_state();
}

/******************************************************************************\
 Populates the video mode list.
\******************************************************************************/
static void populate_modes(void)
{
        int i, j;

        sdl_modes = SDL_ListModes(NULL, SDL_OPENGL | SDL_GL_DOUBLEBUFFER |
                                        SDL_HWPALETTE | SDL_HWSURFACE |
                                        SDL_FULLSCREEN);

        /* SDL_ListModes() won't always return a list */
        if (!sdl_modes || sdl_modes == (void *)-1) {
                C_warning("SDL_ListModes() did not return a list of modes");
                sdl_modes = NULL;
                return;
        }

        /* Add the video modes that work for us */
        orig_indices[OPT_RESOLUTION] = 0;
        for (i = j = 0; sdl_modes[i] && j < VIDEO_MODES - 1; i++) {
                if (sdl_modes[i]->w < R_WIDTH_MIN ||
                    sdl_modes[i]->h < R_HEIGHT_MIN)
                        continue;
                I_select_add_string(options + OPT_RESOLUTION,
                                    C_va("%dx%d", sdl_modes[j]->w,
                                                  sdl_modes[j]->h));
                mode_indices[j++] = i;
        }

        /* Since we added all the modes to the front, reverse indices */
        for (i = 0; i < options[OPT_RESOLUTION].list_len; i++)
                mode_indices[i] = options[OPT_RESOLUTION].list_len -
                                  mode_indices[i] - 1;
}

/******************************************************************************\
 Update interface settings from variable values.
\******************************************************************************/
void I_update_video(void)
{
        int i;

        /* Find a matching video mode */
        orig_indices[OPT_RESOLUTION] = 0;
        for (i = 0; i < options[OPT_RESOLUTION].list_len; i++) {
                int index;

                index = mode_indices[i];
                if (r_width.latched.n != sdl_modes[index]->w ||
                    r_height.latched.n != sdl_modes[index]->h)
                        continue;
                orig_indices[OPT_RESOLUTION] = i;
                break;
        }
        I_select_change(options + OPT_RESOLUTION, orig_indices[OPT_RESOLUTION]);

        /* Do the rest of the widgets */
        for (i = 0; i < OPTIONS; i++)
                if (i != OPT_RESOLUTION) {
                        I_select_update(options + i);
                        orig_indices[i] = options[i].index;
                }
}

/******************************************************************************\
 Initializes video window widgets on the given window.
\******************************************************************************/
void I_init_video(i_window_t *window)
{
        i_select_t *select;

        I_window_init(window);
        window->widget.size = C_vec2(260.f, 0.f);
        window->fit = I_FIT_TOP;

        /* Label */
        I_label_init(&label, C_str("i-video", "Video Settings"));
        label.font = R_FONT_TITLE;
        I_widget_add(&window->widget, &label.widget);

        /* Screen resolution */
        select = options + OPT_RESOLUTION;
        I_select_init(select, C_str("i-video-mode", "Resolution:"), NULL);
        select->on_change = (i_callback_f)set_apply_state;
        populate_modes();
        I_widget_add(&window->widget, &select->widget);

        /* Select color bits */
        select = options + OPT_COLOR_BITS;
        I_select_init(select, C_str("i-video-color", "Color bits:"), NULL);
        select->on_change = (i_callback_f)set_apply_state;
        select->variable = &r_color_bits;
        I_select_add_int(select, 32, NULL);
        I_select_add_int(select, 16, NULL);
        I_widget_add(&window->widget, &select->widget);

        /* Select windowed */
        select = options + OPT_WINDOWED;
        I_select_init(select, C_str("i-video-windowed", "Windowed:"), NULL);
        select->on_change = (i_callback_f)set_apply_state;
        select->variable = &r_windowed;
        I_select_add_int(select, TRUE, "Yes");
        I_select_add_int(select, FALSE, "No");
        I_widget_add(&window->widget, &select->widget);

        /* Select multisampling */
        select = options + OPT_MULTISAMPLE;
        I_select_init(select, C_str("i-video-multisample", "Multisampling:"),
                      NULL);
        select->on_change = (i_callback_f)set_apply_state;
        select->variable = &r_multisample;
        I_select_add_int(select, 4, "4x");
        I_select_add_int(select, 2, "2x");
        I_select_add_int(select, 1, "1x");
        I_select_add_int(select, 0, "Off");
        I_widget_add(&window->widget, &select->widget);

        /* Select gamma */
        select = options + OPT_GAMMA;
        I_select_init(select, C_str("i-video-gamma", "Gamma correction:"),
                      NULL);
        select->variable = &r_gamma;
        select->min = 0.5f;
        select->max = 2.5f;
        select->increment = 0.25f;
        I_widget_add(&window->widget, &select->widget);

        /* Select pixel scale */
        select = options + OPT_PIXEL_SCALE;
        I_select_init(select, C_str("i-video-pixel-scale", "Scale 2D:"), NULL);
        select->variable = &r_pixel_scale;
        select->min = 0.5f;
        select->max = 2.0f;
        select->increment = 0.25f;
        I_widget_add(&window->widget, &select->widget);

        /* Select draw distance */
        select = options + OPT_DRAW_DISTANCE;
        I_select_init(select, C_str("i-video-draw-distance", "Draw Distance:"),
                      NULL);
        select->variable = &g_draw_distance;
        I_select_add_float(select, 30.f, "Far");
        I_select_add_float(select, 15.f, "Medium");
        I_select_add_float(select, 5.f, "Near");
        I_widget_add(&window->widget, &select->widget);

        /* Apply button */
        I_button_init(&apply_button, NULL,
                      C_str("i-video-apply", "Apply"), I_BT_DECORATED);
        apply_button.on_click = (i_callback_f)apply_button_clicked;
        apply_button.widget.margin_front = 1.f;
        I_widget_add(&window->widget, &apply_button.widget);

        /* Get the best interface values from variables */
        I_update_video();
        set_apply_state();
}

