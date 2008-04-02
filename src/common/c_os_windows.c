/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* This file should only be included on Microsoft Windows systems */

#include "c_shared.h"
#include <errno.h>
#include <direct.h>
#include <shlobj.h>

/******************************************************************************\
 Returns the path to the user's writeable Plutocracy directory.
\******************************************************************************/
const char *C_user_dir(void)
{
        static char user_dir[256];

        if (!user_dir[0]) {
                TCHAR app_data[MAX_PATH];
        
                if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL,
                                    SHGFP_TYPE_CURRENT, app_data) != S_OK) {
                        C_warning("Failed to get Application Data directory");
                        return "";
                }
                snprintf(user_dir, sizeof (user_dir),
                         "%s/" PACKAGE, app_data);
                C_debug("Home directory is '%s'", user_dir);
                if (!_mkdir(user_dir))
                        C_debug("Directory created");
                else if (errno != EEXIST)
                        C_warning("Failed to create: %s", strerror(errno));
        }
        return user_dir;
}

