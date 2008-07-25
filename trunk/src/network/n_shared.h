/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* Largest amount of data that can be sent via a message */
#define N_SYNC_MAX 1024

/* Windows compatibility */
#ifdef WINDOWS
typedef UINT_PTR SOCKET;
#else
typedef int SOCKET;
#endif

/* Special client IDs */
typedef enum {
        N_INVALID_ID = -1,
        N_HOST_CLIENT_ID = 0,
        N_CLIENTS_MAX = 32,
        N_SERVER_ID,
        N_UNASSIGNED_ID,
        N_BROADCAST_ID,
} n_client_id_t;

/* Callback function events */
typedef enum {
        N_EV_MESSAGE,
        N_EV_CONNECTED,
        N_EV_DISCONNECTED,
} n_event_t;

/* Callback function prototype */
typedef void (*n_callback_f)(n_client_id_t, n_event_t);

/* Structure for connected clients */
typedef struct n_client {
        SOCKET socket;
        bool connected;
} n_client_t;

/* n_client.c */
void N_cleanup(void);
const char *N_client_to_string(n_client_id_t);
bool N_client_valid(n_client_id_t);
bool N_connect(const char *address, n_callback_f client);
void N_disconnect(void);
void N_init(void);
void N_poll_client(void);

extern n_client_id_t n_client_id;

/* n_server.c */
void N_kick_client(n_client_id_t);
void N_poll_server(void);
int N_start_server(n_callback_f server, n_callback_f client);
void N_stop_server(void);

extern n_client_t n_clients[N_CLIENTS_MAX];

/* n_sync.c */
#define N_broadcast(f, ...) N_send(N_BROADCAST_ID, f, ## __VA_ARGS__)
#define N_broadcast_except(c, f, ...) N_send(-(c) - 1, f, ## __VA_ARGS__)
char N_receive_char(void);
float N_receive_float(void);
int N_receive_int(void);
short N_receive_short(void);
void N_receive_string(char *buffer, int size);
#define N_receive_string_buf(b) N_receive_string(b, sizeof (b))
void N_send(n_client_id_t, const char *format, ...);

/* n_variables.c */
void N_register_variables(void);

