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

/* Draw distance values */
#define DRAW_DISTANCE_NEAR 5.f
#define DRAW_DISTANCE_MEDIUM 15.f
#define DRAW_DISTANCE_FAR 30.f

/* Lists passed to the select widgets must be NULL-terminated! */
static char mode_strings[VIDEO_MODES][16], *list_modes[VIDEO_MODES];
static const char *list_bool[] = {"No", "Yes", NULL},
                  *list_numeric[] = {"0.50", "0.75", "1.00", "1.25",
                                     "1.50", "1.75", "2.00", NULL},
                  *list_color_bits[] = {"16", "32", NULL},
                  *list_multisample[] = {"Off", "2x", "4x", NULL},
                  *list_distance[] = {"Near", "Medium", "Far", NULL};

static i_label_t label;
static i_button_t apply_button;
static i_select_t options[OPTIONS];
static SDL_Rect *video_modes[VIDEO_MODES];
static int orig_indices[OPTIONS];

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
        if (!video_modes[0])
                return;
        i = orig_indices[0];
        if (r_width.value.n != video_modes[i]->w) {
                r_width.latched.n = video_modes[i]->w;
                r_width.has_latched = TRUE;
        } else
                r_width.has_latched = FALSE;
        if (r_height.value.n != video_modes[i]->h) {
                r_height.latched.n = video_modes[i]->h;
                r_height.has_latched = TRUE;
        } else
                r_height.has_latched = FALSE;
        set_apply_state();
}

/******************************************************************************\
 Set a variable when the select widget is changed.
\******************************************************************************/
static void on_change_set(i_select_t *select)
{
        c_var_t *var;

        var = (c_var_t *)select->data;
        C_var_set(var, select->list[select->index]);
        set_apply_state();
}

/******************************************************************************\
 Returns the index of the closest digit in a NULL terminated string array.
\******************************************************************************/
static int closest_index(float value, const char **list)
{
        float index_diff, diff;
        int i, index;

        for (i = index = 0, index_diff = 100000.f; list[i]; i++) {
                diff = (float)atof(list[i]) - value;
                if (!diff)
                        return i;
                if (diff < 0.f)
                        diff = -diff;
                if (diff < index_diff) {
                        index = i;
                        index_diff = diff;
                }
        }
        return index;
}

/******************************************************************************\
 Populates the video mode list.
\******************************************************************************/
static void populate_modes(void)
{
        SDL_Rect **sdl_modes;
        int i, j;

        sdl_modes = SDL_ListModes(NULL, SDL_OPENGL | SDL_GL_DOUBLEBUFFER |
                                        SDL_HWPALETTE | SDL_HWSURFACE |
                                        SDL_FULLSCREEN);

        /* FIXME: SDL_ListModes() won't always return a list */
        if (!sdl_modes || sdl_modes == (void *)-1) {
                C_warning("SDL_ListModes() did not return a list of modes");
                list_modes[0] = NULL;
                orig_indices[0] = 0;
                video_modes[0] = NULL;
                return;
        }

        orig_indices[OPT_RESOLUTION] = 0;
        for (i = j = 0; sdl_modes[i] && i < VIDEO_MODES - 1; i++) {
                if (sdl_modes[i]->w < R_WIDTH_MIN ||
                    sdl_modes[i]->h < R_HEIGHT_MIN)
                        continue;
                if (r_width.latched.n == sdl_modes[i]->w &&
                    r_height.latched.n == sdl_modes[i]->h)
                        orig_indices[OPT_RESOLUTION] = i;
                video_modes[j] = sdl_modes[i];
                list_modes[j] = (char *)(mode_strings + j);
                snprintf(mode_strings[j], sizeof (mode_strings[j]),
                         "%dx%d", video_modes[j]->w, video_modes[j]->h);
                j++;
        }
        list_modes[j] = NULL;
}

/******************************************************************************\
 Set draw distance.
\******************************************************************************/
static void change_draw_distance(i_select_t *select)
{
        g_draw_distance.value.f = DRAW_DISTANCE_MEDIUM;
        if (select->index == 0)
                g_draw_distance.value.f = DRAW_DISTANCE_NEAR;
        if (select->index == 2)
                g_draw_distance.value.f = DRAW_DISTANCE_FAR;
}

/******************************************************************************\
 Update interface settings from variable values.
\******************************************************************************/
static void update_video(void)
{
        /* Resolution */
        populate_modes();

        /* Color depth */
        orig_indices[OPT_COLOR_BITS] =
                closest_index((float)r_color_bits.latched.n, list_color_bits);

        /* Windowed */
        orig_indices[OPT_WINDOWED] = r_windowed.latched.n ? 1 : 0;

        /* Multisampling */
        orig_indices[OPT_MULTISAMPLE] =
                closest_index((float)r_multisample.latched.n, list_multisample);

        /* Gamma correction */
        orig_indices[OPT_GAMMA] = closest_index(r_gamma.latched.f,
                                                list_numeric);

        /* Pixel scale */
        orig_indices[OPT_PIXEL_SCALE] = closest_index(r_pixel_scale.latched.f,
                                                      list_numeric);

        /* Draw distance */
        orig_indices[OPT_DRAW_DISTANCE] = 1;
        if (g_draw_distance.value.f == DRAW_DISTANCE_FAR)
                orig_indices[OPT_DRAW_DISTANCE] = 2;
        else if (g_draw_distance.value.f == DRAW_DISTANCE_NEAR)
                orig_indices[OPT_DRAW_DISTANCE] = 0;
}

