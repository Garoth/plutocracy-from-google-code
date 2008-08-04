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

/* Modes */
#define MODE_BUY 0
#define MODE_SELL 1

/* Cargo widget */
typedef struct cargo_line {
        i_selectable_t sel;
        i_label_t left, label, price, right;
        i_cargo_data_t left_data, right_data;
} cargo_line_t;

static cargo_line_t cargo_lines[G_CARGO_TYPES];
static i_label_t title;
static i_info_t cargo_info;
static i_select_t mode, quantity, price, active;
static i_selectable_t *cargo_group;
static i_box_t button_box;
static i_button_t transfer_button, ten_button, fifty_button;
static int cargo_space;
static bool left_own, configuring;

/******************************************************************************\
 Returns TRUE if a transfer is possible.
\******************************************************************************/
static bool can_transfer(cargo_line_t *cargo, int amount)
{
        int gold;

        gold = cargo_lines[G_CT_GOLD].left_data.amount;
        if (mode.index == MODE_BUY)
                return cargo_space >= amount &&
                       cargo->right_data.minimum >= amount &&
                       gold >= amount * cargo->right_data.sell_price;
        return cargo->right_data.maximum >= amount &&
               cargo->left_data.amount >= amount;
}

/******************************************************************************\
 Configures the control widgets.
\******************************************************************************/
static void configure_controls(cargo_line_t *cargo)
{
        i_widget_state_t state;
        bool enable;

        /* Whether to enable the control widgets */
        enable = cargo && left_own;

        /* Enable/disable control widgets */
        state = enable ? I_WS_READY : I_WS_DISABLED;
        price.widget.state = state;
        quantity.widget.state = state;
        active.widget.state = state;
        transfer_button.widget.state = state;
        ten_button.widget.state = state;
        fifty_button.widget.state = state;

        /* Show/hide price/quantity indicators */
        quantity.item.widget.shown = enable;
        price.item.widget.shown = enable;
        active.item.widget.shown = enable;

        /* Set the amounts */
        if (!enable)
                return;
        configuring = TRUE;
        if (mode.index == MODE_BUY) {
                I_select_change(&active, cargo->left_data.auto_buy);
                I_select_nearest(&quantity, cargo->left_data.maximum);
                I_select_nearest(&price, cargo->left_data.buy_price);
        } else if (mode.index == MODE_SELL) {
                I_select_change(&active, cargo->left_data.auto_sell);
                I_select_nearest(&quantity, cargo->left_data.minimum);
                I_select_nearest(&price, cargo->left_data.sell_price);

        }
        configuring = FALSE;

        /* Make sure transfer buttons are valid quantities */
        if (!can_transfer(cargo, 50))
                fifty_button.widget.state = I_WS_DISABLED;
        if (!can_transfer(cargo, 10))
                ten_button.widget.state = I_WS_DISABLED;
        if (!can_transfer(cargo, 1))
                transfer_button.widget.state = I_WS_DISABLED;
}

/******************************************************************************\
 Configures the control widgets for the currently selected cargo line.
\******************************************************************************/
static void configure_selected(void)
{
        int i;

        for (i = 0; i < G_CARGO_TYPES; i++)
                if (cargo_group == &cargo_lines[i].sel) {
                        configure_controls(cargo_lines + i);
                        break;
                }
}

/******************************************************************************\
 Enable or disable the trade window.
\******************************************************************************/
void I_enable_trade(bool enable, bool own)
{
        I_toolbar_enable(&i_right_toolbar, i_trade_button, enable);
        left_own = own;
}

/******************************************************************************\
 Configures store cargo space.
\******************************************************************************/
void I_set_cargo_space(int used, int capacity)
{
        I_info_configure(&cargo_info, C_va("%d/%d", used, capacity));
        cargo_space = used;
}

/******************************************************************************\
 Configures a cargo line for the current window mode.
\******************************************************************************/
static void cargo_line_configure(cargo_line_t *cargo)
{
        int value;

        /* Left amount */
        if ((cargo->left.widget.shown = cargo->left_data.amount >= 0))
                I_label_configure(&cargo->left,
                                  C_va("%d", cargo->left_data.amount));

        /* Right amount */
        value = cargo->right_data.amount;
        if (value < 0)
                value = mode.index == MODE_BUY ? cargo->right_data.minimum :
                                                 cargo->right_data.maximum;
        if (value > 0)
                I_label_configure(&cargo->right, C_va("%d", value));
        else
                cargo->right.widget.shown = FALSE;

        /* In-line price */
        value = mode.index == MODE_BUY ? cargo->right_data.sell_price :
                                         cargo->right_data.buy_price;
        if ((cargo->price.widget.shown = value > 0))
                I_label_configure(&cargo->price, C_va("%dg", value));

        /* Control widgets */
        if (cargo_group == &cargo->sel)
                configure_controls(cargo);
}

/******************************************************************************\
 Configures a cargo line on the trade window.
\******************************************************************************/
void I_configure_cargo(int i, const i_cargo_data_t *left,
                       const i_cargo_data_t *right)
{
        C_assert(i >= 0 && i < G_CARGO_TYPES);
        if (left)
                cargo_lines[i].left_data = *left;
        else
                cargo_lines[i].left_data.amount = -1;
        if (right)
                cargo_lines[i].right_data = *right;
        else {
                cargo_lines[i].right_data.amount = -1;
                cargo_lines[i].right_data.minimum = -1;
                cargo_lines[i].right_data.maximum = -1;
                cargo_lines[i].right_data.buy_price = -1;
                cargo_lines[i].right_data.sell_price = -1;
        }
        cargo_line_configure(cargo_lines + i);
}

