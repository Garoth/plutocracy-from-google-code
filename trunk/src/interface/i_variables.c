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

c_var_t i_border, i_window_bg;

/******************************************************************************\
 Registers interface namespace variables.
\******************************************************************************/
void I_register_variables(void)
{
        C_register_integer(&i_border, "i_border", 8);
        C_register_string(&i_window_bg, "i_window_bg", "gui/windows/blue.png");
}

