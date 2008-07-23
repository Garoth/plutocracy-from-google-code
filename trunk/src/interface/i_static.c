/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Simple widgets that don't interact with the user */

#include "i_common.h"

static r_texture_t *separator_tex;

/******************************************************************************\
 Theme images for static widgets.
\******************************************************************************/
void I_theme_statics(void)
{
        I_theme_texture(&separator_tex, "separator");
}

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
        I_widget_init(&label->widget, "Label");
        label->widget.event_func = (i_event_f)I_label_event;
        label->widget.state = I_WS_READY;
        label->font = R_FONT_GUI;
        C_strncpy_buf(label->buffer, text);
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

/******************************************************************************\
 Box widget event function.
\******************************************************************************/
int I_box_event(i_box_t *box, i_event_t event)
{
        if (event != I_EV_CONFIGURE)
                return TRUE;
        if (box->width > 0.f) {
                if (box->pack_children == I_PACK_H)
                        box->widget.size.y = box->width;
                else if (box->pack_children == I_PACK_V)
                        box->widget.size.x = box->width;
        }
        I_widget_pack(&box->widget, box->pack_children, box->fit);
        box->widget.size = I_widget_child_bounds(&box->widget);
        return FALSE;
}

/******************************************************************************\
 Initializes a box widget.
\******************************************************************************/
void I_box_init(i_box_t *box, i_pack_t pack, float width)
{
        if (!box)
                return;
        C_zero(box);
        I_widget_init(&box->widget, "Box");
        box->widget.event_func = (i_event_f)I_box_event;
        box->widget.state = I_WS_READY;
        box->pack_children = pack;
        box->fit = I_FIT_NONE;
        box->width = width;
}

/******************************************************************************\
 Image event function.
\******************************************************************************/
int I_image_event(i_image_t *image, i_event_t event)
{
        switch (event) {
        case I_EV_CONFIGURE:
                if (image->theme_texture) {
                        R_sprite_cleanup(&image->sprite);
                        R_sprite_init(&image->sprite, *image->theme_texture);
                        image->original_size = image->sprite.size;
                }
                if (image->resize && image->widget.size.x) {
                        image->sprite.size.x = image->widget.size.x;
                        image->widget.size.y = image->original_size.y;
                } else if (image->resize && image->widget.size.y) {
                        image->sprite.size.y = image->widget.size.y;
                        image->widget.size.x = image->original_size.x;
                } else
                        image->widget.size = image->sprite.size;
        case I_EV_MOVED:
                image->sprite.origin = image->widget.origin;
                break;
        case I_EV_RENDER:
                image->sprite.modulate.a = image->widget.fade;
                R_sprite_render(&image->sprite);
                break;
        case I_EV_CLEANUP:
                R_sprite_cleanup(&image->sprite);
                break;
        default:
                break;
        }
        return TRUE;
}

/******************************************************************************\
 Initializes an image widget.
\******************************************************************************/
void I_image_init(i_image_t *image, const char *icon)
{
        if (!image)
                return;
        C_zero(image);
        I_widget_init(&image->widget, "Image");
        image->widget.event_func = (i_event_f)I_image_event;
        image->widget.state = I_WS_READY;
        if (icon) {
                R_sprite_load(&image->sprite, icon);
                image->original_size = image->sprite.size;
        }
}

/******************************************************************************\
 Initializes an image widget with a pointer to a theme-updated texture
 variable. Pass null for [pptex] to initialize a separator.
\******************************************************************************/
void I_image_init_themed(i_image_t *image, r_texture_t **pptex)
{
        if (!image)
                return;
        I_image_init(image, NULL);
        if (!pptex)
                pptex = &separator_tex;
        image->theme_texture = pptex;
}

