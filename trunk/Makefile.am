INCLUDES = -I.
LDADD = -lm @SDL_LIBS@
AM_CFLAGS = @SDL_CFLAGS@

bin_PROGRAMS = plutocracy

plutocracy_SOURCES = \
	src/plutocracy.c \
	src/common/c_shared.h \
	src/common/c_malloc.c \
	src/common/c_log.c \
	src/common/c_variables.c \
	src/common/c_string.c \
	src/common/c_array.c \
	src/platform/p_shared.h \
	src/platform/p_posix.c \
	src/platform/p_win32.c \
	src/network/n_shared.h \
	src/network/n_common.h \
	src/network/n_client.c \
	src/game/g_shared.h \
	src/game/g_common.h \
	src/render/r_shared.h \
	src/render/r_common.h \
	src/render/r_window.c \
	src/render/r_render.c \
	src/render/r_static_mesh.c \
	src/render/r_variables.c \
	src/interface/i_shared.h \
	src/interface/i_common.h

