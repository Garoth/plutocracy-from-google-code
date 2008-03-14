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
c_var_t i_border, i_button, i_button_active, i_button_hover, i_color,
        i_fade, i_shadow, i_theme, i_window;

/******************************************************************************\
 Registers interface namespace variables.
\******************************************************************************/
void I_register_variables(void)
{
        C_register_integer(&i_debug, "i_debug", FALSE);

        /* Theme variables */
        C_register_integer(&i_border, "i_border", 8);
        C_register_string(&i_button, "i_button",
                          "gui/themes/default/button.png");
        C_register_string(&i_button_active, "i_button_active",
                          "gui/themes/default/button_active.png");
        C_register_string(&i_button_hover, "i_button_hover",
                          "gui/themes/default/button_hover.png");
        C_register_string(&i_color, "i_color", "#eeeeec");
        C_register_float(&i_fade, "i_fade", 1.f);
        C_register_string(&i_theme, "i_theme",
                          "gui/themes/default/theme.cfg");
        C_register_float(&i_shadow, "i_shadow", 1.f);
        C_register_string(&i_window, "i_window",
                          "gui/themes/default/window.png");
}

