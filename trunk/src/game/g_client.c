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

/******************************************************************************\
 Leave the current game.
\******************************************************************************/
void G_quit_game(void)
{
        I_popup(I_PI_NONE, "Left the current game.", NULL);
}

/******************************************************************************\
 Change the player's nation.
\******************************************************************************/
void G_change_nation(int index)
{
        if (index == 0)
                I_popup(I_PI_NONE, "Joined the Ruby nation.", NULL);
        else if (index == 1)
                I_popup(I_PI_NONE, "Joined the Emerald nation.", NULL);
        else if (index == 2)
                I_popup(I_PI_NONE, "Joined the Sapphire nation.", NULL);
        else if (index == 3)
                I_popup(I_PI_NONE, "Became a pirate.", NULL);
}

