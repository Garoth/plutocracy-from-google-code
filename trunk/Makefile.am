INCLUDES =
LDADD = -lm -lz @SDL_LIBS@
AM_CFLAGS = @SDL_CFLAGS@ -DLOCALEDIR=\""$(datadir)/locale"\"
SUBDIRS = po

bin_PROGRAMS = plutocracy

# All source and header files part of the project
plutocracy_SOURCES = \
        src/common/c_file.c \
        src/common/c_log.c \
        src/common/c_memory.c \
        src/common/c_os_posix.c \
        src/common/c_rand.c \
        src/common/c_shared.h \
        src/common/c_string.c \
        src/common/c_time.c \
        src/common/c_variables.c \
        src/common/c_vectors.h \
        src/game/g_common.h \
        src/game/g_shared.h \
        src/game/g_globe.c \
        src/game/g_variables.c \
        src/interface/i_button.c \
        src/interface/i_common.h \
        src/interface/i_console.c \
        src/interface/i_entry.c \
        src/interface/i_game.c \
        src/interface/i_layout.c \
        src/interface/i_shared.h \
        src/interface/i_variables.c \
        src/interface/i_widgets.c \
        src/network/n_client.c \
        src/network/n_common.h \
        src/network/n_shared.h \
        src/render/r_assets.c \
        src/render/r_common.h \
        src/render/r_globe.c \
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
EXTRA_DIST = config.rpath m4/ChangeLog  gui models blender config vc8 gendoc

# We don't want to distrubute SVN cache, delete it out of the packaging
# directories before creating the tarball
dist-hook:
	rm -rf `find $(distdir) -name .svn`


ACLOCAL_AMFLAGS = -I m4
