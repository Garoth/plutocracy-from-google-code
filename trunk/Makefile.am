INCLUDES = -I.
LDADD = -lm @SDL_LIBS@
AM_CFLAGS = @SDL_CFLAGS@

bin_PROGRAMS = plutocracy

# All source and header files part of the project
plutocracy_SOURCES = \
	src/common/c_array.c \
	src/common/c_log.c \
	src/common/c_malloc.c \
	src/common/c_shared.h \
	src/common/c_string.c \
	src/common/c_time.c \
	src/common/c_variables.c \
	src/game/g_common.h \
	src/game/g_shared.h \
	src/game/g_sphere.c \
	src/interface/i_common.h \
	src/interface/i_shared.h \
	src/network/n_client.c \
	src/network/n_common.h \
	src/network/n_shared.h \
	src/platform/p_posix.c \
	src/platform/p_shared.h \
	src/platform/p_win32.c \
	src/render/r_common.h \
	src/render/r_render.c \
	src/render/r_shared.h \
	src/render/r_static_mesh.c \
	src/render/r_variables.c \
	src/render/r_window.c \
	src/plutocracy.c

# Distribute extra files with the tarball
EXTRA_DIST = \
        config/default.cfg

