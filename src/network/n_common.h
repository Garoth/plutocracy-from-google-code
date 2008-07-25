/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "../common/c_shared.h"
#include "n_shared.h"

/* Sockets */
#ifdef WINDOWS
#include <winsock.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

/* Windows compatibility */
#ifndef WINDOWS
#define INVALID_SOCKET -1
#define closesocket close
#else
typedef int socklen_t;
#endif

/* n_client.c */
void N_socket_no_block(SOCKET);

extern SOCKET n_client_socket;

/* n_sync.c */
bool N_receive(int client);

extern n_callback_f n_client_func, n_server_func;

/* n_variables.c */
extern c_var_t n_port;

