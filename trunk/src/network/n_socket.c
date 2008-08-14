/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Convenience and compatibility wrappers for sockets */

#include "n_common.h"

/******************************************************************************\
 Get the socket for the given client ID.
\******************************************************************************/
SOCKET N_client_to_socket(n_client_id_t client)
{
        if (client == N_SERVER_ID)
                return n_client_socket;
        else if (client >= 0 && client < N_CLIENTS_MAX)
                return n_clients[client].socket;
        C_error("Invalid client ID %d", client);
        return INVALID_SOCKET;
}

/******************************************************************************\
 Pass the value returned by the socket operation as [ret]. Returns NULL if no
 real error was generated otherwise returns the string representation of it.
 TODO: Function to convert WinSock error codes to strings
\******************************************************************************/
const char *N_socket_error(int ret)
{
        if (ret >= 0)
                return NULL;
#ifdef WINDOWS
        /* No data (WinSock) */
        if (WSAGetLastError() == WSAEWOULDBLOCK)
                return NULL;
        return C_va("%d", WSAGetLastError());
#else
        /* No data (Berkeley) */
        if (errno == EAGAIN || errno == EWOULDBLOCK)
                return NULL;
        return strerror(errno);
#endif
}

/******************************************************************************\
 Put a socket in non-blocking mode.
\******************************************************************************/
void N_socket_no_block(SOCKET socket)
{
#ifdef WINDOWS
        u_long mode = 1;
        ioctlsocket(socket, FIONBIO, &mode);
#else
        fcntl(socket, F_SETFL, O_NONBLOCK);
#endif
}

/******************************************************************************\
 Waits for a socket to become readable. Returns TRUE on success.
\******************************************************************************/
bool N_socket_select(SOCKET socket, bool write)
{
        struct timeval tv;
        fd_set fds;
        int nfds;

        /* Timeout of one second */
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        FD_ZERO(&fds);
        FD_SET(socket, &fds);
#ifdef WINDOWS
        nfds = 0;
#else
        nfds = socket + 1;
#endif
        if (write)
                select(nfds, NULL, &fds, NULL, &tv);
        else
                select(nfds, &fds, NULL, NULL, &tv);
        return FD_ISSET(socket, &fds);
}

