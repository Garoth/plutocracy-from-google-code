/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Contains the root window and toolbars */

#include "i_common.h"

/* Globe light modulation in limbo mode */
#define LIMBO_GLOBE_LIGHT 0.1f

/* The logo fades in/out during limbo mode changes, but not as fast as the
   other windows and GUI elements */
#define LIMBO_FADE_SCALE 0.2f

/* SDL key-repeat initial timeout and repeat interval */
#define KEY_REPEAT_TIMEOUT 300
#define KEY_REPEAT_INTERVAL 30

/* Root widget contains all other widgets */
i_widget_t i_root;

/* TRUE if the interface is in limbo mode */
int i_limbo;

static i_toolbar_t left_toolbar, right_toolbar;
static r_sprite_t limbo_logo;
static float limbo_fade;
static int layout_frame, cleanup_theme;

/******************************************************************************\
 Root window event function.
\******************************************************************************/
static int root_event(i_widget_t *root, i_event_t event)
{
        switch (event) {
        case I_EV_CONFIGURE:
                i_colors[I_COLOR] = C_color_string(i_color.value.s);
                i_colors[I_COLOR_ALT] = C_color_string(i_color_alt.value.s);

                /* Size and position limbo logo */
                limbo_logo.origin.x = r_width_2d / 2 - limbo_logo.size.x / 2;
                limbo_logo.origin.y = r_height_2d / 2 - limbo_logo.size.y / 2;

                break;
        case I_EV_MOUSE_MOVE:
                I_widget_propagate(root, event);
                I_globe_event(event);
                return FALSE;
        case I_EV_RENDER:

                /* Rotate around the globe during limbo */
                if (limbo_fade > 0.f)
                        R_rotate_cam_by(C_vec3(0.f, limbo_fade *
                                                    c_frame_sec / 60.f * C_PI /
                                                    R_MINUTES_PER_DAY, 0.f));
                if (i_limbo) {
                        R_zoom_cam_by((R_ZOOM_MAX - r_cam_zoom) * c_frame_sec);
                        limbo_fade += i_fade.value.f * c_frame_sec *
                                      LIMBO_FADE_SCALE;
                        if (limbo_fade > 1.f)
                                limbo_fade = 1.f;
                } else {
                        limbo_fade -= i_fade.value.f * c_frame_sec *
                                      LIMBO_FADE_SCALE;
                        if (limbo_fade < 0.f)
                                limbo_fade = 0.f;
                }

                /* Globe lighting is modulated by limbo mode */
                r_globe_light = LIMBO_GLOBE_LIGHT +
                                (1.f - LIMBO_GLOBE_LIGHT) * (1.f - limbo_fade);

                /* Render and fade limbo logo */
                if (limbo_fade > 0.f) {
                        limbo_logo.modulate.a = limbo_fade;
                        R_sprite_render(&limbo_logo);
                }

                break;
        default:
                break;
        }

        /* Propagate to the globe interface processing function */
        I_globe_event(event);

        return TRUE;
}

/******************************************************************************\
 Updates themeable widget assets.
\******************************************************************************/
static void theme_widgets(void)
{
        if (i_debug.value.n)
                C_trace("Theming widgets");
        I_theme_buttons();
        I_theme_windows();
        I_theme_entries();
        I_theme_scrollbacks();
        I_theme_ring();
}

/******************************************************************************\
 Updates after a theme change.
\******************************************************************************/
static void theme_configure(void)
{
        float toolbar_height, toolbar_y;

        C_var_unlatch(&i_border);
        C_var_unlatch(&i_color);
        C_var_unlatch(&i_color_alt);
        C_var_unlatch(&i_shadow);

        /* Root window */
        i_root.size = C_vec2((float)r_width_2d, (float)r_height_2d);

        /* Toolbars */
        toolbar_height = 32.f + i_border.value.n * 2;
        toolbar_y = r_height_2d - toolbar_height - i_border.value.n;
        left_toolbar.widget.size = C_vec2(0.f, toolbar_height);
        left_toolbar.widget.origin = C_vec2((float)i_border.value.n, toolbar_y);
        right_toolbar.widget.size = C_vec2(0.f, toolbar_height);
        right_toolbar.widget.origin = C_vec2((float)(r_width_2d -
                                                     i_border.value.n),
                                             toolbar_y);

        theme_widgets();
}

/******************************************************************************\
 Parse the theme config. Does not unlatch [i_theme]. Returns FALSE if the theme
 name is invalid.
\******************************************************************************/
static int parse_config(const char *name)
{
        const char *filename;

        /* The theme name should never modify the path */
        if (C_is_path(name)) {
                C_warning("Theme name contains path characters");
                return FALSE;
        }

        /* Reset blank theme name to default */
        if (!name[0])
                name = "default";

        filename = C_va("gui/themes/%s/theme.cfg", name);
        if (!C_parse_config_file(filename)) {
                C_warning("Failed to parse theme config '%s'", filename);
                return FALSE;
        }
        return TRUE;
}

/******************************************************************************\
 Parse the theme config.
\******************************************************************************/
void I_parse_config(void)
{
        C_var_unlatch(&i_theme);
        if (!parse_config(i_theme.value.s))
                i_theme.value = i_theme.stock;
}

