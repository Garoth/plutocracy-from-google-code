/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Client code for the server */
#define N_SERVER_ID 0

/* Client code for the hosting player */
#define N_HOST_CLIENT_ID 1

/* Maximum number of players supported */
#define N_CLIENTS_MAX 32

/* Prototype for a message-receiving function */
typedef void (*n_receive_f)(int source_client);

/* n_client.c */
void N_connect(const char *address, int port, n_receive_f);
void N_disconnect(void);

/* n_server.c */
void N_poll_server(void);
void N_start_server(n_receive_f);
void N_stop_server(void);

/* n_sync.c */
char N_receive_char(void);
float N_receive_float(void);
int N_receive_int(void);
short N_receive_short(void);
const char *N_receive_string(void);
void N_send(int client, const char *format, ...);

