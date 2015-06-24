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

/*
* Options:
*
*  mandatory:
*
* --dsn - data source name specification, e.g 'host=<hostname> dbname=<db name> port=<db port> user=<db login> password=<user password>'
* --schema - table's schema name
* --table - table name
*
* optional:
*
* --report - report detected character set encoding, but do not translate
*
* help:
*
* --help
*
*/

// pick up vasprintf
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "transcoder-utils.h"
#include "log.h"

struct option long_options[] =
{
    {"report",  no_argument, &field.report, 1},
    {"help",    no_argument, &field.help,   1},
    {"debug",   no_argument, &field.debug,  1},
    {"force",   no_argument, &field.force,  1},
    {"dsn",     required_argument, 0, 'd'},
    {"schema",  required_argument, 0, 's'},
    {"table",   required_argument, 0, 't'},
    {"one-row", required_argument, 0, 'o'},
    {"restart", required_argument, 0, 'r'},
    {"limit",   required_argument, 0, 'l'},
    {"hint",    required_argument, 0, 'e'},
    {0, 0, 0, 0}
};

static char usage[] = "Usage: transcoder --dsn=<dsn spec> --schema=<schema name> --table=<table name> \\ \n"
                      "                  --one-row=<unique key value> --restart=<unique key value> --limit=<integer> \\\n"
                      "                  --hint=<encoding> --force --report --debug --help\n"
                      "\n"
                      "                  --dsn: dsn spec with the form:\n"
                      "                         'host=<host> port=<port> dbname=<db> user=<dblogin> password=<dbpwd>'\n"
                      "                         if running local on the database server host do not supply the \n"
                      "                         'host' or 'port' dsn parameters to use a Unix domain socket to connect\n"
                      "                         to the database more efficiently.  Required.\n"
                      "                  --schema:  schema name for the table. Required\n"
                      "                  --table:   table name. Required.\n"
                      "                  --one-row: process a particular row, identified by the shortest unique key,\n"
                      "                             e.g. \"'20'::integer\" or \"'3'::integer, 'Hold'::text\" for\n"
                      "                             multicolumn keys.  Optional.\n"
                      "                  --restart: restart at the specified unique key.  Optional. See --one-row for syntax\n"
                      "                  --limit:   limit the number of rows processed.  Optional.\n"
                      "                  --hint:    declared encoding from alternate source, like html header or xml declaration.\n"
                      "                  --force:   force transcoding to UTF8 by dropping invalid, illegal, or unassigned bytes.  Optional.\n"
                      "                  --report:  report detected character sets but do not transcode or update data.  Optional.\n"
                      "                  --debug:   print debug messages.  Optional.\n"
                      "                  --help:    print this message\n";

