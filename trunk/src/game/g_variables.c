/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "g_common.h"

/* Globe variables */
c_var_t g_globe_seed, g_globe_subdiv4;

/******************************************************************************\
 Registers the game namespace variables.
\******************************************************************************/
void G_register_variables(void)
{
        C_register_integer(&g_globe_seed, "g_globe_seed", C_rand());
        g_globe_seed.archive = FALSE;
        C_register_integer(&g_globe_subdiv4, "g_globe_subdiv4", 4);
        g_globe_subdiv4.archive = FALSE;
}

