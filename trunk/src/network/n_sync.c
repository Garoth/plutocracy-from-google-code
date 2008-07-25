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
#include "SDL_endian.h"

/* Receive function that arriving messages are routed to */
n_callback_f n_client_func, n_server_func;

/* Little-Endian message data buffer */
int n_sync_pos, n_sync_size;
char n_sync_buffer[N_SYNC_MAX];

/******************************************************************************\
 Call these functions to retrieve an argument from the current message from
 within the [n_receive] function when it is called.
\******************************************************************************/
char N_receive_char(void)
{
        char value;

        if (n_sync_pos + 1 > n_sync_size)
                return NUL;
        value = n_sync_buffer[n_sync_pos++];
        return value;
}

int N_receive_int(void)
{
        int value;

        if (n_sync_pos + 4 > n_sync_size)
                return 0;
        value = (int)SDL_SwapLE32(*(Uint32 *)(n_sync_buffer + n_sync_pos));
        n_sync_pos += 4;
        return value;
}

float N_receive_float(void)
{
        union {
                int n;
                float f;
        } value;

        value.n = N_receive_int();
        return value.f;
}

short N_receive_short(void)
{
        short value;

        if (n_sync_pos + 2 > n_sync_size)
                return 0;
        value = (short)SDL_SwapLE16(*(Uint16 *)(n_sync_buffer + n_sync_pos));
        n_sync_pos += 2;
        return value;
}

void N_receive_string(char *buffer, int size)
{
        int from, len;

        C_assert(buffer && size > 0);
        for (from = n_sync_pos; n_sync_buffer[n_sync_pos]; n_sync_pos++)
                if (n_sync_pos > n_sync_size) {
                        *buffer = NUL;
                        return;
                }
        len = ++n_sync_pos - from;
        if (len > size)
                len = size;
        memmove(buffer, n_sync_buffer + from, len);
}

/******************************************************************************\
 Get the socket for the given client ID.
\******************************************************************************/
static SOCKET client_to_socket(n_client_id_t client)
{
        if (client == N_SERVER_ID)
                return n_client_socket;
        else if (client >= 0 && client < N_CLIENTS_MAX)
                return n_clients[client].socket;
        C_error("Invalid client ID %d", client);
        return INVALID_SOCKET;
}

/******************************************************************************\
 Sends the current sync buffer out to the given client.
\******************************************************************************/
static void send_buffer(n_client_id_t client)
{
        if (client >= 0 && client < N_CLIENTS_MAX &&
            !n_clients[client].connected) {
                C_warning("Tried to message unconnected client %d", client);
                return;
        }

        /* Deliver server messages to itself directly */
        if (n_client_id == N_HOST_CLIENT_ID) {
                if (client == N_SERVER_ID) {
                        n_sync_pos = 2;
                        n_server_func(N_HOST_CLIENT_ID, N_EV_MESSAGE);
                        return;
                } else if (client == N_HOST_CLIENT_ID) {
                        n_sync_pos = 2;
                        n_client_func(N_SERVER_ID, N_EV_MESSAGE);
                        return;
                }
        }

        /* Send TCP/IP message */
        send(client_to_socket(client), n_sync_buffer, n_sync_size, 0);
}

/******************************************************************************\
 Write bytes to the data buffer. The datum is assumed to be an integer or
 float that needs byte-order rearranging.
\******************************************************************************/
static bool write_bytes(int offset, int bytes, void *data)
{
        void *to;

        if (offset + bytes > N_SYNC_MAX)
                return FALSE;
        to = n_sync_buffer + offset;
        switch (bytes) {
        case 1:
                *(char *)to = *(char *)data;
                break;
        case 2:
                *(Uint16 *)to = SDL_SwapLE16(*(Uint16 *)data);
                break;
        case 4:
        default:
                *(Uint32 *)to = SDL_SwapLE32(*(Uint32 *)data);
                break;
        }
        if (offset + bytes > n_sync_size)
                n_sync_size = offset + bytes;
        return TRUE;
}

