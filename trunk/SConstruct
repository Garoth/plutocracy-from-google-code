#!/bin/python
#
################################################################################
# Plutocracy - Copyright (C) 2008 - Michael Levin
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
################################################################################
#
# While SCons supports Windows, this SConstruct file is for POSIX systems only!

import glob, os, sys

# Package parameters
package = 'plutocracy'
version = '0.0.0'

# Incompatible with Windows, use the Visual Studio project files
if (sys.platform == 'win32'):
        print ('\nWindows is not supported by the SCons script. Use the ' +
               'Visual Studio project files in the vc8 directory.')
        sys.exit(1)

# Options are loaded from 'custom.py'. Default values are only provided for
# the variables that do not get set by SCons itself.
config_file = ARGUMENTS.get('config', 'custom.py')
opts = Options(config_file)
opts.Add('CC', 'Compiler to use')
opts.Add('CFLAGS', 'Compiler flags')
opts.Add('PREFIX', 'Installation path prefix',
         os.environ.get('PREFIX', '/usr/local'))
opts.Add(BoolOption('gch', 'Build a precompiled header (gcc only)', True))

# Create a default environment
default_env = Environment(ENV = os.environ)
if ('CC' in os.environ):
        default_env.Replace(CC = os.environ['CC'])
if ('CFLAGS' in os.environ):
        default_env.Replace(CFLAGS = os.environ['CFLAGS'])
opts.Update(default_env)
opts.Save(config_file, default_env)

# Setup the help information
Help('\n' + package.title() + ' ' + version +
""" is built using SCons. See the README file for detailed
instructions.

Options are initialized from the environment first and can then be overridden
using either the command-line or the configuration file. Values only need to
be specified on the command line once and are then saved to the configuration
file. The filename of the configuration file can be specified using the
'config' command-line option:

  scons config='custom.py'

The following options are available:
""")
Help(opts.GenerateHelpText(default_env))

################################################################################
#
# scons plutocracy -- Compile plutocracy
#
################################################################################
plutocracy_src = ['src/plutocracy.c'] + glob.glob('src/*/*.c')
plutocracy_src.remove('src/common/c_os_windows.c')
plutocracy_env = default_env.Clone()
plutocracy_env.Append(CPPPATH = '.')
plutocracy_env.Append(LIBS = ['SDL_image', 'SDL_ttf', 'GL', 'GLU', 'z'])
plutocracy_env.ParseConfig('sdl-config --cflags --libs')
plutocracy_obj = plutocracy_env.Object(plutocracy_src)
plutocracy = plutocracy_env.Program(package, plutocracy_obj)
Default(plutocracy)

# Build the precompiled header as a dependency of the main program
plutocracy_gch = ''
if plutocracy_env['gch']:
        plutocracy_gch = 'src/common/c_shared.h.gch'
        plutocracy_env.Command('src/common/c_shared.h.gch',
                               glob.glob('src/common/*.h'),
                               '$CC $CFLAGS $CCFLAGS $_CCCOMCOM -x c-header ' +
                               '-c src/common/c_shared.h -o $TARGET')
        plutocracy_env.Depends(plutocracy_obj, plutocracy_gch)

# Generate a config.h with definitions
def WriteConfigH(target, source, env):
        config = open('config.h', 'w')
        config.write('\n/* Package parameters */\n' +
                     '#define PACKAGE "' + package + '"\n' +
                     '#define PACKAGE_STRING "' + package.title() +
                                                  ' ' + version + '"\n' +
                     '\n/* Configured paths */\n' +
                     '#define PKGDATADIR "' + install_data + '"\n')
        config.close()
plutocracy_config = plutocracy_env.Command('config.h', '', WriteConfigH)
plutocracy_env.Depends(plutocracy_obj + [plutocracy_gch], plutocracy_config)
plutocracy_env.Depends(plutocracy_config, config_file)

