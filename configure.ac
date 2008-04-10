#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT([Plutocracy], [0.0.0], [risujin@risujin.org])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/plutocracy.c])
AC_CONFIG_HEADER([config.h])

# Checks for programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_MKDIR_P

# Checks for standard C header files
AC_HEADER_STDC

# Checks for typedefs, structures, and compiler characteristics
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T

# Check for SDL
SDL_VERSION=1.2.0
AM_PATH_SDL($SDL_VERSION, :,
            AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]))
AC_SUBST(SDL_CFLAGS)
AC_SUBST(SDL_LIBS)

# SDL extensions
AC_CHECK_LIB(SDL_image, IMG_Load, [],
             [AC_MSG_ERROR([*** -lSDL_image not found!])])
AC_CHECK_LIB(SDL_ttf, TTF_Init, [],
             [AC_MSG_ERROR([*** -lSDL_ttf not found!])])

# Check for GL and GLU
AC_CHECK_LIB(GL, glEnable, [], [AC_MSG_ERROR([*** -lGL not found!])])
AC_CHECK_LIB(GLU, gluPerspective, [], [AC_MSG_ERROR([*** -lGLU not found!])])

# Check for zlib
AC_CHECK_LIB(z, gzopen, [], [AC_MSG_ERROR([*** -lz not found!])])

# Check for various headers
AC_CHECK_HEADERS([strings.h])

# Checks for library functions
AC_TYPE_SIGNAL
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memset sqrt strcasecmp])

# Checks if '$CC -c -o' is allowed, needed for per-target flags
#AM_PROG_CC_C_O

# If configured with --enable-libc-errors will generate errors when standard
# library functions that have been overriden and should not appear in code
# are used. This may not work properly on all systems.
AC_ARG_ENABLE(libc-errors, [Generate errors for deprecated libc functions])
if test x$enable_libc_errors = xyes; then
      AC_DEFINE([PLUTOCRACY_LIBC_ERRORS], [],
                [Generate errors for deprecated libc functions])
fi

# Look for an external gettext (gettextize wants this)
AM_GNU_GETTEXT([external])

AC_OUTPUT(Makefile po/Makefile.in)

