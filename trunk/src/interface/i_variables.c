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

c_var_t i_debug;

/* Theme varables */
c_var_t i_border, i_button, i_button_active, i_button_hover, i_button_light,
        i_button_prelight, i_color, i_color2, i_fade, i_hanger, i_shadow,
        i_theme, i_window, i_work_area;

/* Interface usability variables */
c_var_t i_scroll_speed, i_zoom_speed;

/******************************************************************************\
 Registers interface namespace variables.
\******************************************************************************/
void I_register_variables(void)
{
        C_register_integer(&i_debug, "i_debug", FALSE,
                           _("interface event traces: 1 = SDL, 2 = widgets"));
        i_debug.edit = C_VE_ANYTIME;

        /* Theme variables */
        C_register_integer(&i_border, "i_border", 8,
                           _("pixel width of borders in interface"));
        i_border.archive = FALSE;
        C_register_string(&i_button, "i_button",
                          "gui/themes/default/button.png",
                          _("path to decorated button texture"));
        i_button.archive = FALSE;
        C_register_string(&i_button_active, "i_button_active",
                          "gui/themes/default/button_active.png",
                          _("path to decorated button pressed texture"));
        i_button_active.archive = FALSE;
        C_register_string(&i_button_hover, "i_button_hover",
                          "gui/themes/default/button_hover.png",
                          _("path to decorated button hover texture"));
        i_button_hover.archive = FALSE;
        C_register_string(&i_button_prelight, "i_button_prelight",
                          "gui/themes/default/button_prelight.png",
                          _("path to icon button hover texture"));
        i_button_prelight.archive = FALSE;
        C_register_string(&i_button_light, "i_button_light",
                          "gui/themes/default/button_light.png",
                          _("path to icon button pressed texture"));
        i_button_light.archive = FALSE;
        C_register_string(&i_color, "i_color", "aluminium1",
                          _("interface text color"));
        i_color.archive = FALSE;
        C_register_string(&i_color2, "i_color2", "aluminium3",
                          _("interface alternate text color"));
        i_color2.archive = FALSE;
        C_register_float(&i_fade, "i_fade", 4.f,
                         _("rate of fade in and out for widgets"));
        i_fade.archive = FALSE;
        i_fade.edit = C_VE_ANYTIME;
        C_register_string(&i_hanger, "i_hanger",
                          "gui/themes/default/hanger.png",
                          _("path to window hanger texture"));
        i_hanger.archive = FALSE;
        C_register_string(&i_theme, "i_theme", "gui/themes/default/theme.cfg",
                          _("path to theme config"));
        C_register_float(&i_shadow, "i_shadow", 1.f,
                         _("darkness of interface shadows: 0-1."));
        i_shadow.archive = FALSE;
        C_register_string(&i_window, "i_window",
                          "gui/themes/default/window.png",
                          _("path to window widget background texture"));
        i_window.archive = FALSE;
        C_register_string(&i_work_area, "i_work_area",
                          "gui/themes/default/work_area.png",
                          _("path to work-area widget background texture"));
        i_work_area.archive = FALSE;

        /* Interface usability variables */
        C_register_float(&i_scroll_speed, "i_scroll_speed", 1.f,
                         _("scrolling speed in pixels per second"));
        i_scroll_speed.edit = C_VE_ANYTIME;
        C_register_float(&i_zoom_speed, "i_zoom_speed", 1.f,
                         _("zoom speed in units per click"));
        i_zoom_speed.edit = C_VE_ANYTIME;
}

