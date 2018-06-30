AC_PREREQ([2.69])
AC_INIT([nngn], [1])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([src/config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Main program name (native vs. WASM).
NNGN_BIN=nngn${EXEEXT}
case "$CXX" in *em++*) NNGN_BIN=${NNGN_BIN}.js;; esac
AC_SUBST([NNGN_BIN])

# Library support.
AC_ARG_WITH([opengl],
    [AS_HELP_STRING([--with-opengl], [enable the OpenGL graphics backend])],
    [], [])
AS_IF([test "x$with_opengl" == xyes],
    [DEPS="$DEPS glfw3 gl glew"
     AC_DEFINE([NNGN_PLATFORM_HAS_GLFW], [1], [Define if you have GLFW])
     AC_DEFINE([NNGN_PLATFORM_HAS_OPENGL], [1], [Define if you have OpenGL])],
    [])

AC_ARG_WITH([vulkan],
    [AS_HELP_STRING([--with-vulkan], [enable the Vulkan graphics backend])],
    [], [])
AS_IF([test "x$with_vulkan" == xyes],
    [DEPS="$DEPS glfw3 vulkan"
     GLSLANG=glslangValidator
     AC_DEFINE([NNGN_PLATFORM_HAS_GLFW], [1], [Define if you have GLFW])
     AC_DEFINE([NNGN_PLATFORM_HAS_VULKAN], [1], [Define if you have Vulkan])],
    [GLSLANG=:])
AC_SUBST([GLSLANG])

AC_ARG_WITH([vma],
    [AS_HELP_STRING([--with-vma], [use the Vulkan memory allocation library])],
    [], [AC_CHECK_HEADER(
        [vk_mem_alloc.h],
        [AC_DEFINE([NNGN_PLATFORM_HAS_VMA], [1], [Define if you have VMA])])])
AS_IF([test "x$with_vma" == xyes],
    [AC_DEFINE([NNGN_PLATFORM_HAS_VMA], [1], [Define if you have VMA])])

AC_ARG_WITH([libpng],
    [AS_HELP_STRING([--with-libpng], [enable loading PNG images])],
    [], [])
AS_IF([test "x$with_libpng" == xyes],
    [DEPS="$DEPS libpng"
     AC_DEFINE([NNGN_PLATFORM_HAS_LIBPNG], [1], [Define if you have libpng])],
    [])
AM_CONDITIONAL([ENABLE_LIBPNG], [test "x$with_libpng" == xyes])

AC_ARG_WITH([freetype2],
    [AS_HELP_STRING([--with-freetype2], [enable fonts])],
    [], [])
AS_IF([test "x$with_freetype2" == xyes],
    [DEPS="$DEPS freetype2"
     AC_DEFINE(
        [NNGN_PLATFORM_HAS_FREETYPE2], [1], [Define if you have freetype2])],
    [])
AM_CONDITIONAL([ENABLE_FREETYPE2], [test "x$with_freetype2" == xyes])

# Non-essential configuration options.
ENABLE_TESTS=no
AC_ARG_ENABLE([tests],
    [AS_HELP_STRING(
        [--enable-tests],
        [build tests, requires Qt (default: no)])],
    [ENABLE_TESTS=$enableval])
AM_CONDITIONAL([ENABLE_TESTS], [test "x$ENABLE_TESTS" == xyes])

ENABLE_BENCHMARKS=no
AC_ARG_ENABLE([benchmarks],
    [AS_HELP_STRING(
        [--enable-benchmarks],
        [build benchmarks, requires Qt (default: no)])],
    [ENABLE_BENCHMARKS=$enableval])
AM_CONDITIONAL([ENABLE_BENCHMARKS], [test "x$ENABLE_BENCHMARKS" == xyes])

ENABLE_LUA_ALLOC=no
AC_ARG_ENABLE([lua-alloc],
    [AS_HELP_STRING(
        [--enable-lua-alloc],
        [use own Lua allocator, required for memory tracking (default: no)])],
    [ENABLE_LUA_ALLOC=$enableval])
AM_CONDITIONAL([ENABLE_LUA_ALLOC], [test "x$ENABLE_LUA_ALLOC" == xyes])
AS_IF([test "$ENABLE_LUA_ALLOC" != no],
    AC_DEFINE([NNGN_LUA_USE_ALLOC], [1], [Define to enable Lua allocator]))

ENABLE_TOOLS=no
AC_ARG_ENABLE([tools],
    [AS_HELP_STRING(
        [--enable-tools],
        [build auxiliary tools, requires Qt and rustc (default: no)])],
    [ENABLE_TOOLS=$enableval])
AM_CONDITIONAL([ENABLE_TOOLS], [test "x$ENABLE_TOOLS" == xyes])

ENABLE_LUA_ALLOC=no
AC_ARG_ENABLE([lua-alloc],
    [AS_HELP_STRING(
        [--enable-lua-alloc],
        [use own Lua allocator, required for memory tracking (default: no)])],
    [ENABLE_LUA_ALLOC=$enableval])
AM_CONDITIONAL([ENABLE_LUA_ALLOC], [test "x$ENABLE_LUA_ALLOC" == xyes])
AS_IF([test "$ENABLE_LUA_ALLOC" != no],
    AC_DEFINE([NNGN_LUA_USE_ALLOC], [1], [Define to enable Lua allocator]))

# Checks for libraries.
PKG_CHECK_MODULES(
    [DEPS], [$DEPS lua sol2], [],
    [PKG_CHECK_MODULES([DEPS], [$DEPS lua-5.4 sol2], [])])
DEPS_CFLAGS=$(echo "$DEPS_CFLAGS" | sed s/-I/-isystem/g)

AS_IF([test "$ENABLE_TESTS" != no],
    [PKG_CHECK_MODULES(
        [TEST_DEPS], [Qt5Core Qt5Test],
        [TEST_DEPS_CFLAGS=$(echo "$TEST_DEPS_CFLAGS" | sed s/-I/-isystem/g)])])
AS_IF([test "$ENABLE_TOOLS" != no],
    [PKG_CHECK_MODULES(
        [QT_DEPS], [Qt5Charts Qt5Core Qt5Network Qt5Widgets],
        [QT_DEPS_CFLAGS=$(echo "$QT_DEPS_CFLAGS" | sed s/-I/-isystem/g)])])

# Checks for header files.
AC_CHECK_HEADERS([termios.h])

# Checks for library functions.
AC_FUNC_STRERROR_R
AC_CHECK_FUNCS([setenv signal])
AC_CHECK_FUNCS([socket],
    AC_DEFINE([NNGN_PLATFORM_HAS_SOCKETS], [1], [Define if you have sockets]))

AC_CONFIG_COMMANDS(src/glsl/gl/mkdir)
AC_CONFIG_COMMANDS(src/glsl/vk/mkdir)
AC_CONFIG_COMMANDS(tools/bin/mkdir)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
