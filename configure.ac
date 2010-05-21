# configure.ac - Configure script for the guile-dbd-postgresql.
# Copyright (C) 2004, 2005, 2006 Free Software Foundation, Inc.
# Written by Maurizio Boriani <baux@member.fsf.org>
#
# This file is part of the guile-dbd-postgresql.
#
# The guile-dbd-postgresql is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# The guile-dbd-postgresql is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
#
#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_INIT
. $srcdir/DBD-VERSION
AM_INIT_AUTOMAKE($PACKAGE,$VERSION,bug-guile@gnu.org)
AM_MAINTAINER_MODE
AC_CONFIG_HEADER([config.h])
AC_GNU_SOURCE

maintainer_mode=${enable_maintainer_mode-no}

AC_CONFIG_MACRO_DIR([m4])
LT_INIT

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
AM_PROG_AS
AC_PROG_LIBTOOL
AC_CHECK_TOOL([AR], [ar], :)
AC_CHECK_TOOL([NM], [nm], :)
AC_PATH_PROG([GUILECONFIG], [guile-config], :)


# Checks for typedefs, structures, and compiler characteristics.
AC_CANONICAL_HOST
AC_C_CONST
AC_C_VOLATILE

AC_CHECK_FUNCS([strerror])
AC_CHECK_FUNCS([strndup])
AC_CHECK_HEADERS([string.h])
AC_FUNC_MALLOC
AC_HEADER_STDC

# Checks for libs.
AC_CHECK_LIB(pq,PQconnectStart,,
        AC_MSG_ERROR([*** Can't find libpq]))

CFLAGS=`guile-config compile`
LIBS=`guile-config link`

AC_SUBST(DBD_MAJOR_VERSION)
AC_SUBST(DBD_MINOR_VERSION)
AC_SUBST(DBD_MICRO_VERSION)
AC_SUBST(DBD_INTERFACE)
AC_SUBST(VERSION)

AC_CONFIG_FILES([Makefile
		 src/Makefile])

AC_OUTPUT

dnl ====================================================================
echo "
$PACKAGE-$VERSION

   prefix:      ${prefix}
   compiler:    ${CC} ${CPPFLAGS} ${CFLAGS}
   link:        ${LIBS}
"

