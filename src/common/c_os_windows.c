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
 Creates directory if it does not already exist. Returns TRUE if the directory
 exists after the call.
\******************************************************************************/
int C_mkdir(const char *path)
{
        if (!_mkdir(path))
                C_debug("Created directory '%s'", path);
        else if (errno != EEXIST) {
                C_warning("Failed to create: %s", strerror(errno));
                return FALSE;
        }
        return TRUE;
}

/******************************************************************************\
 Returns the path to the user's writeable Plutocracy directory.
\******************************************************************************/
const char *C_user_dir(void)
{
        static char user_dir[256];

        if (!user_dir[0]) {
                TCHAR app_data[MAX_PATH];
        
                if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, app_data) != 
                    S_OK) {
                        C_warning("Failed to get Application Data directory");
                        return "";
                }
                snprintf(user_dir, sizeof (user_dir),
                         "%s/" PACKAGE, app_data);
                C_debug("Home directory is '%s'", user_dir);
                C_mkdir(user_dir);
        }
        return user_dir;
}
