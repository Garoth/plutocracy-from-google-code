/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Implements ring menus, callable from within the game namespace */

#include "i_common.h"

/* Ring radius cannot be accurately determined from the graphic alone */
#define RING_OUTER_RADIUS 48.f
#define RING_INNER_RADIUS 17.f
#define RING_ICON_SIZE 32.f

static i_widget_t ring_widget;
static r_sprite_t ring_sprite;
static c_vec3_t world_origin;
static c_vec2_t screen_origin;
static i_ring_f callback;
static i_button_t button_widgets[I_RING_ICONS];
static int buttons;

/******************************************************************************\
 Position a ring on its world origin.
\******************************************************************************/
static void position_and_pack(void)
{
        c_vec2_t origin, size;
        float angle;
        int i, j;

        /* Position the ring */
        screen_origin = C_vec2_from_3(R_project_by_cam(world_origin), 2);
        size = C_vec2_scalef(ring_widget.size, 0.5f);
        I_widget_move(&ring_widget, C_vec2_sub(screen_origin, size));

        /* Pack the visible buttons */
        for (j = i = 0; i < I_RING_ICONS; i++) {
                if (!button_widgets[i].widget.shown)
                        continue;
                angle = 2.f * C_PI * ((float)j++ / buttons - 0.25f);
                origin = C_vec2_scalef(C_vec2(cosf(angle), sinf(angle)),
                                       RING_INNER_RADIUS +
                                       RING_ICON_SIZE / 2.f);
                origin = C_vec2_add(screen_origin, origin);
                origin = C_vec2(origin.x - RING_ICON_SIZE / 2.f,
                                origin.y - RING_ICON_SIZE / 2.f);
                I_widget_move(&button_widgets[i].widget, origin);
        }
}

/******************************************************************************\
 Ring widget event function.
\******************************************************************************/
static int ring_event(i_widget_t *widget, i_event_t event)
{
        float radius;

        /* Calculate mouse radius for mouse events */
        if (event == I_EV_MOUSE_MOVE || event == I_EV_MOUSE_DOWN)
                radius = C_vec2_len(C_vec2_sub(C_vec2(i_mouse_x, i_mouse_y),
                                               screen_origin));

        switch (event) {
        case I_EV_CONFIGURE:
                R_sprite_cleanup(&ring_sprite);
                R_sprite_init(&ring_sprite, i_ring.value.s);
                ring_widget.size = ring_sprite.size;
                I_widget_propagate(widget, I_EV_CONFIGURE);
                position_and_pack();
                return FALSE;
        case I_EV_MOVED:
                ring_sprite.origin = ring_widget.origin;
                break;
        case I_EV_MOUSE_OUT:
                I_close_ring();
                break;
        case I_EV_MOUSE_MOVE:
                if (radius > RING_OUTER_RADIUS)
                        I_close_ring();
                break;
        case I_EV_MOUSE_DOWN:
                if (radius <= RING_INNER_RADIUS)
                        I_close_ring();
                break;
        case I_EV_CLEANUP:
                R_sprite_cleanup(&ring_sprite);
                break;
        case I_EV_RENDER:
                ring_sprite.modulate.a = ring_widget.fade;
                R_sprite_render(&ring_sprite);
                break;
        default:
                break;
        }
        return TRUE;
}

/******************************************************************************\
 Called when a ring button is clicked.
\******************************************************************************/
static void button_clicked(i_button_t *button)
{
        I_close_ring();
        if (!callback)
                return;
        callback((i_ring_icon_t)(button - button_widgets));
}

/******************************************************************************\
 Initialize ring widgets and attach them to the root window.
\******************************************************************************/
void I_init_ring(void)
{
        int i;

        I_widget_set_name(&ring_widget, "Ring");
        ring_widget.event_func = (i_event_f)ring_event;
        ring_widget.state = I_WS_READY;
        ring_widget.clickable = TRUE;
        I_widget_inited(&ring_widget);

        /* Initialize the button widgets */
        I_button_init(button_widgets + I_RI_TEST_BLANK,
                      "gui/icons/ring/test_blank.png", NULL, I_BT_ROUND);
        I_button_init(button_widgets + I_RI_TEST_MILL,
                      "gui/icons/ring/test_mill.png", NULL, I_BT_ROUND);
        I_button_init(button_widgets + I_RI_TEST_TREE,
                      "gui/icons/ring/test_tree.png", NULL, I_BT_ROUND);
        I_button_init(button_widgets + I_RI_TEST_DISABLED,
                      "gui/icons/ring/test_disabled.png", NULL, I_BT_ROUND);
        for (i = 0; i < I_RING_ICONS; i++) {
                I_widget_add(&ring_widget, &button_widgets[i].widget);
                button_widgets[i].on_click = (i_callback_f)button_clicked;
        }

        I_widget_add(&i_root, &ring_widget);
}

/******************************************************************************\
 Show the ring UI centered on a world-space [origin].
\******************************************************************************/
void I_show_ring(c_vec3_t origin, i_ring_f _callback)
{
        world_origin = origin;
        position_and_pack();
        I_widget_show(&ring_widget, TRUE);
        callback = _callback;
}

/******************************************************************************\
 Close the open ring.
\******************************************************************************/
void I_close_ring(void)
{
        I_widget_show(&ring_widget, FALSE);
}

/******************************************************************************\
 Reset ring contents.
\******************************************************************************/
void I_reset_ring(void)
{
        int i;

        for (i = 0; i < I_RING_ICONS; i++) {
                I_widget_show(&button_widgets[i].widget, FALSE);
                button_widgets[i].widget.fade = 0.f;
        }
        buttons = 0;
}

/******************************************************************************\
 Add a button to the ring.
\******************************************************************************/
void I_add_to_ring(i_ring_icon_t icon, int enabled)
{
        C_assert(icon >= 0 && icon < I_RING_ICONS);
        if (button_widgets[icon].widget.shown)
                return;
        I_widget_show(&button_widgets[icon].widget, TRUE);
        button_widgets[icon].widget.state = enabled ? I_WS_READY :
                                                      I_WS_DISABLED;
        buttons++;
}

