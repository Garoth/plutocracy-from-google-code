/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "n_common.h"

/* TRUE if the server is running */
int n_serving;

/******************************************************************************\
 Open server sockets and begin accepting connections.
\******************************************************************************/
void N_start_server(n_receive_f receive_func)
{
        if (n_serving)
                return;
        n_serving = TRUE;
        n_receive_server = receive_func;
}

/******************************************************************************\
 Close server sockets and stop accepting connections.
\******************************************************************************/
void N_stop_server(void)
{
        n_serving = FALSE;
}

/******************************************************************************\
 Poll connections and dispatch any messages that arrive.
\******************************************************************************/
void N_poll_server(void)
{
}

