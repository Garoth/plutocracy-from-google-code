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
n_client_id_t n_client_id;

/* Socket for the connection to the host */
SOCKET n_client_socket;

/******************************************************************************\
 Initializes the network namespace.
\******************************************************************************/
void N_init(void)
{
#ifdef WINDOWS
        WSADATA wsaData;

        if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0)
                C_error("Failed to initialize WinSock");
#endif
        n_client_id = N_INVALID_ID;
}

/******************************************************************************\
 Cleans up the network namespace.
\******************************************************************************/
void N_cleanup(void)
{
#ifdef WINDOWS
        WSACleanup();
#endif
        N_stop_server();
}

/******************************************************************************\
 Connect the client to the given [address] (ip or hostname) and [port].
\******************************************************************************/
bool N_connect(const char *address, n_callback_f client_func)
{
        C_var_unlatch(&n_port);
        n_client_func = client_func;
        n_client_socket = N_connect_socket(address, n_port.value.n);

        /* Connection failed */
        if (n_client_socket == INVALID_SOCKET) {
                n_client_id = N_INVALID_ID;
                return FALSE;
        }

        /* Connected */
        n_client_id = N_UNASSIGNED_ID;
        n_client_func(N_SERVER_ID, N_EV_CONNECTED);
        return n_client_id != N_INVALID_ID;
}

/******************************************************************************\
 Close the client's connection to the server.
\******************************************************************************/
void N_disconnect(void)
{
        if (n_client_id == N_INVALID_ID)
                return;
        if (n_client_func)
                n_client_func(N_SERVER_ID, N_EV_DISCONNECTED);
        if (n_client_id == N_HOST_CLIENT_ID)
                N_stop_server();
        else if (n_client_socket >= 0) {
                closesocket(n_client_socket);
                n_client_socket = INVALID_SOCKET;
        }
        n_client_id = N_INVALID_ID;
        C_debug("Disconnected from server");
}

/******************************************************************************\
 Receive events from the server.
\******************************************************************************/
void N_poll_client(void)
{
        if (n_client_id == N_INVALID_ID || n_client_id == N_HOST_CLIENT_ID)
                return;
        if (!N_receive(N_SERVER_ID))
                N_disconnect();
}

/******************************************************************************\
 Returns a string representation of a client ID.
\******************************************************************************/
const char *N_client_to_string(n_client_id_t client)
{
        if (client == N_HOST_CLIENT_ID)
                return "host client";
        else if (client == N_SERVER_ID)
                return "server";
        else if (client == N_UNASSIGNED_ID)
                return "unassigned";
        else if (client == N_BROADCAST_ID)
                return "broadcast";
        else if (client == N_INVALID_ID)
                return "invalid";
        return C_va("client %d", client);
}

/******************************************************************************\
 Returns TRUE if a client index is valid.
\******************************************************************************/
bool N_client_valid(n_client_id_t client)
{
        return client == N_SERVER_ID ||
               (client >= 0 && client < N_CLIENTS_MAX &&
                n_clients[client].connected);
}

