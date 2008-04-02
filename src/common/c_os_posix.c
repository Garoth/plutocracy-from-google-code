/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* This file should only be included on POSIX-compliant systems */

#include "c_shared.h"
#include <errno.h>
#include <sys/stat.h>

/******************************************************************************\
 Returns the path to the user's writeable Plutocracy directory. The directory
 is not returned with a trailing slash.
\******************************************************************************/
const char *C_user_dir(void)
{
        static char user_dir[256];

        if (!user_dir[0]) {
                snprintf(user_dir, sizeof (user_dir),
                         "%s/." PACKAGE, getenv("HOME"));
                C_debug("Home directory is '%s'", user_dir);
                if (!mkdir(user_dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
                        C_debug("Directory created");
                else if (errno != EEXIST)
                        C_warning("Failed to create: %s", strerror(errno));
        }
        return user_dir;
}

