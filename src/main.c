/*
transcoder

A C program detect the character set of text-based columns in PostgreSQL
and transcode the same to UTF8.

 - ICU (http://site.icu-project.org/)

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

// PostgreSQL and C includes
#include "libpq-fe.h"

// transcoder includes
#include "transcoder.h"
#include "transcoder-utils.h"
#include "convert.h"
#include "log.h"
#include "vector.h"
#include "colresult.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <sys/time.h>

/*
 * program structure:

 1. main
 2. getopts
 3. open read and write connections
 4. get shortest unique index column(s) for table
 5. get character-based columns for table
 6. get row with lowest uniquely index value
 7. detect
 8. convert
 9. write converted data back to same row
10. log
11. loop to 6 until no more rows
12. cleanup
13. exit

*/

PGconn* readCxn;
PGconn* writeCxn;

int main (int argc, char** argv)
{
    PGresult *writeResult = NULL;

    // exit code for transcoder
    int exitCode = EXIT_SUCCESS;

    char fullTableName[130] = {0};

    // variables of holding
    char *uniqueKeyColsCast = NULL;
    char *uniqueKeyCols = NULL;
    char *uniqueKeyDataTypes = NULL;
    char *uniqueKeyValues = NULL;
    char *nextKeyValues = NULL;
    char *prevUniqueKeyValues = NULL;

    // character-based column names and values
    Vector cbColNames;
    Vector cbColValues;
    Vector newCBColValues;

    // pointer to string to convert
    const char* buffer = NULL;
    const char* converted_buffer = NULL;

    // encoding, language & confidence level
    char *encoding = NULL;
    char *lang = NULL;
    int32_t confidence = 0;

    // conversion timestamp
    char conversion_ts[28] = {0};

    // runtime stats
    struct timeval start_tv, end_tv, diff_tv;
    double runtime = 0;

    // rows processed
    unsigned long totalRows = 0;
    unsigned long rowsUpdated = 0;

    // boolean flags
    bool converted = false;
    bool dropped_bytes = false;

    // read and write char-based columns queries
    const char *readQuery  = NULL;
    const char *writeQuery = NULL;

    // conversion log
    char * conversionLogHeader = NULL;

    // for loop index
    int i = 0;

    // pre and post conversion sql
    char* sqlPre = NULL;
    char* sqlPost  = NULL;

    // set start time
    gettimeofday(&start_tv, NULL);

    // get command line options
    process_long_options(argc, (const char**) argv);

    // construct full schema-prefixed table name
    snprintf(fullTableName, sizeof(fullTableName), "%s.%s",
             field.schema, field.table);

    // open read db connections
    readCxn  = openDbConnection(field.dsn);
    writeCxn = openDbConnection(field.dsn);

    getShortestUniqueIndex(field.schema, field.table,
                            &uniqueKeyColsCast,
                            &uniqueKeyCols,
                            &uniqueKeyDataTypes);

    // get character-based columns for table
    getCBColNames(&cbColNames, field.schema, field.table);

    // construct read query
    readQuery = constructReadQuery(&cbColNames);

    if (field.oneRowKey)
    {
        uniqueKeyValues = field.oneRowKey;
    }
    else if (field.restartKey)
    {
        uniqueKeyValues = field.restartKey;
    }
    else
    {
        // get lowest unique key value
        uniqueKeyValues = getInitUniqueKeyValues(field.schema, field.table,
                            uniqueKeyCols);
    }

    LOGSTDERR(INFO, PQresStatus(PGRES_COMMAND_OK),
              "Starting conversion of %s\n", fullTableName);

    // print conversion log csv header
    printConversionLogHeader();

    // convert until no more rows
    while (uniqueKeyValues != NULL)
    {
        totalRows++;

        if (field.limit > 0 && field.limit < totalRows)
        {
            totalRows--;
            break;
        }

        LOGSTDERR(INFO, PQresStatus(PGRES_COMMAND_OK),
                "Converting %s: %s", uniqueKeyCols, uniqueKeyValues);

        // initialize  values vectors to appropriate size
        vector_init(&cbColValues, "PGColResult*", cbColNames.size);
        vector_init(&newCBColValues, "PGColResult*", cbColNames.size);

        // get row to convert
        getCBColValues(&cbColValues, readQuery,
                      fullTableName, uniqueKeyCols, uniqueKeyValues);

        // detect charset and transcode it
        for(i = 0; i < cbColValues.size; i++)
        {
            // pointers for column result structs
            PGColResult* colResult = NULL;
            PGColResult* newColResult = NULL;

            // get the current value for the first column
            colResult = vector_get(&cbColValues, i);

            // copy it to the new column result struct and update value and length after conversion
            // copyColResult allocates memory for newColResult;
            newColResult = copyColResult(colResult, newColResult);

            // transcode and get converted value
            converted_buffer = transcode(colResult, field.hint,
                                &encoding, &lang, &confidence,
                                conversion_ts, sizeof(conversion_ts),
                                &converted, &dropped_bytes);

            // save converted value and length in vector
            colResultSetValue(newColResult, converted_buffer);

            // newColResult gets freed when newCbColValues gets freed
            vector_set(&newCBColValues, i, (void *) newColResult);

            // clear and populate vector value
            // for each column
            ConversionLog* cl = newConversionLog();

            cl = populateConversionLog(
                    cl,
                    colResult,
                    newColResult,
                    uniqueKeyCols,
                    uniqueKeyValues,
                    encoding,
                    lang,
                    confidence,
                    conversion_ts,
                    converted,
                    dropped_bytes);

            // developer log it!
            printConversionLog(cl);

            // don't need the conversion log anymore
            freeConversionLog(cl);
            free((void *) cl);

            // encoding, lang and escaped string are
            // freed by freeConversionLog
            // reset indicators
            confidence = 0;
            converted = false;
            dropped_bytes = false;

            // this isn't a memory leak, since they're freed when
            // cbColValues and newCBColValues are freed below
            buffer = NULL;
            free((void*) converted_buffer);
            colResult = NULL;
        }

        // if passed --report option do not save to db
        if(!field.report)
        {
            // compare pre and post conversion update statements
            sqlPre = constructWriteQuery(fullTableName,
                           &cbColValues,
                           uniqueKeyCols,
                           uniqueKeyValues);


            sqlPost = constructWriteQuery(fullTableName,
                           &newCBColValues,
                           uniqueKeyCols,
                           uniqueKeyValues);

            if (strcmp(sqlPre, sqlPost))
            {
                rowsUpdated++;

                // write converted data back to same row
                writeResult = pq_query(writeCxn, sqlPost);

                if (PQresultStatus(writeResult) == PGRES_COMMAND_OK)
                {
                   // log success on stdout
                   if (field.debug)
                       LOGSTDOUT(DEBUG, PQresStatus(PQresultStatus(writeResult)),
                           "%s.%s, %s=%s updated.\n",
                           field.schema, field.table, uniqueKeyCols, uniqueKeyValues);

                   exitCode = EXIT_SUCCESS;
                }
                else
                {
                   // log failure on stderr
                   LOGSTDOUT(ERROR, PQresStatus(PQresultStatus(writeResult)),
                       "%s.%s, %s=%s update failed.\n",
                       field.schema, field.table, uniqueKeyCols, uniqueKeyValues);

                   exitCode = EXIT_FAILURE;
                }

                PQclear(writeResult);
            }
            else
            {
                LOGSTDERR(INFO, "NO_CONVERSION",
                    "No columns require conversion - skipping update of %s=%s.",
                     uniqueKeyCols, uniqueKeyValues);

                 exitCode = EXIT_SUCCESS;
            }

            free ((void *) sqlPre);
            free ((void *) sqlPost);
        }

        // free the column data and converted data buffers
        vector_free(&cbColValues);
        vector_free(&newCBColValues);

        if (field.oneRowKey)
        {
            break;
        }
        else
        {
            nextKeyValues = getNextUniqueKeyValues(field.schema, field.table,
                    uniqueKeyCols, uniqueKeyValues);
            free(uniqueKeyValues);
            uniqueKeyValues = nextKeyValues;
        }
    } // while (uniqueKeyValues != NULL)

    // cleanup after ourselves
    vector_free(&cbColNames);
    free((void *) readQuery);
    free((void *) writeQuery);
    free((void *) conversionLogHeader);
    free((void *) uniqueKeyColsCast);
    free((void *) uniqueKeyCols);
    free((void *) uniqueKeyDataTypes);
    free((void *) uniqueKeyValues);
    free((void *) prevUniqueKeyValues);
    free((void *) conversionLogHeader);

    LOGSTDERR(INFO, PQresStatus(PGRES_COMMAND_OK),
              "Completed conversion of %s", fullTableName);

    // run time, total rows, and avg rows per second
    gettimeofday(&end_tv, NULL);
    timersub(&end_tv, &start_tv, &diff_tv);
    runtime = diff_tv.tv_sec + diff_tv.tv_usec/1000000.0;
    setlocale(LC_NUMERIC, "");
    fprintf(stderr, "\n");
    fprintf(stderr, "===============================\n");
    fprintf(stderr, " Run time (secs):  %'.6f\n", runtime);
    fprintf(stderr, " Total rows:       %'ld\n", totalRows);
    fprintf(stderr, " Rows updated:     %'ld\n", rowsUpdated);
    fprintf(stderr, " %% updated:        %'.02f\n", (100.0 * rowsUpdated/totalRows));
    if (runtime)
        fprintf(stderr, " Avg rows/sec:     %.2f\n", totalRows/runtime);
    else
        fprintf(stderr, " *All* the rows in %.6f seconds!\n", runtime);
    fprintf(stderr, "===============================\n");
    fprintf(stderr, "\n");
    // exit
    clean_exit(exitCode);

    // this never gets reached since clean_exit does an exit(x)
    // but it should keep -Wall from squawking
    return exitCode;

} // end main()
