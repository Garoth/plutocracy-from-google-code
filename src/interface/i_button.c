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
 Button widget event function.
\******************************************************************************/
int I_button_event(i_button_t *button, i_event_t event)
{
        c_vec2_t origin, size;
        float width;

        switch (event) {
        case I_EV_CONFIGURE:

                /* Setup decorations */
                R_window_cleanup(&button->normal);
                R_window_cleanup(&button->active);
                R_window_cleanup(&button->hover);
                R_sprite_cleanup(&button->light);
                R_sprite_cleanup(&button->prelight);
                if (button->decorated) {
                        R_window_init(&button->normal, i_button.value.s);
                        R_window_init(&button->hover, i_button_hover.value.s);
                        R_window_init(&button->active, i_button_active.value.s);
                } else {
                        R_sprite_init(&button->light, i_button_light.value.s);
                        R_sprite_init(&button->prelight,
                                      i_button_prelight.value.s);
                }

                /* Setup text */
                R_sprite_cleanup(&button->text);
                R_sprite_init_text(&button->text, R_FONT_GUI, 0.f,
                                   i_shadow.value.f, FALSE, button->buffer);
                button->text.modulate = i_colors[I_COLOR];

                /* Size requisition */
                if (button->widget.size.y < 1) {
                        button->widget.size.y = button->text.size.y;
                        if (button->icon.texture &&
                            button->icon.size.y > button->widget.size.y)
                                button->widget.size.y = button->icon.size.y;
                        if (button->decorated)
                                button->widget.size.y += i_border.value.n * 2;
                }
                if (button->widget.size.x < 1) {
                        button->widget.size.x = button->text.size.x +
                                                button->icon.size.x;
                        if (button->decorated)
                                button->widget.size.x += i_border.value.n * 2;
                        if (button->icon.texture && button->buffer[0])
                                button->widget.size.x += i_border.value.n;
                }

        case I_EV_MOVED:

                /* Setup decorations (for each state) to cover full area */
                origin = button->widget.origin;
                size = button->widget.size;
                if (button->decorated) {
                        button->normal.sprite.origin = origin;
                        button->normal.sprite.size = size;
                        button->hover.sprite.origin = origin;
                        button->hover.sprite.size = size;
                        button->active.sprite.origin = origin;
                        button->active.sprite.size = size;
                } else {
                        c_vec2_t origin2, size2;
                        float border;

                        border = (float)i_border.value.n;
                        if (border > size.x / 4)
                                border = size.x / 4;
                        if (border > size.y / 4)
                                border = size.y / 4;
                        origin2 = C_vec2_subf(origin, border);
                        size2 = C_vec2_addf(size, border * 2.f);
                        button->light.origin = origin2;
                        button->light.size = size2;
                        button->prelight.origin = origin2;
                        button->prelight.size = size2;
                }

                /* Pack the icon left, vertically centered */
                origin.y += size.y / 2;
                if (button->decorated) {
                        origin.x += i_border.value.n;
                        size = C_vec2_subf(size, i_border.value.n * 2.f);
                }
                if (button->icon.texture) {
                        button->icon.origin = C_vec2(origin.x, origin.y -
                                                     button->icon.size.y / 2);
                        width = button->icon.size.x + i_border.value.n;
                        origin.x += width;
                        size.x -= width;
                }

                /* Pack text */
                button->text.origin = origin;
                button->text.origin.y -= button->text.size.y / 2;

                /* If there is some room left, center the icon and text */
                width = (size.x - button->text.size.x) / 2.f;
                if (width > 0.f) {
                        button->icon.origin.x += width;
                        button->text.origin.x += width;
                }

                /* Clamp origins to prevent blurriness */
                button->text.origin = C_vec2_clamp(button->text.origin,
                                                   r_pixel_scale.value.f);

                break;
        case I_EV_CLEANUP:
                R_window_cleanup(&button->normal);
                R_window_cleanup(&button->active);
                R_window_cleanup(&button->hover);
                R_sprite_cleanup(&button->light);
                R_sprite_cleanup(&button->prelight);
                R_sprite_cleanup(&button->icon);
                R_sprite_cleanup(&button->text);
                break;
        case I_EV_MOUSE_DOWN:
                if (i_mouse == SDL_BUTTON_LEFT)
                        button->widget.state = I_WS_ACTIVE;
                break;
        case I_EV_MOUSE_UP:
                if (button->widget.state == I_WS_ACTIVE && button->on_click) {
                        button->on_click(button);
                        button->widget.state = I_WS_HOVER;
                }
                break;
        case I_EV_RENDER:
                button->icon.modulate.a = button->widget.fade;
                button->text.modulate.a = button->widget.fade;
                button->hover.sprite.modulate.a = button->widget.fade;
                button->active.sprite.modulate.a = button->widget.fade;
                button->normal.sprite.modulate.a = button->widget.fade;
                if (button->widget.state == I_WS_HOVER) {
                        R_window_render(&button->hover);
                        button->prelight.modulate.a = button->widget.fade;
                        R_sprite_render(&button->prelight);
                } else if (button->widget.state == I_WS_ACTIVE) {
                        R_window_render(&button->active);
                        button->light.modulate.a = button->widget.fade;
                        R_sprite_render(&button->light);
                } else
                        R_window_render(&button->normal);
                R_push_clip();
                R_clip_rect(button->widget.origin, button->widget.size);
                R_sprite_render(&button->icon);
                R_sprite_render(&button->text);
                R_pop_clip();
                break;
        default:
                break;
        }
        return TRUE;
}

/******************************************************************************\
 Configure the button widget. The button must have already been initialized.
 The [icon] path and [text] strings can be NULL and do not need to persist
 after calling the function. Call with [bg] set to TRUE to decorate the button
 background.
\******************************************************************************/
void I_button_configure(i_button_t *button, const char *icon, const char *text,
                        int bg)
{
        button->decorated = bg;
        R_sprite_init(&button->icon, icon);
        C_strncpy_buf(button->buffer, text);
        I_widget_event(&button->widget, I_EV_CONFIGURE);
}

/******************************************************************************\
 Initializes a button widget. The [icon] path and [text] strings can be NULL
 and do not need to persist after calling the function. Call with [bg] set to
 TRUE to decorate the button background.
\******************************************************************************/
void I_button_init(i_button_t *button, const char *icon, const char *text,
                   int bg)
{
        C_zero(button);
        I_widget_set_name(&button->widget, "Button");
        button->widget.event_func = (i_event_f)I_button_event;
        button->widget.state = I_WS_READY;
        button->widget.clickable = TRUE;
        button->on_click = NULL;
        button->decorated = bg;
        R_sprite_init(&button->icon, icon);
        C_strncpy_buf(button->buffer, text);
        I_widget_inited(&button->widget);
}

