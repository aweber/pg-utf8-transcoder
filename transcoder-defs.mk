# Make definitions for transcoder
#
# Copyright © 2015, AWeber Communications
# All Rights Reserved.
#
# Please be sure to update config/Makefile.inc.in  if you add something here.
#

# Shell to use

SHELL = /bin/bash

# Standard directories

prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
datarootdir = ${prefix}/share
datadir = ${datarootdir}
libdir = ${exec_prefix}/lib
includedir = ${prefix}/include
mandir = ${datarootdir}/man
sysconfdir = ${prefix}/etc

# Package information

PACKAGE_DESCRIPTION = "PostgreSQL Charset Transcoder to UTF8"
#PACKAGE_ICU_URL = "http://icu-project.org"
PACKAGE = pg_transcoder
VERSION = 0.1

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 755
INSTALL_SCRIPT = ${INSTALL}

# Compiler and tools

ENABLE_DEBUG = 1
ENABLE_RELEASE = 1
EXEEXT =
CC = gcc
LD = gcc
AR = ar
ARFLAGS =  r
RANLIB = ranlib
COMPILE_LINK_ENVVAR =

# Various flags for the tools

# CFLAGS is for C only flags
CFLAGS = -g -O0  -std=c99 -Wall -pedantic -Wshadow -Wpointer-arith -Wmissing-prototypes -Wwrite-strings
# CPPFLAGS is for C Pre-Processor flags
CPPFLAGS = $(CPPFLAGS) -DDEBUG=1
# DEFAULT_LIBS are the default libraries to link against
DEFAULT_LIBS = -lpthread -ldl -lm
# LIB_M is for linking against the math library
LIB_M =
# LIB_THREAD is for linking against the threading library
LIB_THREAD =
# OUTOPT is for creating a specific output name
OUTOPT = -o # The extra space after the argument is needed.
# AR_OUTOPT is for creating a specific output name for static libraries.
AR_OUTOPT =

ENABLE_RPATH = NO
ifeq ($(ENABLE_RPATH),YES)
RPATHLDFLAGS = $(LD_RPATH)$(LD_RPATH_PRE)$(libdir)
endif
LDFLAGS =  $(RPATHLDFLAGS)

# Commands to compile
COMPILE.c=    $(CC) $(CPPFLAGS) $(DEFS) $(CFLAGS) -c

# Commands to link
LINK.c=       $(CC) $(CFLAGS) $(LDFLAGS)

# Commands to make a shared library
SHLIB.c=      $(CC) $(CFLAGS) $(LDFLAGS) -shared $(LD_SOOPTIONS)

# Environment variable to set a runtime search path
LDLIBRARYPATH_ENVVAR = LD_LIBRARY_PATH

# Force removal [for make clean]
RMV = rm -rf

# Location of the libraries before "make install" is used
LIBDIR=$(top_builddir)/lib

# Location of the executables before "make install" is used
BINDIR=$(top_builddir)/bin

# Link commands to link to ICU libs
LLIBDIR		= -L$(LIBDIR)
LSTUBDIR	= -L$(top_builddir)/stubdata
LCTESTFW	= -L$(top_builddir)/tools/ctestfw

LIBICUDT	= $(LLIBDIR) $(LSTUBDIR) $(ICULIBS_DT)
LIBICUUC	= $(LLIBDIR) $(ICULIBS_UC) $(LSTUBDIR) $(ICULIBS_DT)
LIBICUI18N	= $(LLIBDIR) $(ICULIBS_I18N)
LIBICULE	= $(ICULEHB_CFLAGS) $(LLIBDIR) $(ICULIBS_LE)
LIBICULX	= $(LLIBDIR) $(ICULIBS_LX)
LIBCTESTFW	= $(LCTESTFW) $(ICULIBS_CTESTFW)
LIBICUTOOLUTIL	= $(LLIBDIR) $(ICULIBS_TOOLUTIL)
LIBICUIO	= $(LLIBDIR) $(ICULIBS_IO)

# Invoke, set library path for all ICU libraries.
# overridden by icucross.mk
INVOKE = $(LDLIBRARYPATH_ENVVAR)=$(LIBRARY_PATH_PREFIX)$(LIBDIR):$(top_builddir)/stubdata:$(top_builddir)/tools/ctestfw:$$$(LDLIBRARYPATH_ENVVAR) $(LEAK_CHECKER)
# prefer stubdata
PKGDATA_INVOKE = $(LDLIBRARYPATH_ENVVAR)=$(top_builddir)/stubdata:$(top_builddir)/tools/ctestfw:$(LIBRARY_PATH_PREFIX)$(LIBDIR):$$$(LDLIBRARYPATH_ENVVAR) $(LEAK_CHECKER)
INSTALLED_INVOKE = $(LDLIBRARYPATH_ENVVAR)=$(libdir):$$$(LDLIBRARYPATH_ENVVAR)

# Current full path directory.
CURR_FULL_DIR?=$(shell pwd | sed 's/ /\\ /g')
# Current full path directory for use in source code in a -D compiler option.
CURR_SRCCODE_FULL_DIR?=$(shell pwd | sed 's/ /\\ /')

# for tests
ifneq ($(TEST_STATUS_FILE),)
TEST_OUTPUT_OPTS="-E$(TEST_STATUS_FILE)"
endif
