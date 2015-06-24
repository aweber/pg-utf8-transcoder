# program name and install location
bin_PROGRAMS = transcoder

# sources
transcoder_SOURCES = log.c vector.c convert.c flagcb.c colresult.c transcoder-utils.c transcoder.c main.c

# preprocessor, linker and linker flags
AM_CPPFLAGS = $(ICU_CPPFLAGS) $(PGSQL_CPPFLAGS)
AM_CFLAGS = -g -O0 -Wall
AM_LDFLAGS = -O0
LDADD = $(ICU_LDFLAGS) $(PGSQL_LDFLAGS)

