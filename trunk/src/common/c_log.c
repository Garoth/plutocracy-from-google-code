/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* This file implements the logging system */

#include "c_shared.h"

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
        if (!c_log_file.value.s[0])
                return;
        log_file = fopen(c_log_file.value.s, "r");
        if (log_file)
                C_debug("Opened log-file '%s'", c_log_file.value.s);
        else
                C_warning("Failed to open log-file '%s'",
                          c_log_file.value.s);
}

/******************************************************************************\
 Prints a string to the log file or to standard output. The output detail
 can be controlled using [c_log_level]. Debug calls without any text are
 considered traces.
\******************************************************************************/
void C_log(c_log_level_t level, const char *file, int line,
           const char *function, const char *fmt, ...)
{
        char fmt2[128];
        va_list va;

        if (level >= C_LOG_DEBUG && (!fmt || !fmt[0]))
                level = C_LOG_TRACE;
        if (level > C_LOG_ERROR && level > c_log_level.value.n)
                return;
        va_start(va, fmt);
        if (c_log_level.value.n <= C_LOG_STATUS) {
                if (level >= C_LOG_STATUS)
                        snprintf(fmt2, sizeof(fmt2), "%s\n", fmt);
                else if (level == C_LOG_WARNING)
                        snprintf(fmt2, sizeof(fmt2), "* %s\n", fmt);
                else
                        snprintf(fmt2, sizeof(fmt2), "*** %s\n", fmt);
        } else if (c_log_level.value.n == C_LOG_DEBUG) {
                if (level >= C_LOG_DEBUG)
                        snprintf(fmt2, sizeof(fmt2), "| %s(): %s\n",
                                 function, fmt);
                else if (level == C_LOG_STATUS)
                        snprintf(fmt2, sizeof(fmt2), "\n%s(): %s --\n",
                                 function, fmt);
                else if (level == C_LOG_WARNING)
                        snprintf(fmt2, sizeof(fmt2), "* %s(): %s\n",
                                 function, fmt);
                else
                        snprintf(fmt2, sizeof(fmt2), "*** %s(): %s\n",
                                 function, fmt);
        } else if (c_log_level.value.n >= C_LOG_TRACE) {
                if (level >= C_LOG_TRACE)
                        snprintf(fmt2, sizeof(fmt2), ": %s:%d, %s()\n",
                                 file, line, function);
                else if (level == C_LOG_DEBUG)
                        snprintf(fmt2, sizeof(fmt2), "| %s:%d, %s(): %s\n",
                                 file, line, function, fmt);
                else if (level == C_LOG_STATUS)
                        snprintf(fmt2, sizeof(fmt2), "\n%s:%d, %s(): %s --\n",
                                 file, line, function, fmt);
                else if (level == C_LOG_WARNING)
                        snprintf(fmt2, sizeof(fmt2), "* %s:%d, %s(): %s\n",
                                 file, line, function, fmt);
                else
                        snprintf(fmt2, sizeof(fmt2), "*** %s:%d, %s(): %s\n",
                                 file, line, function, fmt);
        }
        vfprintf(log_file ? log_file : stderr, fmt2, va);
        va_end(va);
        if (level == C_LOG_ERROR)
                abort();
}

