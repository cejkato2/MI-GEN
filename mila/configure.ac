#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([mila-opt], [1beta], [tomas@kassoft.eu])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([main.cpp])
AC_CONFIG_HEADERS([config.h])
CXXFLAGS="-Wno-write-strings -g"

# Checks for programs.
AC_PROG_CXX
#AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AC_CONFIG_FILES([Makefile])

AC_OUTPUT
