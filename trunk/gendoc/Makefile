################################################################################
# Plutocracy Gendoc - Copyright (C) 2008 - Michael Levin
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
################################################################################

CFLAGS=-g -Wall -Werror
GAME_SHARED=../src/game/g_shared.h
INTERFACE_SHARED=../src/interface/i_shared.h ../src/interface/*.c
INTERFACE_FILES=../src/interface/i_common.h $(INTERFACE_SHARED)
RENDER_SHARED=../src/render/r_shared.h ../src/render/*.c
RENDER_FILES=../src/render/r_common.h $(RENDER_SHARED)
SHARED_FILES=../src/common/c_shared.h $(RENDER_SHARED) $(INTERFACE_SHARED)
TEST_FILES=test/test.h test/test.c

all: gendoc
# test/test.html
# docs/shared.html docs/interface.html docs/render.html

clean:
	rm -f gendoc

.PHONY: clean

test/test.html: gendoc $(TEST_FILES)
	./gendoc test/test.html $(TEST_FILES)

docs/shared.html: gendoc $(SHARED_FILES)
	./gendoc docs/shared.html $(SHARED_FILES)

docs/interface.html: gendoc $(INTERFACE_FILES)
	./gendoc docs/interface.html $(INTERFACE_FILES)

docs/render.html: gendoc
	./gendoc docs/render.html $(RENDER_FILES)

gendoc: gendoc.c
	$(CC) $(CFLAGS) $(LDFLAGS) gendoc.c -o gendoc