/******************************************************************************\
 Update interface settings from variable values and reconfigure the select
 widgets with the new values.
\******************************************************************************/
void I_update_video(void)
{
        int i;

        C_debug("Reconfiguring video window options");
        update_video();
        for (i = 0; i < OPTIONS; i++)
                I_select_change(options + i, orig_indices[i]);
}

/******************************************************************************\
 Initializes video window widgets on the given window.
\******************************************************************************/
void I_init_video(i_window_t *window)
{
        I_window_init(window);
        window->widget.size = C_vec2(260.f, 0.f);
        window->fit = I_FIT_TOP;

        /* Label */
        I_label_init(&label, C_str("i-video", "Video Settings"));
        label.font = R_FONT_TITLE;
        I_widget_add(&window->widget, &label.widget);

        /* Get the best interface values from variables */
        update_video();

        /* Screen resolution */
        I_select_init(options + OPT_RESOLUTION,
                      C_str("i-video-mode", "Resolution:"),
                      (const char **)list_modes,
                      orig_indices[OPT_RESOLUTION], TRUE);
        options[OPT_RESOLUTION].on_change = (i_callback_f)set_apply_state;
        I_widget_add(&window->widget, &options[OPT_RESOLUTION].widget);

        /* Select color bits */
        I_select_init(options + OPT_COLOR_BITS,
                      C_str("i-video-color", "Color bits:"),
                      list_color_bits, orig_indices[OPT_COLOR_BITS], FALSE);
        options[OPT_COLOR_BITS].on_change = (i_callback_f)on_change_set;
        options[OPT_COLOR_BITS].data = &r_color_bits;
        I_widget_add(&window->widget, &options[OPT_COLOR_BITS].widget);

        /* Select windowed */
        I_select_init(options + OPT_WINDOWED,
                      C_str("i-video-windowed", "Windowed:"),
                      list_bool, orig_indices[OPT_WINDOWED], FALSE);
        options[OPT_WINDOWED].on_change = (i_callback_f)on_change_set;
        options[OPT_WINDOWED].data = &r_windowed;
        I_widget_add(&window->widget, &options[OPT_WINDOWED].widget);

        /* Select multisampling */
        I_select_init(options + OPT_MULTISAMPLE,
                      C_str("i-video-multisample", "Multisampling:"),
                      list_multisample, orig_indices[OPT_MULTISAMPLE], FALSE);
        options[OPT_MULTISAMPLE].on_change = (i_callback_f)on_change_set;
        options[OPT_MULTISAMPLE].data = &r_multisample;
        I_widget_add(&window->widget, &options[OPT_MULTISAMPLE].widget);

        /* Select gamma */
        I_select_init(options + OPT_GAMMA,
                      C_str("i-video-gamma", "Gamma correction:"),
                      list_numeric, orig_indices[OPT_GAMMA], FALSE);
        options[OPT_GAMMA].on_change = (i_callback_f)on_change_set;
        options[OPT_GAMMA].data = &r_gamma;
        I_widget_add(&window->widget, &options[OPT_GAMMA].widget);

        /* Select pixel scale */
        I_select_init(&options[OPT_PIXEL_SCALE],
                      C_str("i-video-pixel-scale", "Scale 2D:"),
                      list_numeric, orig_indices[OPT_PIXEL_SCALE], FALSE);
        options[OPT_PIXEL_SCALE].on_change = (i_callback_f)on_change_set;
        options[OPT_PIXEL_SCALE].data = &r_pixel_scale;
        I_widget_add(&window->widget, &options[OPT_PIXEL_SCALE].widget);

        /* Select draw distance */
        I_select_init(&options[OPT_DRAW_DISTANCE],
                      C_str("i-video-draw-distance", "Draw Distance:"),
                      list_distance, orig_indices[OPT_DRAW_DISTANCE], FALSE);
        options[OPT_DRAW_DISTANCE].on_change =
                (i_callback_f)change_draw_distance;
        I_widget_add(&window->widget, &options[OPT_DRAW_DISTANCE].widget);

        /* Apply button */
        I_button_init(&apply_button, NULL,
                      C_str("i-video-apply", "Apply"), I_BT_DECORATED);
        apply_button.on_click = (i_callback_f)apply_button_clicked;
        apply_button.widget.margin_front = 1.f;
        apply_button.widget.state = I_WS_DISABLED;
        I_widget_add(&window->widget, &apply_button.widget);
}