/******************************************************************************\
 Sends a message to [client]. If [client] is 0, sends to the server. The
 [format] string describes the variable argument list given to the function.

   c, 1    char       1 byte
   d, 2    short      2 bytes
   l, 4    integer    4 bytes
   f       float      4 bytes
   s       string     NULL-terminated

 If [client] is (-id - 1), all clients except id will receive the message. Note
 that you can either be receiving or sending. The moment you send a new
 message, the one you were receiving is lost!
\******************************************************************************/
void N_send(int client, const char *format, ...)
{
        va_list va;
        union {
                float f;
                int n;
                short s;
                char c;
        } value;
        const char *string;
        int string_len;

        /* We're not connected */
        if (n_client_id < 0)
                return;

        /* Clients don't send messages to anyone but the server */
        if (n_client_id != N_HOST_CLIENT_ID && client != N_SERVER_ID)
                return;

        /* Pack the message into the sync buffer */
        va_start(va, format);
        for (n_sync_size = 2; *format; format++)
                switch (*format) {
                case '1':
                case 'c':
                        value.c = (char)va_arg(va, int);
                        if (!write_bytes(n_sync_size, 1, &value.c))
                                goto overflow;
                        break;
                case '2':
                case 'd':
                        value.s = (short)va_arg(va, int);
                        if (!write_bytes(n_sync_size, 2, &value.s))
                                goto overflow;
                        break;
                case '4':
                case 'l':
                        value.n = va_arg(va, int);
                        if (!write_bytes(n_sync_size, 4, &value.n))
                                goto overflow;
                        break;
                case 'f':
                        value.f = (float)va_arg(va, double);
                        if (!write_bytes(n_sync_size, 4, &value.f))
                                goto overflow;
                        break;
                case 's':
                        string = va_arg(va, const char *);
                        string_len = C_strlen(string) + 1;
                        if (string_len <= 1) {
                                if (n_sync_size > N_SYNC_MAX - 1)
                                        goto overflow;
                                n_sync_buffer[n_sync_size++] = NUL;
                                break;
                        }
                        if (n_sync_size + string_len > N_SYNC_MAX)
                               goto overflow;
                        memcpy(n_sync_buffer + n_sync_size, string, string_len);
                        n_sync_size += string_len;
                        break;
                default:
                        C_error("Invalid format character '%c'", *format);
                }
        va_end(va);

        /* Write the size of the message as the first 2-bytes */
        value.s = (short)n_sync_size;
        write_bytes(0, 2, &value.s);

        /* Broadcast to every client */
        if (client == N_BROADCAST_ID || client < 0) {
                int i;

                C_assert(n_client_id == N_HOST_CLIENT_ID);
                client = -client - 1;
                for (i = 0; i < N_CLIENTS_MAX; i++)
                        if (n_clients[i].connected && i != client)
                                send_buffer(i);
                return;
        }

        /* Single-client message */
        send_buffer(client);
        return;

overflow:
        va_end(va);
        C_warning("Outgoing message buffer overflow");
}

/******************************************************************************\
 Receive data from a socket. Returns FALSE if an error occured and the
 connection should be dropped.
\******************************************************************************/
bool N_receive(n_client_id_t client)
{
        SOCKET socket;
        int len, message_size;

        if (client == n_client_id)
                return TRUE;
        socket = client_to_socket(client);
        for (;;) {

                /* Receive the message size */
                len = (int)recv(socket, n_sync_buffer, N_SYNC_MAX, MSG_PEEK);

                /* Orderly shutdown */
                if (!len)
                        return FALSE;

                /* Error */
                if (len < 0) {
#ifdef WINDOWS
                        /* No data (WinSock) */
                        if (WSAGetLastError() == WSAEWOULDBLOCK)
                                return TRUE;
                        C_debug("WinSock error %d (recv returned %d, %s)",
                                WSAGetLastError(), len,
                                N_client_to_string(client));
#else
                        /* No data (Berkeley) */
                        if (errno == EAGAIN)
                                return TRUE;
                        C_debug("%s (recv returned %d, %s)", strerror(errno),
                                len, N_client_to_string(client));
#endif
                        return FALSE;
                }

                /* Did not receive even the message size */
                if (len < 2)
                        return TRUE;

                /* Read the message length */
                n_sync_pos = 0;
                n_sync_size = 2;
                message_size = N_receive_short();
                if (message_size < 1 || message_size > N_SYNC_MAX) {
                        C_warning("Invalid message size %d "
                                  "(recv returned %d, %s)", message_size, len,
                                  N_client_to_string(client));
                        return FALSE;
                }

                /* Check if we have enough room now */
                if (len < message_size)
                        return TRUE;

                /* Read the entire message */
                recv(socket, n_sync_buffer, message_size, 0);

                /* Dispatch the message */
                n_sync_pos = 2;
                n_sync_size = message_size;
                if (n_client_id == N_HOST_CLIENT_ID)
                        n_server_func(client, N_EV_MESSAGE);
                else
                        n_client_func(N_SERVER_ID, N_EV_MESSAGE);
        }
}

