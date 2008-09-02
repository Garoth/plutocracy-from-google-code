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
short n_sync_pos, n_sync_size;
char n_sync_buffer[N_SYNC_MAX];

static bool sending;

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
 Sends the current sync buffer out to the given client.
\******************************************************************************/
static void send_buffer(n_client_id_t client)
{
        SOCKET socket;
        int i, ret, bytes_sent;

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
        socket = N_client_to_socket(client);
        for (ret = bytes_sent = i = 0; bytes_sent < n_sync_size && i < 5; i++) {
                const char *error;
                int ret;

                if (!N_socket_select(socket, TRUE))
                        break;
                ret = send(socket, n_sync_buffer, n_sync_size, 0);
                if ((error = N_socket_error(ret))) {
                        C_warning("Error sending to %s: %s",
                                  N_client_to_string(client), error);
                        break;
                }
                bytes_sent += ret;
                if (bytes_sent >= n_sync_size)
                        return;
                SDL_Delay(10);
        }

        /* Send failed */
        C_warning("Send to %s failed, returned %d; %d tries, %d/%d bytes",
                  N_client_to_string(client), ret, i, bytes_sent,
                  n_sync_size);
        N_drop_client(client);
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
 Reset the sync buffer. Use before sending via N_send_* calls.
\******************************************************************************/
void N_send_start(void)
{
        n_sync_size = 2;
}

/******************************************************************************\
 Add data to the send buffer. Call N_send(client, NULL) to finish sending data.
 Returns FALSE if the buffer overflowed.
\******************************************************************************/
bool N_send_char(char ch)
{
        return write_bytes(n_sync_size, 1, &ch);
}

bool N_send_short(short n)
{
        return write_bytes(n_sync_size, 2, &n);
}

bool N_send_int(int n)
{
        return write_bytes(n_sync_size, 4, &n);
}

bool N_send_float(float f)
{
        return write_bytes(n_sync_size, 4, &f);
}

bool N_send_string(const char *string)
{
        int string_len;

        string_len = C_strlen(string) + 1;
        if (string_len <= 1) {
                if (n_sync_size > N_SYNC_MAX - 1)
                        return FALSE;
                n_sync_buffer[n_sync_size++] = NUL;
                return TRUE;
        }
        if (n_sync_size + string_len > N_SYNC_MAX)
               return FALSE;
        memcpy(n_sync_buffer + n_sync_size, string, string_len);
        n_sync_size += string_len;
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
void N_send_full(const char *file, int line, const char *func,
                 int client, const char *format, ...)
{
        static int stamp;
        va_list va;
        int sentinel;
        
        /* We're not connected */
        if (n_client_id < 0)
                return;

        /* Clients don't send messages to anyone but the server */
        if (n_client_id != N_HOST_CLIENT_ID && client != N_SERVER_ID)
                return;

        /* Pack the message into the sync buffer */
        if (!format || !format[0])
                goto skip;
        va_start(va, format);
        for (n_sync_size = 2; *format; format++)
                switch (*format) {
                case '1':
                case 'c':
                        if (!N_send_char((char)va_arg(va, int)))
                                goto overflow;
                        break;
                case '2':
                case 'd':
                        if (!N_send_short((short)va_arg(va, int)))
                                goto overflow;
                        break;
                case '4':
                case 'l':
                        if (!N_send_int(va_arg(va, int)))
                                goto overflow;
                        break;
                case 'f':
                        if (!N_send_float((float)va_arg(va, double)))
                                goto overflow;
                        break;
                case 's':
                        if (!N_send_string(va_arg(va, const char *)))
                                goto overflow;
                        break;
                default:
                        C_error_full(file, line, func,
                                     "Invalid format character '%c'", *format);
                }

        /* Check sentinel; if its missing then you probably forgot to add
           an argument to the argument list or have one too many */
        sentinel = va_arg(va, int);
        if (sentinel != N_SENTINEL)
                C_error_full(file, line, func, "Missing sentinel");
        va_end(va);

        /* Write the size of the message as the first 2-bytes */
skip:   write_bytes(0, 2, &n_sync_size);
        stamp++;

        /* Broadcast to every client */
        if (client == N_BROADCAST_ID || client == N_SELECTED_ID || client < 0) {
                int i, except, broadcast_stamp;

                C_assert(n_client_id == N_HOST_CLIENT_ID);
                broadcast_stamp = stamp;
                except = -client - 1;
                for (i = 0; i < N_CLIENTS_MAX; i++) {
                        if (!n_clients[i].connected || i == except ||
                            (!n_clients[i].selected && client == N_SELECTED_ID))
                                continue;
                        if (stamp != broadcast_stamp)
                                C_error_full(file, line, func,
                                             "Broadcast buffer overwritten");
                        send_buffer(i);
                }
                return;
        }

        /* Single-client message */
        send_buffer(client);
        return;

overflow:
        va_end(va);
        C_warning_full(file, line, func, "Outgoing message buffer overflow");
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
        for (socket = N_client_to_socket(client); ; ) {
                const char *error;

                /* Receive the message size */
                len = (int)recv(socket, n_sync_buffer, N_SYNC_MAX, MSG_PEEK);

                /* Orderly shutdown */
                if (!len)
                        return FALSE;

                /* Error */
                if ((error = N_socket_error(len))) {
                        C_debug("Error receiving from %s: %s",
                                N_client_to_string(client), error);
                        return FALSE;
                }

                /* Did not receive the message size */
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

