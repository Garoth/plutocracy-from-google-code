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
static int button_event(i_button_t *button, i_event_t event)
{
        c_vec2_t origin, size;
        float width;

        switch (event) {
        case I_EV_CONFIGURE:

                /* Setup decorations */
                R_window_cleanup(&button->normal);
                R_window_cleanup(&button->active);
                R_window_cleanup(&button->hover);
                if (button->decorated) {
                        R_window_init(&button->normal, i_button.value.s);
                        R_window_init(&button->hover, i_button_hover.value.s);
                        R_window_init(&button->active, i_button_active.value.s);
                }

                /* Setup text */
                R_sprite_cleanup(&button->text);
                R_sprite_init_text(&button->text, R_FONT_GUI, 0.f,
                                   i_shadow.value.f, FALSE, button->buffer);
                button->text.modulate = i_colors[I_COLOR];

                /* Size requisition */
                if (!button->widget.size.y)
                        button->widget.size.y = button->text.size.y +
                                                i_border.value.n * 2;
                if (!button->widget.size.x) {
                        button->widget.size.x = button->text.size.x +
                                                i_border.value.n * 2;
                        if (button->icon.texture)
                                button->widget.size.x += button->icon.size.x +
                                                         i_border.value.n;
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
                }

                /* Pack the icon left, vertically centered */
                origin.x += i_border.value.n;
                origin.y += size.y / 2;
                size = C_vec2_subf(size, i_border.value.n * 2);
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
                R_sprite_cleanup(&button->icon);
                R_sprite_cleanup(&button->text);
                break;
        case I_EV_MOUSE_DOWN:
                if (i_mouse == SDL_BUTTON_LEFT)
                        button->widget.state = I_WS_ACTIVE;
                break;
        case I_EV_MOUSE_UP:
                if (button->widget.state == I_WS_ACTIVE && button->on_click)
                        button->on_click(button);
                break;
        case I_EV_RENDER:
                button->icon.modulate.a = button->widget.fade;
                button->text.modulate.a = button->widget.fade;
                R_sprite_render(&button->icon);
                if (button->widget.state == I_WS_HOVER) {
                        button->hover.sprite.modulate.a = button->widget.fade;
                        R_window_render(&button->hover);
                } else if (button->widget.state == I_WS_ACTIVE) {
                        button->active.sprite.modulate.a = button->widget.fade;
                        R_window_render(&button->active);
                } else {
                        button->normal.sprite.modulate.a = button->widget.fade;
                        R_window_render(&button->normal);
                }
                R_clip_rect(button->widget.origin, button->widget.size);
                R_sprite_render(&button->text);
                R_clip_disable();
                break;
        default:
                break;
        }
        return TRUE;
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
        button->widget.event_func = (i_event_f)button_event;
        button->widget.state = I_WS_READY;
        button->on_click = NULL;
        button->decorated = bg;
        R_sprite_init(&button->icon, icon);
        C_strncpy_buf(button->buffer, text);
        I_widget_inited(&button->widget);
}

