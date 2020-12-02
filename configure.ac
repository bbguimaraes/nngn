AC_PREREQ([2.69])
AC_INIT([impero], [0.0.1])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([src/main.cpp])

ENABLE_TESTS=no
AC_ARG_ENABLE([tests],
    [AS_HELP_STRING(
        [--enable-tests],
        [build tests, requires Qt (default: no)])],
    [ENABLE_TESTS=$enableval])
AM_CONDITIONAL([ENABLE_TESTS], [test "x$ENABLE_TESTS" == xyes])

# Checks for programs.
AC_PROG_CXX

# Checks for libraries.
PKG_CHECK_MODULES(
    [DEPS], [Qt5Core Qt5Widgets],
    [DEPS_CFLAGS=$(echo "$DEPS_CFLAGS" | sed s/-I/-isystem/g)])

AS_IF([test "$ENABLE_TESTS" != no],
    [PKG_CHECK_MODULES(
        [TEST_DEPS], [Qt5Test],
        [TEST_DEPS_CFLAGS=$(echo "$TEST_DEPS_CFLAGS" | sed s/-I/-isystem/g)])])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
