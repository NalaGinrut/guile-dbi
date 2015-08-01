# configure.ac - Configure script for the guile-dbi.
# Copyright (C) 2004, 2005 Free Software Foundation, Inc.
# Written by Maurizio Boriani <baux@member.fsf.org>
#
# This file is part of the guile-dbi.
# 
# The guile-dbi is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# The guile-dbi is distributed in the hope that it will be useful,
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
. $srcdir/DBI-VERSION
AM_INIT_AUTOMAKE($PACKAGE,$VERSION,bug-guile@gnu.org)
AM_MAINTAINER_MODE
AC_CONFIG_HEADER([config.h])
AC_GNU_SOURCE

maintainer_mode=${enable_maintainer_mode-no}

# Checks for programs.
AC_PROG_CC
AM_PROG_AS
AC_PROG_INSTALL
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_CHECK_TOOL([AR], [ar], :)
AC_CHECK_TOOL([NM], [nm], :)
AC_CHECK_TOOL([GREG], [greg], :)
AC_PATH_PROG([GUILESNARF], [guile-snarf], :)
AC_PATH_PROG([GUILECONFIG], [guile-config], :)

# Checks for typedefs, structures, and compiler characteristics.
AC_LANG([C])
AC_CANONICAL_HOST
AC_C_CONST
AC_C_VOLATILE
AC_TYPE_SIZE_T
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strerror])

# ltmain needs these:
AC_CONFIG_MACRO_DIR([m4])
LT_INIT
# AC_PROG_INSTALL
# AC_PROG_MAKE_SET
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])], [AC_SUBST([AM_DEFAULT_VERBOSITY],1)])


CFLAGS=`$GUILECONFIG compile`
LIBS=`$GUILECONFIG link`
GUILE_SITE=`$GUILECONFIG info sitedir`

AC_ARG_WITH([guile-site-dir],
	[AS_HELP_STRING([--with-guile-site-dir=DIR],
	    [use the specified installtion path for Guile modules [default=auto]]
	)],
	[guilesitedir="$withval"],
	[guilesitedir="$GUILE_SITE"]
)

AC_SUBST(DBI_MAJOR_VERSION)
AC_SUBST(DBI_MINOR_VERSION)
AC_SUBST(DBI_MICRO_VERSION)
AC_SUBST(DBI_INTERFACE)
AC_SUBST(VERSION)
AC_SUBST(guilesitedir)


# Checks for library functions.
AC_CONFIG_FILES([Makefile
		 guile-dbi.pc
		 src/Makefile
		 src/dbi/Makefile
		 src/test/Makefile
		 include/Makefile
		 include/guile-dbi/Makefile
		 doc/Makefile])

AC_CONFIG_FILES([src/test/check-guile-dbi],
	[chmod ugo+x src/test/check-guile-dbi])

AC_OUTPUT

dnl ====================================================================
echo "
$PACKAGE-$VERSION

	prefix:      ${prefix}
	sitedir:     ${guilesitedir}
	compiler:    ${CC} ${CPPFLAGS} ${CFLAGS}
	link:        ${LIBS}
"
