INCLUDES =
LDADD = -lm @SDL_LIBS@
AM_CFLAGS = @SDL_CFLAGS@

bin_PROGRAMS = plutocracy

# All source and header files part of the project
plutocracy_SOURCES = \
        src/common/c_log.c \
        src/common/c_memory.c \
	src/common/c_noise.c \
        src/common/c_precompiled.h \
        src/common/c_shared.h \
        src/common/c_string.c \
        src/common/c_time.c \
        src/common/c_variables.c \
        src/game/g_common.h \
        src/game/g_shared.h \
	src/game/g_globe.c \
        src/interface/i_common.h \
        src/interface/i_shared.h \
        src/network/n_client.c \
        src/network/n_common.h \
        src/network/n_shared.h \
        src/platform/p_posix.c \
        src/platform/p_shared.h \
        src/platform/p_win32.c \
        src/render/r_assets.c \
        src/render/r_common.h \
        src/render/r_model.c \
        src/render/r_render.c \
        src/render/r_shared.h \
        src/render/r_sprite.c \
        src/render/r_static_mesh.c \
        src/render/r_variables.c \
        src/plutocracy.c

# Precompiled header rules
${plutocracy_OBJECTS}: ${srcdir}/src/common/c_shared.h.gch
${srcdir}/src/common/c_shared.h.gch: ${srcdir}/src/common/c_shared.h \
                                     ${srcdir}/src/common/c_vectors.h config.h
	${CC} $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CFLAGS) $(CFLAGS) \
	      -x c-header -c ${srcdir}/src/common/c_shared.h \
	      -o ${srcdir}/src/common/c_shared.h.gch
CLEANFILES = src/common/c_shared.h.gch

# Distribute extra files with the tarball
EXTRA_DIST = \
        config/default.cfg

