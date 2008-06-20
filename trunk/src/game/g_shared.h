/******************************************************************************\
 Plutocracy - Copyright (C) 2008 - Michael Levin

 This program is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License as published by the Free Software
 Foundation; either version 2, or (at your option) any later version.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
\******************************************************************************/

/* g_globe.c */
void G_cleanup_globe(void);
void G_init_globe(void);
void G_mouse_ray(c_vec3_t origin, c_vec3_t forward);
void G_process_click(int button);
void G_generate_globe(void);
void G_render_globe(void);

/* g_host.c */
void G_host_game(void);

/* g_variables.c */
void G_register_variables(void);

