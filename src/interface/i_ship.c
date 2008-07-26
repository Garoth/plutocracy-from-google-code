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

/* Number of separators */
#define SEPARATORS (G_CARGO_TYPES / 4 + 1)

/* Cargo widget */
typedef struct cargo {
        i_widget_t widget;
        i_button_t left, right;
        i_label_t label, left_amount, price, right_amount;
} cargo_t;

static cargo_t cargo_widgets[G_CARGO_TYPES];
static i_label_t title;
static i_image_t separators[SEPARATORS];
static i_info_t cargo_info, class_info, owner_info;

/******************************************************************************\
 Cargo widget event function.
\******************************************************************************/
static int cargo_event(cargo_t *cargo, i_event_t event)
{
        float width;

        switch (event) {
        case I_EV_CONFIGURE:

                /* See how wide we need the labels to be to space properly */
                cargo->price.width = R_font_size(R_FONT_GUI, "000g").x /
                                     r_pixel_scale.value.f;
                width = R_font_size(R_FONT_GUI, "00000").x /
                        r_pixel_scale.value.f;
                cargo->left_amount.width = width;
                cargo->right_amount.width = width;

                I_widget_pack(&cargo->widget, I_PACK_H, I_FIT_NONE);
                cargo->widget.size = I_widget_child_bounds(&cargo->widget);
                return FALSE;
        default:
                break;
        }
        return TRUE;
}

/******************************************************************************\
 Left arrow clicked on a cargo widget.
\******************************************************************************/
static void cargo_left_clicked(void)
{
}

/******************************************************************************\
 Right arrow clicked on a cargo widget.
\******************************************************************************/
static void cargo_right_clicked(void)
{
}

/******************************************************************************\
 Initialize a cargo widget.
\******************************************************************************/
static void cargo_init(cargo_t *cargo, const char *name)
{
        C_zero(cargo);
        I_widget_init(&cargo->widget, "Cargo");
        cargo->widget.event_func = (i_event_f)cargo_event;
        cargo->widget.state = I_WS_READY;

        /* Label */
        I_label_init(&cargo->label, name);
        cargo->label.widget.expand = TRUE;
        I_widget_add(&cargo->widget, &cargo->label.widget);

        /* Left amount */
        I_label_init(&cargo->left_amount, "0");
        cargo->left_amount.width = 40.f;
        cargo->left_amount.color = I_COLOR_ALT;
        I_widget_add(&cargo->widget, &cargo->left_amount.widget);

        /* Left button */
        I_button_init(&cargo->left, "gui/icons/arrow-left.png", NULL,
                      I_BT_ROUND);
        cargo->left.on_click = (i_callback_f)cargo_left_clicked;
        cargo->left.data = cargo;
        cargo->left.widget.margin_front = 0.5f;
        cargo->left.widget.margin_rear = 0.5f;
        I_widget_add(&cargo->widget, &cargo->left.widget);

        /* Price */
        I_label_init(&cargo->price, "0g");
        cargo->price.width = 48.f;
        cargo->price.color = I_COLOR_ALT;
        I_widget_add(&cargo->widget, &cargo->price.widget);

        /* Right button */
        I_button_init(&cargo->right, "gui/icons/arrow-right.png", NULL,
                      I_BT_ROUND);
        cargo->right.on_click = (i_callback_f)cargo_right_clicked;
        cargo->right.data = cargo;
        cargo->right.widget.margin_front = 0.5f;
        cargo->right.widget.margin_rear = 0.5f;
        I_widget_add(&cargo->widget, &cargo->right.widget);

        /* Right amount */
        I_label_init(&cargo->right_amount, "0");
        cargo->right_amount.width = 40.f;
        cargo->right_amount.color = I_COLOR_ALT;
        I_widget_add(&cargo->widget, &cargo->right_amount.widget);
}

/******************************************************************************\
 Configures a cargo widget for trading.
\******************************************************************************/
static void cargo_trading(cargo_t *cargo, int left, int price, int right,
                          bool trading)
{
        i_widget_state_t state;

        state = trading ? I_WS_READY : I_WS_DISABLED;
        cargo->left.widget.state = state;
        cargo->right.widget.state = state;
        cargo->right_amount.widget.state = state;
        cargo->price.widget.state = state;

        /* Set labels */
        I_label_configure(&cargo->left_amount, C_va("%d", left));
        I_label_configure(&cargo->right_amount, C_va("%d", right));
        I_label_configure(&cargo->price, C_va("%dg", price));

        /* Disable the labels if not carrying and can't buy that good */
        if (left < 1 && (right < 1 || !trading)) {
                cargo->left_amount.widget.state = I_WS_DISABLED;
                cargo->label.widget.state = I_WS_DISABLED;
        } else {
                cargo->left_amount.widget.state = I_WS_READY;
                cargo->label.widget.state = I_WS_READY;
        }
}

/******************************************************************************\
 Deselect a ship.
\******************************************************************************/
void I_deselect_ship(void)
{
        I_toolbar_enable(&i_right_toolbar, i_ship_button, FALSE);
}

/******************************************************************************\
 Selected a ship.
\******************************************************************************/
void I_select_ship(const g_cargo_t *cargo, i_color_t color, const char *name,
                   const char *owner, const char *class_name)
{
        int i;

        I_toolbar_enable(&i_right_toolbar, i_ship_button, TRUE);

        /* Title */
        I_label_configure(&title, C_va("%s:", name));

        /* Infos */
        owner_info.right.color = color;
        I_info_configure(&owner_info, owner);
        I_info_configure(&class_info, class_name);
        I_info_configure(&cargo_info,
                         C_va("%d/%d", G_cargo_space(cargo), cargo->capacity));

        /* Not trading */
        for (i = 0; i < G_CARGO_TYPES; i++)
                cargo_trading(cargo_widgets + i, cargo->amounts[i],
                              0, 0, FALSE);
}

/******************************************************************************\
 Initialize the ship window.
\******************************************************************************/
void I_init_ship(i_window_t *window)
{
        int i, seps;

        I_window_init(window);
        window->natural_size = C_vec2(275.f, 0.f);
        window->fit = I_FIT_TOP;

        /* Label */
        I_label_init(&title, "Unnamed:");
        title.width = window->natural_size.x - i_border.value.n;
        title.font = R_FONT_TITLE;
        title.center = FALSE;
        I_widget_add(&window->widget, &title.widget);

        /* Owner */
        I_info_init(&owner_info, C_str("i-cargo", "Owner:"), "Unowned");
        I_widget_add(&window->widget, &owner_info.widget);

        /* Ship class */
        I_info_init(&class_info, C_str("i-cargo", "Ship class:"), "Unknown");
        I_widget_add(&window->widget, &class_info.widget);

        /* Cargo space */
        I_info_init(&cargo_info, C_str("i-cargo", "Cargo space:"), "0/0");
        I_widget_add(&window->widget, &cargo_info.widget);

        /* Top separator */
        I_image_init_sep(separators);
        separators[0].resize = TRUE;
        I_widget_add(&window->widget, &separators[0].widget);

        /* Cargo items */
        for (i = 0, seps = 1; i < G_CARGO_TYPES; i++) {
                cargo_init(cargo_widgets + i, C_va("%s:", g_cargo_names[i]));
                I_widget_add(&window->widget, &cargo_widgets[i].widget);

                /* Add separator */
                if ((i - 1) % 4 || i >= G_CARGO_TYPES - 1)
                        continue;
                C_assert(seps < SEPARATORS);
                I_image_init_sep(separators + seps);
                separators[seps].resize = TRUE;
                I_widget_add(&window->widget, &separators[seps++].widget);
        }
}

