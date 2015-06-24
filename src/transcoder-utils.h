/*
transcoder utilities

Copyright © 2014-2015, AWeber Communications.

This program is licensed under the PostgreSQL license, except where
other licences apply.

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose, without fee, and without a written agreement
is hereby granted, provided that the above copyright notice and this paragraph
and the following two paragraphs appear in all copies.

IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE TO ANY
PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES,
INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
DOCUMENTATION, EVEN IF AWEBER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
DAMAGE.

AWEBER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND AWEBER HAS
NO OBLIGATIONS TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
MODIFICATIONS.
*/

#ifndef _TRANSCODER_UTILS_H_
#define _TRANSCODER_UTILS_H_

// PostgreSQL and C includes
#include <libpq-fe.h>

// pick up vasprintf
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <getopt.h>

#include "log.h"

struct GlobalArgs
{
        char dsn[128];
        char schema[65];
        char table[65];
        char *oneRowKey;
        char *restartKey;
        unsigned long limit;
        char *hint;
        int  report;
        int  debug;
        int  force;
        int  help;
} field;

void process_long_options(const int argc, const char** argv);

PGresult * pq_query(PGconn* cxn, const char* query);
PGresult * pq_vaquery(PGconn* cxn, const char* format, ...);
char * pq_escape (PGconn* cxn, const char* input, int len);
char* concat (const char *str, ...);

#endif // #ifndef _TRANSCODER_UTILS_H_
