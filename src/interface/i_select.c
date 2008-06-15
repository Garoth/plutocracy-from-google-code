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

/******************************************************************************\
 Selection widget event function.
\******************************************************************************/
int I_select_event(i_select_t *select, i_event_t event)
{
        if (event == I_EV_CONFIGURE) {
                float width;
                int i;

                /* Find the length of the longest option */
                if (select->list) {
                        for (i = 0, width = 0.f; select->list[i]; i++) {
                                c_vec2_t size;

                                size = R_font_size(R_FONT_GUI, select->list[i]);
                                size = C_vec2_divf(size, r_pixel_scale.value.f);
                                if (size.x > width)
                                        width = size.x;
                        }
                        select->item.width = width;
                        select->list_len = i;
                }

                select->widget.size.y = R_font_height(R_FONT_GUI) /
                                        r_pixel_scale.value.f;
                I_widget_pack(&select->widget, I_PACK_H, I_FIT_NONE);
                select->widget.size = I_widget_child_bounds(&select->widget);
                return FALSE;
        }
        return TRUE;
}

/******************************************************************************\
 Selection changed.
\******************************************************************************/
static void select_change(i_select_t *select, int index)
{
        if (index <= 0) {
                index = 0;
                select->left.widget.state = I_WS_DISABLED;
        } else if (select->left.widget.state == I_WS_DISABLED)
                select->left.widget.state = I_WS_READY;
        if (index >= select->list_len - 1) {
                index = select->list_len - 1;
                select->right.widget.state = I_WS_DISABLED;
        } else if (select->right.widget.state == I_WS_DISABLED)
                select->right.widget.state = I_WS_READY;
        I_label_configure(&select->item, select->list[index]);
        select->index = index;
        if (select->on_change)
                select->on_change(select);
}

/******************************************************************************\
 Left select button clicked.
\******************************************************************************/
static void left_arrow_clicked(i_button_t *button)
{
        i_select_t *select;

        select = (i_select_t *)button->data;
        select_change(select, --select->index);
}

/******************************************************************************\
 Right select button clicked.
\******************************************************************************/
static void right_arrow_clicked(i_button_t *button)
{
        i_select_t *select;

        select = (i_select_t *)button->data;
        select_change(select, ++select->index);
}

/******************************************************************************\
 Initialize a selection widget.
\******************************************************************************/
void I_select_init(i_select_t *select, const char *label, const char **list,
                   int initial)
{
        if (!select)
                return;
        C_zero(select);
        I_widget_set_name(&select->widget, "Select");
        select->widget.event_func = (i_event_f)I_select_event;
        select->widget.clickable = TRUE;
        select->widget.state = I_WS_READY;
        select->list = list;
        select->index = initial;

        /* Description label */
        I_label_init(&select->label, label);
        select->label.widget.expand = TRUE;
        select->label.widget.margin_rear = 0.5f;
        I_widget_add(&select->widget, &select->label.widget);

        /* Don't add buttons or item list if there is no list */
        if (!list || !list[0]) {
                I_widget_inited(&select->widget);
                return;
        }

        /* Left button */
        I_button_init(&select->left, "gui/icons/arrow-left.png", NULL,
                      I_BT_ICON_ROUND);
        select->left.on_click = (i_callback_f)left_arrow_clicked;
        select->left.data = select;
        select->left.widget.margin_rear = 0.5f;
        if (initial < 1)
                select->left.widget.state = I_WS_DISABLED;
        I_widget_add(&select->widget, &select->left.widget);

        /* Selected item label */
        I_label_init(&select->item, list[initial]);
        select->item.color = I_COLOR_ALT;
        I_widget_add(&select->widget, &select->item.widget);

        /* Find the length of the options list */
        for (select->list_len = 0; list[select->list_len]; select->list_len++);

        /* Right button */
        I_button_init(&select->right, "gui/icons/arrow-right.png", NULL,
                      I_BT_ICON_ROUND);
        select->right.on_click = (i_callback_f)right_arrow_clicked;
        select->right.data = select;
        select->right.widget.margin_front = 0.5f;
        if (initial >= select->list_len - 1)
                select->right.widget.state = I_WS_DISABLED;
        I_widget_add(&select->widget, &select->right.widget);

        I_widget_inited(&select->widget);
}

