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

GAME_SHARED=../src/game/g_shared.h
INTERFACE_SHARED=../src/interface/i_shared.h ../src/interface/*.c
INTERFACE_FILES=../src/interface/i_common.h $(INTERFACE_SHARED)
RENDER_SHARED=../src/render/r_shared.h ../src/render/*.c
RENDER_FILES=../src/render/r_common.h $(RENDER_SHARED)
GAME_SHARED=../src/game/g_shared.h ../src/game/*.c
GAME_FILES=../src/game/g_common.h $(GAME_SHARED)
NETWORK_SHARED=../src/network/n_shared.h ../src/network/*.c
NETWORK_FILES=../src/network/n_common.h $(NETWORK_SHARED)
COMMON_FILES=../src/common/c_*.h ../src/common/*.c
SHARED_FILES=$(COMMON_FILES) $(RENDER_SHARED) $(INTERFACE_SHARED) $(GAME_SHARED)
GENDOC_FILES=gendoc.h gendoc*.c

CFLAGS=-g -Wall
SOURCES=gendoc.c gendoc_entry.c gendoc_parse.c

all: docs/gendoc.html docs/shared.html docs/interface.html docs/render.html \
     docs/game.html docs/network.html

clean:
	rm -f gendoc

.PHONY: clean

docs/gendoc.html: gendoc $(GENDOC_FILES) Makefile
	./gendoc $(GENDOC_FILES) > docs/gendoc.html

docs/shared.html: gendoc $(SHARED_FILES) Makefile
	./gendoc --title "Plutocracy Shared" \
	         $(SHARED_FILES) > docs/shared.html

docs/interface.html: gendoc $(INTERFACE_FILES) Makefile
	./gendoc --title "Plutocracy Interface" \
	         $(INTERFACE_FILES) > docs/interface.html

docs/render.html: gendoc $(RENDER_FILES) Makefile
	./gendoc --title "Plutocracy Render" \
	         $(RENDER_FILES) > docs/render.html

docs/game.html: gendoc $(GAME_FILES) Makefile
	./gendoc --title "Plutocracy Game" \
	         $(GAME_FILES) > docs/game.html

docs/network.html: gendoc $(NETWORK_FILES) Makefile
	./gendoc --title "Plutocracy Network" \
	         $(NETWORK_FILES) > docs/network.html

gendoc: $(SOURCES) gendoc.h Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $(SOURCES) -o gendoc

