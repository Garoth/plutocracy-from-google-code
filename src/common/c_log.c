/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

#include "c_shared.h"
#include <stdio.h>
#include <stdarg.h>

extern c_var_t c_log_level, c_log_file;

static FILE *log_file;

/******************************************************************************\
 Close the log file to conserve file handles.
\******************************************************************************/
void C_close_log_file(void)
{
        if (!log_file || log_file == stderr)
                return;
        C_debug("Closing log-file");
        fclose(log_file);
}

/******************************************************************************\
 Opens the log file or falls back to standard error output.
\******************************************************************************/
void C_open_log_file(void)
{
        if (c_log_file.value.s[0]) {
                log_file = fopen(c_log_file.value.s, "r");
                if (log_file)
                        C_debug("Opened log-file '%s'", c_log_file.value.s);
                else
                        C_warning("Failed to open log-file '%s'",
                                  c_log_file.value.s);
        }
        if (!log_file)
                log_file = stderr;
}

/******************************************************************************\
 Prints a string to the debug log file or to standard output.
\******************************************************************************/
void C_debug_full(const char *file, int line, const char *function,
                  const char *fmt, ...)
{
        char fmt2[128];
        va_list va;

        if (!log_file)
                return;
        va_start(va, fmt);
        snprintf(fmt2, sizeof(fmt2), "*** %s:%d, %s() -- %s\n",
                 file, line, function, fmt);
        vfprintf(log_file, fmt2, va);
        va_end(va);
}

