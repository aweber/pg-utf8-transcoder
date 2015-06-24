# init autoconfig and automake
AC_INIT([PostgreSQL UTF8 Transcoder], [0.1], [bobl@aweber.com])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])

# set config source directory
AC_CONFIG_SRCDIR([configure.ac])

# check for programs
AC_PROG_CC

# checking for ICU
AC_CHECK_HEADER([unicode/ucnv.h], [], [AC_MSG_ERROR([*** unicode headers are required, install ICU development files])])
AC_PATH_PROG([ICU_CONFIG], [icu-config], [AC_MSG_ERROR([*** icu-config is required, install ICU tools])])

# set cflags and ldflags for icu
ICU_CPPFLAGS="$($ICU_CONFIG --cppflags)"
ICU_LDFLAGS="$($ICU_CONFIG --ldflags)"
AC_SUBST(ICU_CPPFLAGS)
AC_SUBST(ICU_LDFLAGS)

# checking for postgresql.
AC_CHECK_HEADER([postgresql/libpq-fe.h], [], [AC_MSG_ERROR([*** PostgreSQL header files are required, install PostgreSQL development files])])
AC_PATH_PROG([PG_CONFIG], [pg_config], [AC_MSG_ERROR([*** pg_config is required, install PostgreSQL tools])])

# set cflags and ldflags for postgresql
PGSQL_CPPFLAGS="-I$($PG_CONFIG --includedir)"
PGSQL_LDFLAGS="-L$($PG_CONFIG --pkglibdir) -lpq"
AC_SUBST(PGSQL_CPPFLAGS)
AC_SUBST(PGSQL_LDFLAGS)

# config files to generate
AC_CONFIG_FILES([Makefile src/Makefile])

# make 'em
AC_OUTPUT
