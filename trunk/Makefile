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
# Wrapper around scons for gdb and people who can't read.

all:
	scons

dist:
	scons dist

clean:
	scons -c plutocracy gendoc
distclean: clean
realclean: clean

install:
	scons install prefix=$PREFIX

gendoc:
	scons gendoc

check:

distcheck:

.PHONY: all dist clean distclean realclean install gendoc check distcheck