/******************************************************************************\
 Update function for i_theme. Will also reload fonts.
\******************************************************************************/
static int theme_update(c_var_t *var, c_var_value_t value)
{
        /* Reset theme variables before parsing the theme config */
        C_var_reset(&i_border);
        C_var_reset(&i_color);
        C_var_reset(&i_color_alt);
        C_var_reset(&i_shadow);
        R_stock_fonts();

        /* Treat blank as default */
        if (!value.s[0])
                C_strncpy_buf(value.s, "default");

        /* Parse the theme configuration file, if there is one */
        if (!parse_config(value.s))
                return FALSE;
        i_theme.value = value;

        /* Reload fonts */
        R_free_fonts();
        R_load_fonts();

        /* Update theme assets */
        theme_configure();
        I_widget_event(&i_root, I_EV_CONFIGURE);
        return TRUE;
}

/******************************************************************************\
 Convenience function that cleans up and reloads a texture. The [name] argument
 is the filename of the texture file in the theme directory minus the image
 format suffix.
\******************************************************************************/
void I_theme_texture(r_texture_t **ppt, const char *name)
{
        const char *filename;

        R_texture_free(*ppt);
        if (cleanup_theme)
                return;
        *ppt = NULL;

        /* Load the file if it exists, otherwise avoid the warning */
        filename = C_va("gui/themes/%s/%s.png", i_theme.value.s, name);
        if (C_file_exists(filename))
                *ppt = R_texture_load(filename, FALSE);

        /* Try to load default if the selected theme is missing this texture */
        if (!*ppt) {
                C_debug("Theme '%s' is missing texture '%s'",
                        i_theme.value.s, name);
                filename = C_va("gui/themes/%s/%s.png", i_theme.stock.s, name);
                *ppt = R_texture_load(filename, FALSE);
                if (!*ppt)
                        C_error("Stock texture '%s' is missing", filename);
        }
}

/******************************************************************************\
 Leave limbo mode.
\******************************************************************************/
void I_leave_limbo(void)
{
        i_limbo = FALSE;
        I_widget_show(&right_toolbar.widget, TRUE);
}

/******************************************************************************\
 Enter limbo mode.
\******************************************************************************/
void I_enter_limbo(void)
{
        i_limbo = TRUE;
        I_widget_show(&right_toolbar.widget, FALSE);
}

/******************************************************************************\
 Loads interface assets and initializes windows.
\******************************************************************************/
void I_init(void)
{
        C_status("Initializing interface");

        /* Enable key repeats so held keys generate key-down events */
        SDL_EnableKeyRepeat(KEY_REPEAT_TIMEOUT, KEY_REPEAT_INTERVAL);

        /* Enable Unicode so we get Unicode key strokes AND capitals */
        SDL_EnableUNICODE(1);

        /* Root window */
        C_zero(&i_root);
        I_widget_init(&i_root, "Root Window");
        i_root.state = I_WS_READY;
        i_root.event_func = (i_event_f)root_event;
        i_root.configured = 1;
        i_root.entry = TRUE;
        i_root.shown = TRUE;
        i_root.clickable = TRUE;
        i_key_focus = &i_root;

        /* Left toolbar */
        I_toolbar_init(&left_toolbar, FALSE);
        I_toolbar_add_button(&left_toolbar, "gui/icons/game.png",
                             (i_callback_f)I_game_init);
        I_toolbar_add_button(&left_toolbar, "gui/icons/console.png",
                             (i_callback_f)I_console_init);
        I_toolbar_add_button(&left_toolbar, "gui/icons/video.png",
                             (i_callback_f)I_video_init);
        I_widget_add(&i_root, &left_toolbar.widget);

        /* Right toolbar */
        I_toolbar_init(&right_toolbar, TRUE);
        I_toolbar_add_button(&right_toolbar, "gui/icons/nations.png",
                             (i_callback_f)I_nations_init);
        I_widget_add(&i_root, &right_toolbar.widget);

        /* Theme can now be modified dynamically */
        theme_configure();
        i_theme.update = (c_var_update_f)theme_update;
        i_theme.edit = C_VE_FUNCTION;

        /* Create ring widgets */
        I_init_ring();

        /* Limbo resources */
        R_sprite_load(&limbo_logo, "gui/logo.png");

        /* Configure all widgets */
        I_widget_event(&i_root, I_EV_CONFIGURE);

        /* Start in limbo */
        I_enter_limbo();
        limbo_fade = 1.f;
}

/******************************************************************************\
 Free interface assets.
\******************************************************************************/
void I_cleanup(void)
{
        I_widget_event(&i_root, I_EV_CLEANUP);
        R_sprite_cleanup(&limbo_logo);

        /* We reuse the theme function calls to cleanup their textures by
           setting a special mode */
        cleanup_theme = TRUE;
        theme_widgets();
        cleanup_theme = FALSE;
}

/******************************************************************************\
 Render all visible windows.
\******************************************************************************/
void I_render(void)
{
        /* If video parameters changed last frame, we need to reconfigure */
        if (r_pixel_scale.changed > layout_frame ||
            r_width.changed > layout_frame ||
            r_height.changed > layout_frame) {
                theme_configure();
                I_widget_event(&i_root, I_EV_CONFIGURE);
                layout_frame = c_frame;
        }

        I_widget_event(&i_root, I_EV_RENDER);
}

