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
#define N_SERVER_ID -1

/* Client code for the hosting player */
#define N_HOST_CLIENT_ID 0

/* ID used when not connected */
#define N_INVALID_ID -2

/* Maximum number of players supported */
#define N_CLIENTS_MAX 32

/* Special client ID for broadcasting to all clients */
#define N_BROADCAST_ID N_CLIENTS_MAX

/* Callback function events */
typedef enum {
        N_EV_MESSAGE,
        N_EV_CONNECTED,
        N_EV_DISCONNECTED,
} n_event_t;

/* Callback function prototype */
typedef void (*n_callback_f)(int client, n_event_t);

/* Structure for connected clients */
typedef struct n_client {
        bool connected;
} n_client_t;

/* n_client.c */
void N_connect(const char *address, int port, n_callback_f client);
void N_disconnect(void);

extern int n_client_id;

/* n_server.c */
void N_cleanup(void);
void N_kick_client(int client);
void N_init(void);
void N_poll_server(void);
int N_start_server(n_callback_f server, n_callback_f client);
void N_stop_server(void);

extern n_client_t n_clients[N_CLIENTS_MAX];

/* n_sync.c */
char N_receive_char(void);
float N_receive_float(void);
int N_receive_int(void);
short N_receive_short(void);
void N_receive_string(char *buffer, int size);
void N_send(int client, const char *format, ...);

