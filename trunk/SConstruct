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

# Platform considerations
windows = sys.platform == 'win32'

# Convert a POSIX path to OS-independent
def path(p):
        if windows:
                p = p.replace('/', '\\')
        return p

# Create a default environment. Have to set environment variables after
# initialization so that SCons doesn't mess them up.
default_env = Environment(ENV = os.environ)

# Adds an option that may have been set via environment flags
def AddEnvOption(env, opts, opt_name, opt_desc, env_name = None, value = None):
        if env_name == None:
                env_name = opt_name;
        if env_name == opt_name:
                opt_desc += ' (environment variable)'
        else:
                opt_desc += ' (environment variable ' + env_name + ')'
        if env_name in os.environ:
                opts.Add(opt_name, opt_desc, os.environ[env_name])
        elif value == None:
                opts.Add(opt_name, opt_desc, env.get(opt_name))
        else:
                opts.Add(opt_name, opt_desc, value)

# Options are loaded from 'custom.py'. Default values are only provided for
# the variables that do not get set by SCons itself.
config_file = ARGUMENTS.get('config', 'custom.py')
opts = Options(config_file)

# First load the mingw variable because it affects default variables
opts.Add(BoolOption('mingw', 'Set to True if compiling with MinGW', False))
opts.Update(default_env)
mingw = default_env.get('mingw')
if mingw:
        default_env = Environment(tools = ['mingw'], ENV = os.environ)

# Now load the rest of the options
AddEnvOption(default_env, opts, 'CC', 'Compiler to use')
AddEnvOption(default_env, opts, 'CFLAGS', 'Compiler flags')
AddEnvOption(default_env, opts, 'LINK', 'Linker to use')
AddEnvOption(default_env, opts, 'LINKFLAGS', 'Linker flags', 'LDFLAGS')
AddEnvOption(default_env, opts, 'PREFIX', 'Installation path prefix', 
             'PREFIX', '/usr/local/')
opts.Add(BoolOption('gch', 'Build a precompiled header (gcc only)', True))
opts.Add(BoolOption('dump_env', 'Dump SCons Environment contents', False))
opts.Add(BoolOption('dump_path', 'Dump path enviornment variable', False))
opts.Update(default_env)
opts.Save(config_file, default_env)

# Windows compiler and linker need some extra flags
if windows:
        if default_env['CC'] == 'cl':
                default_env.PrependUnique(CFLAGS = '/MD')
        if default_env['LINK'] == 'link':
                default_env.PrependUnique(LINKFLAGS = '/SUBSYSTEM:CONSOLE')

# Dump Environment
if default_env['dump_env']:
        print default_env.Dump()
if default_env['dump_path']:
        print default_env['ENV']['PATH']

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
plutocracy_src = ([path('src/plutocracy.c')] + glob.glob(path('src/*/*.c')))
plutocracy_env = default_env.Clone()
plutocracy_objlibs = ''
if windows:
        plutocracy_src.remove(path('src/common/c_os_posix.c'))
        plutocracy_env.Append(CPPPATH = 'windows/include')
        plutocracy_env.Append(LIBPATH = 'windows/lib')
        plutocracy_env.Append(LIBS = ['SDL', 'SDLmain', 'OpenGL32', 'GlU32', 
                                      'user32', 'shell32'])
        if mingw:
                plutocracy_env.ParseConfig('sh sdl-config --prefix=windows' +
                                                        ' --cflags --libs')
                plutocracy_objlibs = [path('windows/lib/zdll.lib'),
                                      path('windows/lib/SDL_ttf.lib'),
                                      path('windows/lib/SDL_image.lib')]
        else:
                plutocracy_env.Append(LIBS = ['SDL_image', 'SDL_ttf', 'zdll'])
else:
        plutocracy_src.remove(path('src/common/c_os_windows.c'))
        plutocracy_env.Append(CPPPATH = '.')
        plutocracy_env.Append(LIBS = ['SDL_image', 'SDL_ttf', 'GL', 'GLU', 'z'])
        plutocracy_env.ParseConfig('sdl-config --cflags --libs')
plutocracy_obj = plutocracy_env.Object(plutocracy_src)
plutocracy = plutocracy_env.Program(package, plutocracy_obj +
                                             plutocracy_objlibs)
if windows:
        plutocracy_env.Clean(plutocracy, 'plutocracy.exe.manifest')
Default(plutocracy)

# Build the precompiled header as a dependency of the main program
plutocracy_gch = ''
if plutocracy_env['gch'] and not windows:
        plutocracy_gch = 'src/common/c_shared.h.gch'
        plutocracy_env.Command('src/common/c_shared.h.gch',
                               glob.glob('src/common/*.h'),
                               '$CC $CFLAGS $CCFLAGS $_CCCOMCOM -x c-header ' +
                               '-c src/common/c_shared.h -o $TARGET')
        plutocracy_env.Depends(plutocracy_obj, plutocracy_gch)