/******************************************************************************\
 Updates trade control parameters.
\******************************************************************************/
static void controls_changed(void)
{
        cargo_line_t *cargo;
        int buy_price, sell_price;

        /* User didn't actually change the select widget */
        if (configuring || !left_own)
                return;

        C_assert(cargo_group);
        cargo = (cargo_line_t *)cargo_group;

        /* Update our stored values */
        if (mode.index == MODE_BUY) {
                cargo->left_data.auto_buy = active.index;
                cargo->left_data.buy_price = I_select_value(&price);
                cargo->left_data.maximum = I_select_value(&quantity);
        } else if (mode.index == MODE_SELL) {
                cargo->left_data.auto_sell = active.index;
                cargo->left_data.sell_price = I_select_value(&price);
                cargo->left_data.minimum = I_select_value(&quantity);
        }

        /* Pass update back to the game namespace */
        sell_price = buy_price = -1;
        if (cargo->left_data.auto_buy)
                buy_price = cargo->left_data.buy_price;
        if (cargo->left_data.auto_sell)
                sell_price = cargo->left_data.sell_price;
        G_trade_params(cargo - cargo_lines, buy_price, sell_price,
                       cargo->left_data.minimum, cargo->left_data.maximum);
}

/******************************************************************************\
 Updates when the window mode changes.
\******************************************************************************/
static void mode_changed(void)
{
        if (mode.index == MODE_BUY) {
                I_label_configure(&active.label, "Auto-buy:");
                I_label_configure(&quantity.label, "Maximum:");
                I_button_configure(&transfer_button, NULL, "Buy",
                                   I_BT_DECORATED);
        } else if (mode.index == MODE_SELL) {
                I_label_configure(&active.label, "Auto-sell:");
                I_label_configure(&quantity.label, "Minimum:");
                I_button_configure(&transfer_button, NULL, "Sell",
                                   I_BT_DECORATED);
        }
        configure_selected();
}

/******************************************************************************\
 Initialize a cargo widget.
\******************************************************************************/
static void cargo_line_init(cargo_line_t *cargo, const char *name)
{
        I_selectable_init(&cargo->sel, &cargo_group, 0.f);
        cargo->sel.on_select = (i_callback_f)configure_selected;

        /* Left amount */
        I_label_init(&cargo->left, "99999 ");
        cargo->left.color = I_COLOR_ALT;
        I_widget_add(&cargo->sel.widget, &cargo->left.widget);

        /* Label */
        I_label_init(&cargo->label, name);
        cargo->label.widget.expand = TRUE;
        I_widget_add(&cargo->sel.widget, &cargo->label.widget);

        /* Price */
        I_label_init(&cargo->price, " 999g");
        cargo->price.widget.expand = TRUE;
        cargo->price.color = I_COLOR_ALT;
        I_widget_add(&cargo->sel.widget, &cargo->price.widget);

        /* Right amount */
        I_label_init(&cargo->right, " 99999");
        cargo->right.color = I_COLOR_ALT;
        cargo->right.justify = I_JUSTIFY_RIGHT;
        I_widget_add(&cargo->sel.widget, &cargo->right.widget);
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
        I_select_change(&mode, 0);
        mode.on_change = (i_callback_f)mode_changed;
        I_widget_add(&window->widget, &mode.widget);

        /* Cargo space */
        I_info_init(&cargo_info, C_str("i-cargo", "Cargo space:"), "0/0");
        cargo_info.widget.margin_rear = 0.5f;
        I_widget_add(&window->widget, &cargo_info.widget);

        /* Cargo items */
        cargo_group = &cargo_lines[0].sel;
        for (i = 0, seps = 1; i < G_CARGO_TYPES; i++) {
                cargo_line_init(cargo_lines + i, g_cargo_names[i]);
                I_widget_add(&window->widget, &cargo_lines[i].sel.widget);
                if (!((i - 2) % 4))
                        cargo_lines[i].sel.widget.margin_front = 0.5f;
        }

        /* Selling, buying, both, or neither */
        I_select_init(&active, C_str("i-cargo-auto-buy", "Auto-buy:"), NULL);
        I_select_add_string(&active, C_str("i-yes", "Yes"));
        I_select_add_string(&active, C_str("i-no", "No"));
        active.widget.margin_front = 0.5f;
        active.decimals = 0;
        active.on_change = (i_callback_f)controls_changed;
        I_widget_add(&window->widget, &active.widget);

        /* Quantity */
        I_select_init(&quantity, C_str("i-cargo-maximum", "Maximum:"), NULL);
        quantity.min = 0;
        quantity.max = 100;
        quantity.decimals = 0;
        quantity.on_change = (i_callback_f)controls_changed;
        I_widget_add(&window->widget, &quantity.widget);

        /* Selling, buying, both, or neither */
        I_select_init(&price, C_str("i-cargo-price", "Price:"), NULL);
        price.min = 0;
        price.max = 999;
        price.suffix = "g";
        price.decimals = 0;
        price.on_change = (i_callback_f)controls_changed;
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

