#!/bin/python
#
# Plutocracy - Copyright (C) 2008 - Michael Levin
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

# Note that while SCONS supports Windows, this SConstruct file is for POSIX
# systems only!

import glob

sources = glob.glob('src/*.c') + glob.glob('src/*/*.c')
sources.remove('src/common/c_os_windows.c')

env = Environment()
env.Append(CFLAGS = ['-g', '-Wall', '-DLOCALEDIR=\\"/usr/share/locale\\"'])
env.Append(LIBS = ['SDL_image', 'SDL_ttf', 'GL', 'GLU', 'z'])
env.ParseConfig('sdl-config --cflags')
env.ParseConfig('sdl-config --libs')
program = env.Program('plutocracy', sources)
env.Command('src/common/c_shared.h.gch', 'src/common/c_shared.h',
            '$CC $CFLAGS $CCFLAGS $_CCCOMCOM -x c-header -c $SOURCE -o $TARGET')
env.Depends(program, 'src/common/c_shared.h.gch')

