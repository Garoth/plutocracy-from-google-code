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

/* ID of this client in the game */
int n_client_id;

/******************************************************************************\
 Connect the client to the given [address] (ip or hostname) and [port].
\******************************************************************************/
void N_connect(const char *address, int port, n_callback_f client_func)
{
        n_client_func = client_func;
}

/******************************************************************************\
 Close the client's connection to the server.
\******************************************************************************/
void N_disconnect(void)
{
        n_client_func(N_SERVER_ID, N_EV_DISCONNECTED);
}