################################################################################
#
# scons install -- Install plutocracy
#
################################################################################
install_prefix = os.path.join(os.getcwd(), plutocracy_env['PREFIX'])
install_data = os.path.join(install_prefix, 'share/plutocracy')
Alias('install', install_prefix)
Depends('install', plutocracy)

def InstallRecursive(dest, src):
        files = os.listdir(src)
        for f in files:
                ext = f.rsplit('.')
                ext = ext[len(ext) - 1]
                if (f[0] == '.' or ext == 'o' or ext == 'obj' or ext == 'gch'):
                        continue
                src_path = os.path.join(src, f)
                if (os.path.isdir(src_path)):
                        InstallRecursive(os.path.join(dest, f), src_path)
                        continue
                Install(dest, src_path)

# Files that get installed
Install(os.path.join(install_prefix, 'bin'), plutocracy)
Install(install_data, ['AUTHORS', 'ChangeLog', 'CC', 'COPYING', 'README',
                       'TODO'])
InstallRecursive(os.path.join(install_data, 'gui/'), 'gui')
InstallRecursive(os.path.join(install_data, 'models/'), 'models')

################################################################################
#
# scons gendoc -- Generate automatic documentation
#
################################################################################
gendoc_env = default_env.Clone()
gendoc = gendoc_env.Program('gendoc/gendoc', glob.glob('gendoc/*.c'))

def GendocOutput(output, path, title, inputs = []):
        inputs = (glob.glob(path + '/*.h') + glob.glob(path + '/*.c') + inputs);
        cmd = gendoc_env.Command(output, inputs, 'gendoc/gendoc ' +
                                 '--header "gendoc/header.txt" ' +
                                 '--title "' + title + '" ' +
                                 ' '.join(inputs) + ' > $TARGET')
        gendoc_env.Depends(cmd, gendoc)

# Gendoc HTML files
GendocOutput('gendoc/docs/gendoc.html', 'gendoc', 'GenDoc')
GendocOutput('gendoc/docs/shared.html', 'src/common', 'Plutocracy Common',
             ['src/render/r_shared.h', 'src/interface/i_shared.h',
              'src/game/g_shared.h', 'src/network/n_shared.h'] +
             glob.glob('src/render/*.c') + glob.glob('src/interface/*.c') +
             glob.glob('src/game/*.c') + glob.glob('src/network/*.c'))
GendocOutput('gendoc/docs/render.html', 'src/render', 'Plutocracy Render')
GendocOutput('gendoc/docs/interface.html', 'src/interface',
             'Plutocracy Interface')
GendocOutput('gendoc/docs/game.html', 'src/game', 'Plutocracy Game')
GendocOutput('gendoc/docs/network.html', 'src/network', 'Plutocracy Network')

################################################################################
#
# scons dist -- Distributing the package tarball
#
################################################################################
dist_name = package + '-' + version
Depends(dist_name, 'gendoc')
dist_tarball = dist_name + '.tar.gz'
Command(dist_tarball, dist_name,
        [Delete(os.path.join(dist_name, 'gendoc/gendoc')),
         'tar -czf ' + dist_tarball + ' ' + dist_name])
Alias('dist', dist_tarball)
AddPostAction(dist_tarball, Delete(dist_name))

# Files that get distributed
Install(dist_name, ['AUTHORS', 'ChangeLog', 'CC', 'COPYING', 'README',
                    'SConstruct', 'todo.sh', 'TODO'])
InstallRecursive(os.path.join(dist_name, 'blender/'), 'blender')
InstallRecursive(os.path.join(dist_name, 'gendoc/'), 'gendoc')
InstallRecursive(os.path.join(dist_name, 'gui/'), 'gui')
InstallRecursive(os.path.join(dist_name, 'models/'), 'models')
InstallRecursive(os.path.join(dist_name, 'src/'), 'src')
InstallRecursive(os.path.join(dist_name, 'vc8/'), 'vc8')

