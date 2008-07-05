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

/* Largest amount of data that can be sent via a message */
#define BUFFER_LEN 1024

/* Receive function that arriving messages are routed to */
n_callback_f n_client_func, n_server_func;

/* Little-Endian message data buffer */
int n_sync_pos, n_sync_size;
char n_sync_buffer[BUFFER_LEN];

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
 Sends a message to [client]. If [client] is 0, sends to the server. The
 [format] string describes the variable argument list given to the function.

   c, 1    char       1 byte
   d, 2    short      2 bytes
   l, 4    integer    4 bytes
   f       float      4 bytes
   s       string     NULL-terminated

 If a negative [client] id is given, all clients except -[client] will receive
 the message. Note that you can either be receiving or sending. The moment you
 send a new message, the one you were receiving is lost!
\******************************************************************************/
void N_send(int client, const char *format, ...)
{
        va_list va;
        const char *string;
        int string_len;

        /* Clients should never be sending messages to anyone but the server */
        C_assert(n_client_id == N_HOST_CLIENT_ID || client == N_SERVER_ID);

        /* Pack the message into the sync buffer */
        va_start(va, format);
        for (n_sync_size = 0; *format; format++)
                switch (*format) {
                case '1':
                case 'c':
                        if (n_sync_size > BUFFER_LEN - 1)
                                goto overflow;
                        n_sync_buffer[n_sync_size++] = (char)va_arg(va, int);
                        break;
                case '2':
                case 'd':
                        if (n_sync_size > BUFFER_LEN - 2)
                                goto overflow;
                        *(Uint16 *)(n_sync_buffer + n_sync_size) =
                                SDL_SwapLE16((Uint16)va_arg(va, int));
                        n_sync_size += 2;
                        break;
                case '4':
                case 'l':
                case 'f':
                        if (n_sync_size > BUFFER_LEN - 4)
                                goto overflow;
                        *(Uint32 *)(n_sync_buffer + n_sync_size) =
                                SDL_SwapLE32((Uint32)va_arg(va, int));
                        n_sync_size += 4;
                        break;
                case 's':
                        string = va_arg(va, const char *);
                        string_len = C_strlen(string) + 1;
                        if (string_len <= 1) {
                                if (n_sync_size > BUFFER_LEN - 1)
                                        goto overflow;
                                n_sync_buffer[n_sync_size++] = NUL;
                                break;
                        }
                        if (n_sync_size + string_len > BUFFER_LEN)
                               goto overflow;
                        memcpy(n_sync_buffer + n_sync_size, string, string_len);
                        n_sync_size += string_len;
                        break;
                default:
                        C_error("Invalid format character '%c'", *format);
                }
        va_end(va);

        /* If we are the server, deliver messages to the server and to our
           client directly */
        if (n_client_id == N_HOST_CLIENT_ID) {
                n_sync_pos = 0;
                if (client == N_SERVER_ID) {
                        n_server_func(N_HOST_CLIENT_ID, N_EV_MESSAGE);
                        return;
                }
                if (client == N_HOST_CLIENT_ID || client == N_BROADCAST_ID ||
                    (client < 0 && -client != n_client_id))
                        n_client_func(N_SERVER_ID, N_EV_MESSAGE);
        }

        /* TODO: Send messages over the network */

        return;

overflow:
        va_end(va);
        C_warning("Outgoing message buffer overflow");
}