void process_long_options(const int argc, const char** argv)
{
    int c;

    // initialize cstring arguments
    field.oneRowKey = NULL;
    field.restartKey = NULL;
    field.limit = 0;

    while (1)
    {
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long (argc, (char *const *) argv, "d:s:t:o:r:l:e:",
        long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 0:
                /* If this option set a flag, do nothing else now. */
                if (long_options[option_index].flag != 0)
                    break;
                printf ("option %s", long_options[option_index].name);
                if (optarg)
                        printf (" with arg %s", optarg);
                printf ("\n");
                break;

            case 'd':
                printf ("option --dsn with value '%s'\n", optarg);
                strcpy(field.dsn, optarg);
                break;

            case 's':
                printf ("option --schema with value '%s'\n", optarg);
                strcpy(field.schema, optarg);
                break;

            case 't':
                printf ("option --table with value '%s'\n", optarg);
                strcpy(field.table, optarg);
                break;

            case 'o':
                printf ("option --one-row with value '%s'\n", optarg);
                // have to allocate since can't know in advance
                // the key's size
                field.oneRowKey = malloc(strlen(optarg) + 1);
                memset(field.oneRowKey, 0, strlen(optarg) + 1);
                strcpy(field.oneRowKey, optarg);
                break;

            case 'r':
                printf ("option --restart with value '%s'\n", optarg);
                field.restartKey = malloc(strlen(optarg) + 1);
                memset(field.restartKey, 0, strlen(optarg) + 1);
                strcpy(field.restartKey, optarg);
                break;

            case 'l':
                printf ("option --limit with value '%s'\n", optarg);
                // convert input to unsigned long, base 10
                field.limit = strtoul(optarg, NULL, 10);
                break;

            case 'e':
                printf ("option --hint with value '%s'\n", optarg);
                field.hint = malloc(strlen(optarg) + 1);
                memset(field.hint, 0, strlen(optarg) + 1);
                strcpy(field.hint, optarg);
                break;

            case '?':
                fprintf(stderr, usage, argv[0]);
                exit(EXIT_FAILURE);
                break;

            default:
                raise (SIGABRT);  // dump core
        }
    }

    if (field.force)
        puts ("force flag is set");

    if (field.report)
        puts ("report flag is set");

    if (field.debug)
        puts ("debug flag is set");

    if (field.help)
    {
        puts ("help flag is set");
        fprintf(stderr, usage, argv[0]);
        exit(EXIT_SUCCESS);
    }

    if (!field.dsn[0])
    {
        fprintf(stderr, "ERROR: dsn required.\n");
        fprintf(stderr, usage, argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!field.schema[0])
    {
        fprintf(stderr, "ERROR: schema required.\n");
        fprintf(stderr, usage, argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!field.table[0])
    {
        fprintf(stderr, "ERROR: table required.\n");
        fprintf(stderr, usage, argv[0]);
        exit(EXIT_FAILURE);
    }
}

PGresult * pq_query(PGconn* cxn, const char* query)
{
    PGresult *result;

    if (!query)
        return(0);

    result = PQexec(cxn, query);

    // free result in caller
    return(result);
}

PGresult * pq_vaquery(PGconn* cxn, const char* format, ...)
{
    va_list argv;
    char *query = NULL;
    PGresult *result = NULL;
    int bytesPrinted = 0;

    va_start(argv, format);
    bytesPrinted = vasprintf(&query, format, argv);
    va_end(argv);

    if (!query)
        return(0);

    result = PQexec(cxn, query);

    if (query)
        free(query);

    // free result in caller
    return(result);
}

char * pq_escape (PGconn* cxn, const char* input, int len)
{
    char *output;

    output = PQescapeLiteral(cxn, input, len);

    if (output == NULL)
        LOGSTDERR(ERROR, PQresStatus(PGRES_FATAL_ERROR),
            "Failed to escape string: %s", input);

    // free output in caller
    return(output);
}

// concatenate strings
// last va_list arg has to be NULL
// thanks GNU!
char* concat (const char *str, ...)
{
  va_list ap;
  size_t allocated = 100;

  // pointer to concatenated string
  char *result = (char *) malloc (allocated);

  if (result != NULL)
    {
      // working pointers
      char *newp;
      char *wp;
      const char *s;

      va_start (ap, str);

      wp = result;
      for (s = str; s != NULL; s = va_arg (ap, const char *))
        {
          size_t len = strlen (s);

          /* Resize the allocated memory if necessary.  */
          if (wp + len + 1 > result + allocated)
            {
              allocated = (allocated + len) * 2;
              newp = (char *) realloc (result, allocated);
              if (newp == NULL)
                {
                  // oops! realloc didn't work!
                  free (result);
                  va_end(ap);
                  return NULL;
                }
              wp = newp + (wp - result);
              result = newp;
            }

          wp = mempcpy (wp, s, len);
        }

      /* Terminate the result string.  */
      *wp++ = '\0';

      /* Resize memory to the optimal size.  */
      newp = realloc (result, wp - result);
      if (newp != NULL)
        result = newp;

      va_end (ap);
    }

  return result;
}
