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
 Label widget event function.
\******************************************************************************/
int I_label_event(i_label_t *label, i_event_t event)
{
        switch (event) {
        case I_EV_CONFIGURE:
                if (label->width > 0.f)
                        label->widget.size.x = label->width;
                R_sprite_cleanup(&label->text);
                R_sprite_init_text(&label->text, label->font,
                                   label->widget.size.x, i_shadow.value.f,
                                   FALSE, label->buffer);
                label->widget.size.y = label->text.size.y;
                if (label->width <= 0.f)
                        label->widget.size.x = label->text.size.x;
                label->text.modulate = i_colors[label->color];
        case I_EV_MOVED:
                label->text.origin = C_vec2_clamp(label->widget.origin,
                                                  r_pixel_scale.value.f);
                if (label->text.size.x >= label->width)
                        break;
                label->text.origin.x += (label->width - label->text.size.x) / 2;
                label->text.origin = C_vec2_clamp(label->text.origin,
                                                  r_pixel_scale.value.f);
                break;
        case I_EV_CLEANUP:
                R_sprite_cleanup(&label->text);
                break;
        case I_EV_RENDER:
                label->text.modulate.a = label->widget.fade;
                R_sprite_render(&label->text);
                break;
        default:
                break;
        }
        return TRUE;
}

/******************************************************************************\
 Change a label's text.
\******************************************************************************/
void I_label_configure(i_label_t *label, const char *text)
{
        C_strncpy_buf(label->buffer, text);
        I_widget_event(&label->widget, I_EV_CONFIGURE);
}

/******************************************************************************\
 Initializes a label widget.
\******************************************************************************/
void I_label_init(i_label_t *label, const char *text)
{
        if (!label)
                return;
        C_zero(label);
        I_widget_set_name(&label->widget, "Label");
        label->widget.event_func = (i_event_f)I_label_event;
        label->widget.state = I_WS_READY;
        label->font = R_FONT_GUI;
        C_strncpy_buf(label->buffer, text);
        I_widget_inited(&label->widget);
}

/******************************************************************************\
 Dynamically allocates a label widget.
\******************************************************************************/
i_label_t *I_label_new(const char *text)
{
        i_label_t *label;

        label = C_malloc(sizeof (*label));
        I_label_init(label, text);
        label->widget.heap = TRUE;
        return label;
}

