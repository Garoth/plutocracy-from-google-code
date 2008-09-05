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

/* The number of send() tries before failure */
#define SEND_RETRY 5

/* Socket connection timeout in seconds */
#define SOCKET_TIMEOUT 1

/******************************************************************************\
 Make a generic TCP/IP socket connection.
\******************************************************************************/
SOCKET N_connect_socket(const char *address, int port)
{
        struct sockaddr_in addr;
        struct hostent *host;
        SOCKET sock;
        int i, last_colon, ret;
        const char *error;
        char buffer[64], *host_ip;

        /* Parse the port out of the address string */
        for (last_colon = -1, i = 0; address[i]; i++)
                if (address[i] == ':')
                        last_colon = i;
        if (last_colon >= 0) {
                int value;

                if ((value = atoi(address + last_colon + 1)))
                        port = value;
                memcpy(buffer, address, last_colon);
                buffer[last_colon] = NUL;
                address = buffer;
        }

        /* Resolve hostnames */
        host = gethostbyname(address);
        if (!host) {
                C_warning("Failed to resolve hostname '%s'", address);
                return INVALID_SOCKET;
        }
        host_ip = inet_ntoa(*((struct in_addr *)host->h_addr));
        C_debug("Resolved '%s' to %s", address, host_ip);

        /* Connect to the server */
        sock = socket(PF_INET, SOCK_STREAM, 0);
        N_socket_no_block(sock);
        C_zero(&addr);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(host_ip);
        ret = connect(sock, (struct sockaddr *)&addr, sizeof (addr));

        /* Error connecting */
        if ((error = N_socket_error(ret))) {
                C_warning("Connect error: %s", error);
                return INVALID_SOCKET;
        }

        /* Connection failed */
        if (!N_socket_select(sock, TRUE)) {
                closesocket(sock);
                C_warning("Failed to connect to %s:%d", host_ip, port);
                return INVALID_SOCKET;
        }

        /* Connected */
        C_debug("Connected to %s:%d", host_ip, port);
        return sock;
}

/******************************************************************************\
 Send generic data over a socket. Returns FALSE if there was an error.
\******************************************************************************/
bool N_socket_send(SOCKET socket, const char *data, int size)
{
        int i, ret, bytes_sent;
        const char *error;

        for (ret = bytes_sent = i = 0;
             bytes_sent < size && i < SEND_RETRY; i++) {
                if (!N_socket_select(socket, TRUE))
                        break;
                ret = send(socket, data, size, 0);
                if ((error = N_socket_error(ret))) {
                        C_warning("Send error: %s", error);
                        break;
                }
                bytes_sent += ret;
                if (bytes_sent >= size)
                        return TRUE;
                SDL_Delay(10);
        }

        /* Send failed */
        C_warning("Send failed, returned %d; %d tries, %d/%d bytes",
                  ret, i, bytes_sent, size);
        return FALSE;
}

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
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS)
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
bool N_socket_select(SOCKET sock, bool write)
{
        struct timeval tv;
        fd_set fds;
        int nfds;

        /* Timeout */
        tv.tv_sec = SOCKET_TIMEOUT;
        tv.tv_usec = 0;

        FD_ZERO(&fds);
        FD_SET(sock, &fds);
#ifdef WINDOWS
        nfds = 0;
#else
        nfds = sock + 1;
#endif
        if (write)
                select(nfds, NULL, &fds, NULL, &tv);
        else
                select(nfds, &fds, NULL, NULL, &tv);
        return FD_ISSET(sock, &fds);
}

