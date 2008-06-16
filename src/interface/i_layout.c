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

/* Number of windows defined */
#define WINDOWS_LEN 3

/* Globe light modulation in limbo mode */
#define LIMBO_GLOBE_LIGHT 0.1f

/* The logo fades in/out during limbo mode changes, but not as fast as the
   other windows and GUI elements */
#define LIMBO_FADE_SCALE 0.2f

/* SDL key-repeat initial timeout and repeat interval */
#define KEY_REPEAT_TIMEOUT 300
#define KEY_REPEAT_INTERVAL 30

/* Windows */
static struct property_t {
        void (*init)(i_window_t *);
        char *icon;
        c_vec2_t size;
} properties[] = {
        {I_game_init, "gui/icons/game.png", {200.f, 0.f}},
        {I_console_init, "gui/icons/console.png", {480.f, 240.f}},
        {I_video_init, "gui/icons/video.png", {260.f, 0.f}},
};

/* Root widget contains all other widgets */
i_widget_t i_root;

/* TRUE if the interface is in limbo mode */
int i_limbo;

static i_window_t left_toolbar, *open_window, windows[WINDOWS_LEN];
static i_button_t buttons[WINDOWS_LEN];
static r_sprite_t limbo_logo;
static float limbo_fade;
static int layout_frame;

/******************************************************************************\
 Root window event function.
\******************************************************************************/
static int root_event(i_widget_t *root, i_event_t event)
{
        int i;

        switch (event) {
        case I_EV_CONFIGURE:
                i_colors[I_COLOR] = C_color_string(i_color.value.s);
                i_colors[I_COLOR_ALT] = C_color_string(i_color2.value.s);
                I_widget_propagate(root, event);

                /* Position window hangers */
                for (i = 0; i < WINDOWS_LEN; i++)
                        I_window_hanger(windows + i, &buttons[i].widget, TRUE);

                /* Size and position limbo logo */
                limbo_logo.origin.x = r_width_2d / 2 - limbo_logo.size.x / 2;
                limbo_logo.origin.y = r_height_2d / 2 - limbo_logo.size.y / 2;

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
 Updates after a theme change.
\******************************************************************************/
static void theme_configure(void)
{
        float offset;
        int i;

        C_var_unlatch(&i_border);
        C_var_unlatch(&i_button);
        C_var_unlatch(&i_button_active);
        C_var_unlatch(&i_button_hover);
        C_var_unlatch(&i_color);
        C_var_unlatch(&i_color2);
        C_var_unlatch(&i_hanger);
        C_var_unlatch(&i_ring);
        C_var_unlatch(&i_round_active);
        C_var_unlatch(&i_round_hover);
        C_var_unlatch(&i_shadow);
        C_var_unlatch(&i_square_active);
        C_var_unlatch(&i_square_hover);
        C_var_unlatch(&i_window);
        C_var_unlatch(&i_work_area);

        /* Root window */
        i_root.size = C_vec2((float)r_width_2d, (float)r_height_2d);

        /* Left toolbar */
        left_toolbar.widget.size = C_vec2(0.f, 32.f + i_border.value.n * 2);
        left_toolbar.widget.origin = C_vec2((float)i_border.value.n,
                                            r_height_2d -
                                            left_toolbar.widget.size.y -
                                            i_border.value.n);

        /* Windows */
        offset = r_height_2d - left_toolbar.widget.size.y -
                 i_border.value.n * 2;
        for (i = 0; i < WINDOWS_LEN; i++) {
                windows[i].widget.size = properties[i].size;
                windows[i].widget.origin = C_vec2((float)i_border.value.n,
                                                  offset -
                                                  windows[i].widget.size.y);
        }
}

/******************************************************************************\
 Update function for i_theme. Will also reload fonts.
\******************************************************************************/
static int theme_update(c_var_t *var, c_var_value_t value)
{
        if (!C_parse_config_file(value.s))
                return FALSE;
        theme_configure();
        R_free_fonts();
        R_load_fonts();
        I_widget_event(&i_root, I_EV_CONFIGURE);
        return TRUE;
}

/******************************************************************************\
 Left toolbar button handlers.
\******************************************************************************/
static void left_toolbar_open(i_window_t *window)
{
        if (open_window && open_window->widget.shown) {
                I_widget_show(&open_window->widget, FALSE);
                if (open_window == window) {
                        open_window = NULL;
                        return;
                }
        }
        I_widget_show(&window->widget, TRUE);
        open_window = window;
}

static void button_click(i_button_t *button)
{
        left_toolbar_open(button->data);
}

/******************************************************************************\
 Parse the theme config.
\******************************************************************************/
void I_parse_config(void)
{
        C_var_unlatch(&i_theme);
        C_parse_config_file(i_theme.value.s);
}

/******************************************************************************\
 Leave limbo mode.
\******************************************************************************/
void I_leave_limbo(void)
{
        i_limbo = FALSE;
}

/******************************************************************************\
 Enter limbo mode.
\******************************************************************************/
void I_enter_limbo(void)
{
        i_limbo = TRUE;
}

/******************************************************************************\
 Loads interface assets and initializes windows.
\******************************************************************************/
void I_init(void)
{
        int i;

        C_status("Initializing interface");

        /* Enable key repeats so held keys generate key-down events */
        SDL_EnableKeyRepeat(KEY_REPEAT_TIMEOUT, KEY_REPEAT_INTERVAL);

        /* Enable Unicode so we get Unicode key strokes AND capitals */
        SDL_EnableUNICODE(1);

        /* Root window */
        C_zero(&i_root);
        I_widget_set_name(&i_root, "Root Window");
        i_root.state = I_WS_READY;
        i_root.event_func = (i_event_f)root_event;
        i_root.configured = 1;
        i_root.entry = TRUE;
        i_root.shown = TRUE;
        i_root.clickable = TRUE;
        i_key_focus = &i_root;

        /* Left toolbar */
        I_window_init(&left_toolbar);
        left_toolbar.fit = I_FIT_BOTTOM;
        left_toolbar.pack_children = I_PACK_H;
        I_widget_add(&i_root, &left_toolbar.widget);

        /* Window and buttons */
        for (i = 0; i < WINDOWS_LEN; i++) {
                properties[i].init(windows + i);
                I_widget_add(&i_root, &windows[i].widget);
                I_button_init(buttons + i, properties[i].icon, NULL,
                              I_BT_SQUARE);
                buttons[i].on_click = (i_callback_f)button_click;
                buttons[i].data = windows + i;
                if (i < WINDOWS_LEN - 1)
                        buttons[i].widget.margin_rear = 0.5f;
                I_widget_add(&left_toolbar.widget, &buttons[i].widget);
        }

        /* Theme can now be modified dynamically */
        theme_configure();
        i_theme.update = (c_var_update_f)theme_update;
        i_theme.edit = C_VE_FUNCTION;

        /* Start in limbo */
        R_sprite_init(&limbo_logo, "gui/logo.png");
        limbo_fade = 1.f;
        I_enter_limbo();

        /* Create ring widgets */
        I_init_ring();

        I_widget_event(&i_root, I_EV_CONFIGURE);
        I_widget_show(&left_toolbar.widget, TRUE);
}

/******************************************************************************\
 Free interface assets.
\******************************************************************************/
void I_cleanup(void)
{
        I_widget_event(&i_root, I_EV_CLEANUP);
        R_sprite_cleanup(&limbo_logo);
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

