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

/* Cargo widget */
typedef struct cargo {
        i_selectable_t sel;
        i_label_t left, label, price, right;
} cargo_t;

static cargo_t cargo_widgets[G_CARGO_TYPES];
static i_label_t title;
static i_info_t cargo_info;
static i_select_t mode, quantity, price, active;
static i_selectable_t *cargo_group;
static i_box_t button_box;
static i_button_t transfer_button, ten_button, fifty_button;

/******************************************************************************\
 Initialize a cargo widget.
\******************************************************************************/
static void cargo_init(cargo_t *cargo, const char *name)
{
        I_selectable_init(&cargo->sel, &cargo_group, 0.f);

        /* Left amount */
        I_label_init(&cargo->left, "99999");
        cargo->left.widget.expand = TRUE;
        cargo->left.color = I_COLOR_ALT;
        I_widget_add(&cargo->sel.widget, &cargo->left.widget);

        /* Label */
        I_label_init(&cargo->label, name);
        I_widget_add(&cargo->sel.widget, &cargo->label.widget);

        /* Price */
        I_label_init(&cargo->price, "999g");
        cargo->price.color = I_COLOR_ALT;
        I_widget_add(&cargo->sel.widget, &cargo->price.widget);

        /* Right amount */
        I_label_init(&cargo->right, "99999");
        cargo->right.widget.expand = TRUE;
        cargo->right.color = I_COLOR_ALT;
        cargo->right.justify = I_JUSTIFY_RIGHT;
        I_widget_add(&cargo->sel.widget, &cargo->right.widget);
}

/******************************************************************************\
 Selected something that trades cargo. If [cargo] is NULL, disables the
 trade window.
\******************************************************************************/
void I_select_trade(const g_cargo_t *cargo)
{
        /* Deselect cargo */
        if (!cargo) {
                I_toolbar_enable(&i_right_toolbar, i_trade_button, FALSE);
                return;
        }

        I_toolbar_enable(&i_right_toolbar, i_trade_button, TRUE);
        I_info_configure(&cargo_info,
                         C_va("%d/%d", G_cargo_space(cargo), cargo->capacity));
}

/******************************************************************************\
 Initialize trade window.
\******************************************************************************/
void I_init_trade(i_window_t *window)
{
        int i, seps;

        I_window_init(window);
        window->widget.size = C_vec2(240.f, 0.f);
        window->fit = I_FIT_TOP;

        /* Label */
        I_label_init(&title, "Trade");
        title.font = R_FONT_TITLE;
        I_widget_add(&window->widget, &title.widget);

        /* Trade mode */
        I_select_init(&mode, C_str("i-cargo-mode", "Window mode:"), NULL);
        I_select_add_string(&mode, C_str("i-sell", "Sell"));
        I_select_add_string(&mode, C_str("i-buy", "Buy"));
        I_widget_add(&window->widget, &mode.widget);

        /* Cargo space */
        I_info_init(&cargo_info, C_str("i-cargo", "Cargo space:"), "0/0");
        cargo_info.widget.margin_rear = 0.5f;
        I_widget_add(&window->widget, &cargo_info.widget);

        /* Cargo items */
        for (i = 0, seps = 1; i < G_CARGO_TYPES; i++) {
                cargo_init(cargo_widgets + i, g_cargo_names[i]);
                I_widget_add(&window->widget, &cargo_widgets[i].sel.widget);
                if (!((i - 2) % 4))
                        cargo_widgets[i].sel.widget.margin_front = 0.5f;
        }

        /* Selling, buying, both, or neither */
        I_select_init(&active, C_str("i-cargo-auto-buy", "Auto-buy:"), NULL);
        I_select_add_string(&active, C_str("i-yes", "Yes"));
        I_select_add_string(&active, C_str("i-no", "No"));
        active.widget.margin_front = 0.5f;
        active.decimals = 0;
        I_widget_add(&window->widget, &active.widget);

        /* Quantity */
        I_select_init(&quantity, C_str("i-cargo-maximum", "Maximum:"), NULL);
        quantity.min = 0;
        quantity.max = 100;
        quantity.decimals = 0;
        I_widget_add(&window->widget, &quantity.widget);

        /* Selling, buying, both, or neither */
        I_select_init(&price, C_str("i-cargo-price", "Price:"), NULL);
        price.min = 0;
        price.max = 999;
        price.suffix = "g";
        price.decimals = 0;
        I_widget_add(&window->widget, &price.widget);

        /* Button box */
        I_box_init(&button_box, I_PACK_H, 0.f);
        I_widget_add(&window->widget, &button_box.widget);

        /* Buy, sell, and transfer buttons */
        I_button_init(&transfer_button, NULL, C_str("i-cargo-buy", "Buy"),
                      I_BT_DECORATED);
        transfer_button.widget.expand = 4;
        I_widget_add(&button_box.widget, &transfer_button.widget);
        button_box.widget.margin_front = 0.5f;
        I_button_init(&ten_button, NULL, "10", I_BT_DECORATED);
        ten_button.widget.expand = TRUE;
        I_widget_add(&button_box.widget, &ten_button.widget);
        I_button_init(&fifty_button, NULL, "50", I_BT_DECORATED);
        fifty_button.widget.expand = TRUE;
        I_widget_add(&button_box.widget, &fifty_button.widget);
}