# Windows has a prebuilt config
if windows:
        plutocracy_config = 'windows/include/config.h'

# Generate a config.h with definitions
else:
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

plutocracy_env.Depends(plutocracy_obj + [plutocracy_gch], 
                       plutocracy_config)
plutocracy_env.Depends(plutocracy_config, config_file)
        
################################################################################
#
# scons install -- Install plutocracy
#
################################################################################
install_prefix = os.path.join(os.getcwd(), plutocracy_env['PREFIX'])
install_data = os.path.join(install_prefix, 'share', 'plutocracy')
default_env.Alias('install', install_prefix)
default_env.Depends('install', plutocracy)

def InstallRecursive(dest, src, exclude = []):
        bad_exts = ['exe', 'o', 'obj', 'gch', 'pch', 'user', 'ncb', 'suo', 
                    'manifest']
        files = os.listdir(src)
        for f in files:
                ext = f.rsplit('.')
                ext = ext[len(ext) - 1]
                if (f[0] == '.' or ext in bad_exts):
                        continue
                src_path = os.path.join(src, f)
                if src_path in exclude:
                        continue
                if (os.path.isdir(src_path)):
                        InstallRecursive(os.path.join(dest, f), src_path)
                        continue
                default_env.Install(dest, src_path)

# Files that get installed
default_env.Install(os.path.join(install_prefix, 'bin'), plutocracy)
default_env.Install(install_data, ['AUTHORS', 'ChangeLog', 'CC', 'COPYING', 
                                   'README'])
InstallRecursive(os.path.join(install_data, 'gui'), 'gui')
InstallRecursive(os.path.join(install_data, 'models'), 'models')

################################################################################
#
# scons gendoc -- Generate automatic documentation
#
################################################################################
gendoc_env = default_env.Clone()
gendoc_header = path('gendoc/header.txt')
gendoc = gendoc_env.Program(path('gendoc/gendoc'),
                            glob.glob(path('gendoc/*.c')))
if windows:
        gendoc_env.Clean(gendoc, path('gendoc/gendoc.exe.manifest'))

def GendocOutput(output, in_path, title, inputs = []):
        output = os.path.join('gendoc', 'docs', output)
        inputs = (glob.glob(os.path.join(in_path, '*.h')) +
                  glob.glob(os.path.join(in_path, '*.c')) + inputs);
        cmd = gendoc_env.Command(output, inputs, path('gendoc/gendoc') +
                                 ' --file $TARGET --header "' + 
                                 gendoc_header + '" --title "' + title + '" ' +
                                 ' '.join(inputs))
        gendoc_env.Depends(cmd, gendoc)
        gendoc_env.Depends(cmd, gendoc_header)

# Gendoc HTML files
GendocOutput('gendoc.html', 'gendoc', 'GenDoc')
GendocOutput('shared.html', path('src/common'), 'Plutocracy Common',
             [path('src/render/r_shared.h'),
              path('src/interface/i_shared.h'),
              path('src/game/g_shared.h'),
              path('src/network/n_shared.h')] +
             glob.glob(path('src/render/*.c')) +
             glob.glob(path('src/interface/*.c')) +
             glob.glob(path('src/game/*.c')) +
             glob.glob(path('src/network/*.c')))
GendocOutput('render.html', path('src/render'), 'Plutocracy Render')
GendocOutput('interface.html', path('src/interface'),
             'Plutocracy Interface')
GendocOutput('game.html', path('src/game'), 'Plutocracy Game')
GendocOutput('network.html', path('src/network'), 'Plutocracy Network')

################################################################################
#
# scons dist -- Distributing the package tarball
#
################################################################################
dist_name = package + '-' + version
dist_tarball = dist_name + '.tar.gz'
default_env.Depends(dist_name, 'gendoc')
default_env.Command(dist_tarball, dist_name,
                    ['tar -czf ' + dist_tarball + ' ' + dist_name])
default_env.Alias('dist', dist_tarball)
default_env.AddPostAction(dist_tarball, Delete(dist_name))

# Files that get distributed
default_env.Install(dist_name, ['AUTHORS', 'ChangeLog', 'CC', 'COPYING', 
                                'README', 'SConstruct', 'todo.sh'])
InstallRecursive(os.path.join(dist_name, 'blender'), 'blender')
InstallRecursive(os.path.join(dist_name, 'gendoc'), 'gendoc', 
                 [path('gendoc/gendoc')])
InstallRecursive(os.path.join(dist_name, 'gui'), 'gui')
InstallRecursive(os.path.join(dist_name, 'models'), 'models')
InstallRecursive(os.path.join(dist_name, 'src'), 'src')
InstallRecursive(os.path.join(dist_name, 'windows'), 'windows',
                 ['windows/vc8/Debug', 'windows/vc8/Release'])
