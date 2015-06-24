/*
 * transcoder.c
 *
 * Copyright © 2014-2015, AWeber Communications.
 *
 */

// pick up vasprintf
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "transcoder.h"
#include "transcoder-utils.h"
#include "log.h"
#include "vector.h"

// PG connections
extern PGconn *readCxn;
extern PGconn *writeCxn;

PGconn* openDbConnection(const char* dsn)
{
    // set application name in db
    char* extDsn = NULL;
    char appName[] = " application_name=transcoder";
    int extDsnLen = 0;

    extDsnLen = strlen(dsn) + strlen(appName) + 1;
    extDsn = malloc(extDsnLen);
    memset(extDsn, 0, extDsnLen);

    strcat(extDsn, dsn);
    strcat(extDsn, appName);

    // open db connection
    PGconn* cxn = PQconnectdb(extDsn);

    if (PQstatus(cxn) == CONNECTION_BAD)
    {
        puts("Cannot establish database connection");
        clean_exit(EXIT_FAILURE);
    }

    free((void *) extDsn);

    // closed and freed by clean_exit()
    return cxn;
}

void clean_exit(int exitStatus)
{
    // clean up
    if (readCxn)
        PQfinish(readCxn);

    if (writeCxn)
        PQfinish(writeCxn);

    exit(exitStatus);
}

const char* getShortestUniqueIndex(const char* schema, const char* table,
                                    char** shortestUniqueIndexCast,
                                    char** shortestUniqueIndexCols,
                                    char** shortestUniqueIndexDataTypes)
{
    // query results
    PGresult *readResult = NULL;

    // rows and columns
    int row = 0;
    int col = 0;

    // records processed
    int readRecCount = 0;
    int readColCount = 0;

    // get unique key columns zipped with their data types
    // query yields one row per column/datatype
    const char suiSql[] =
    "select array_to_string(public.zip(out_unique_key_col, out_unique_key_data_type), '::') as col_cast"
    "   from public.get_shortest_unique_key('%s', '%s');";

    readResult = pq_vaquery(readCxn, suiSql, schema, table);

    if (PQresultStatus(readResult) != PGRES_TUPLES_OK)
    {
        LOGSTDERR(ERROR,PQresStatus(PQresultStatus(readResult)),
            "Shortest unique index query failed: %s", suiSql);
        clean_exit(EXIT_FAILURE);
    }

    readRecCount = PQntuples(readResult);
    readColCount = PQnfields(readResult);

    if (readRecCount < 1 || readColCount > 1)
    {
        LOGSTDERR(ERROR,PQresStatus(PQresultStatus(readResult)),
            "Shortest unique index query returned too few rows (%d) or too many columns (%d)",
                    readRecCount, readColCount);
        clean_exit(EXIT_FAILURE);
    }

    char* tmp = NULL;
    char* stop = NULL;

    // freed when *shortestUniqueIndexCast is freed
    char* suic = calloc(sizeof(char), 1);

    for (row = 0; row < readRecCount; row++)
    {
        tmp = strdup(PQgetvalue(readResult, row, col));
        suic = concat(suic, tmp, stop);

        if (readRecCount > 0 && row < readRecCount - 1)
            suic = concat(suic, ", ", stop);

        if (suic == NULL)
        {
            LOGSTDERR(ERROR, "STRING_CONCAT_FAILED",
                "Shortest unique index string is NULL. Check coredump file. Aborting...", NULL);
            raise(SIGABRT);
        }
    }

    free((void *) tmp);

    *shortestUniqueIndexCast = suic;

    PQclear(readResult);

    // get unique key columns and data types in separate arrays
    const char suiSql2[] =
    "select array_to_string(out_unique_key_col, ', '),"
    "       array_to_string(out_unique_key_data_type, ', ')"
    "   from public.get_shortest_unique_key('%s', '%s');";

    readResult = pq_vaquery(readCxn, suiSql2, schema, table);

    if (PQresultStatus(readResult) != PGRES_TUPLES_OK)
    {
        LOGSTDERR(ERROR,PQresStatus(PQresultStatus(readResult)),
            "Shortest unique index query failed: %s", suiSql2);
        clean_exit(EXIT_FAILURE);
    }

    readRecCount = PQntuples(readResult);
    readColCount = PQnfields(readResult);

    if (readRecCount < 1 || readColCount > 2)
    {
        LOGSTDERR(ERROR,PQresStatus(PQresultStatus(readResult)),
            "Shortest unique index query returned too few rows (%d) or too many columns (%d)",
            readRecCount, readColCount);
        clean_exit(EXIT_FAILURE);
    }

    *shortestUniqueIndexCols      = strdup(PQgetvalue(readResult, 0, 0));
    *shortestUniqueIndexDataTypes = strdup(PQgetvalue(readResult, 0, 1));

    PQclear(readResult);

    return *shortestUniqueIndexCast;
}

void getCBColNames(Vector* cn, const char* schema, const char* table)
{
    // query results
    PGresult *readResult;

    // rows and columns
    int row = 0;
    int col = 0;

    // records processed
    int readRecCount = 0;
    int readColCount = 0;

    // get character-based columns for table
    const char cbColsSql[] =
    "SELECT pg_attribute.attname AS column_name"
    "  FROM pg_class"
    "  JOIN pg_namespace ON (pg_class.relnamespace = pg_namespace.oid)"
    "  JOIN pg_attribute ON (pg_attribute.attrelid = pg_class.oid)"
    "  JOIN pg_type ON (pg_attribute.atttypid = pg_type.oid)"
    " WHERE pg_class.relkind = 'r'"
    "   AND pg_type.typtype = 'b'"
    "   AND pg_type.typcategory = 'S'"
    "   AND NOT pg_attribute.attisdropped"
    "   AND pg_attribute.attnum > 0"
    "   AND pg_namespace.nspname = quote_ident('%s')"
    "   AND pg_class.relname = quote_ident('%s')"
    " ORDER BY pg_attribute.attnum;";

    readResult = pq_vaquery(readCxn, cbColsSql, schema, table);

    if (PQresultStatus(readResult) != PGRES_TUPLES_OK)
    {
        LOGSTDERR(ERROR,PQresStatus(PQresultStatus(readResult)),
        "Character-based columns query failed: %s", cbColsSql);
        clean_exit(EXIT_FAILURE);
    }

    readRecCount = PQntuples(readResult);
    readColCount = PQnfields(readResult);

    if (readColCount > 1)
    {
        LOGSTDERR(ERROR,PQresStatus(PQresultStatus(readResult)),
        "Character-based columns query returned too many columns (%d)", readColCount);
        clean_exit(EXIT_FAILURE);
    }

    // size vector
    vector_init(cn, "char*", readRecCount);

    // first column is the name
    col = 0;

    // PQntuples counts from 0
    for (row = 0; row < readRecCount; row++)
    {
        vector_append(cn,
            (void *) strdup(PQgetvalue(readResult, row, col)));
    }

    PQclear(readResult);
}

void getCBColValues(Vector *cv,
                    const char* readQuery,
                    const char* fullTableName,
                    const char* uniqueKeyCols,
                    const char* uniqueKeyValues)
{
    // query results
    PGresult    *readResult = NULL;
    char* cmdStatus  = NULL;  // freed when related PGresult is passed to PQclear
    char* cmdTuples  = NULL;  // freed when related PGresult is passed to PQclear

    // rows and columns
    int row = 0;
    int col = 0;

    // records processed
    int readRecCount = 0;
    int readColCount = 0;

    readResult = pq_vaquery(readCxn, readQuery,
                            fullTableName,
                            uniqueKeyCols,
                            uniqueKeyValues);

    cmdStatus = PQcmdStatus(readResult);
    cmdTuples = PQcmdTuples(readResult);

    if (PQresultStatus(readResult) != PGRES_TUPLES_OK)
    {
        LOGSTDERR(ERROR, PQresStatus(PQresultStatus(readResult)),
        "Get column value(s) query failed: %s", readQuery);
        clean_exit(EXIT_FAILURE);
    }
    else if (field.debug)
    {
        int numtups = atoi(cmdTuples);

        LOGSTDERR(DEBUG, PQresStatus(PQresultStatus(readResult)),
        "Get column values:\n%s\n%s\n(%d %s)\n",
        readQuery, cmdStatus, numtups, (numtups == 1 ? "row" : "rows"));
    }

    readRecCount = PQntuples(readResult);
    readColCount = PQnfields(readResult);

    if (readRecCount > 1)
    {
        LOGSTDERR(ERROR, PQresStatus(PQresultStatus(readResult)),
        "Character-based columns query returned too many rows (%d)", readRecCount);
        clean_exit(EXIT_FAILURE);
    }

    // PQntuples counts from 0
    for (row = 0; row < readRecCount; row++)
    {
        for(col = 0; col < readColCount; col++)
        {
            // pointer to hold result struct
            PGColResult* colResult = malloc(sizeof(PGColResult));

            // populate struct
            colResult->fname     = strdup(PQfname(readResult, col));
            colResult->fnumber   = PQfnumber(readResult, colResult->fname);
            colResult->ftable    = PQftable(readResult, col);
            colResult->ftablecol = PQftablecol(readResult, col);
            colResult->fformat   = PQfformat(readResult, col);
            colResult->ftype     = PQftype(readResult, col);
            colResult->fmod      = PQfmod(readResult, col);
            colResult->fsize     = PQfsize(readResult, col);
            colResult->isnull    = (PQgetisnull(readResult, row, col) ? true: false);
            colResult->length    = PQgetlength(readResult, row, col);
            colResult->value     = strdup(PQgetvalue(readResult, row, col));

            vector_append(cv, (void *) colResult);
        }
    }

    PQclear(readResult);
}

char* constructWriteQuery(const char* fullTableName,
                     const Vector* colValues,
                     const char* uniqueKeyCols,
                     const char* uniqueKeyValues)
{
    /* construct write query from character-based column names

        "update %s"
        "   set <colx> = '%s', ..."
        " where (%s) = (%s);"
    */

    int sqlLen = 0, i = 0;
    const char update_format[] = "update %s set ";
    const char column_format[] = " %s = %s ";
    const char where_format[]  = " where (%s) = (%s);";

    char* update = NULL;
    char* column = NULL;
    char* ptr = NULL;
    char* where  = NULL;
    char* sql = NULL;
    char comma = ',';
    char* quotedVal = NULL;

    // start of update statement
    if (!(asprintf(&update, update_format, fullTableName)))
    {
        perror("asprintf - update");
        clean_exit(EXIT_FAILURE);
    }

    // where clause
    if (!(asprintf(&where, where_format, uniqueKeyCols, uniqueKeyValues)))
    {
        perror("asprintf - where");
        clean_exit(EXIT_FAILURE);
    }

    sqlLen = strlen(update) + strlen(where);

    // calc sql string length
    for (i = 0; i < colValues->size; i++)
    {
        PGColResult* colResult = (PGColResult*) colValues->data[i];

        char* fname  = colResult->fname;
        bool  isnull = colResult->isnull;
        char* value  = colResult->value;
        int   length = colResult->length;

        // value is not NULL or ""
        if (isnull == false && length > 0 )
        {
            quotedVal = pq_escape(writeCxn, value, length);
        }
        // value is NULL, not ""
        else if (isnull == true)
        {
            quotedVal = strdup("NULL");
        }
        // value is "", not NULL
        else if (isnull == false && length == 0)
        {
            quotedVal = strdup("''");
        }

        sqlLen += strlen(fname) +
                  strlen(quotedVal) +
                  5;  // 4 spaces  & equals sign

        free((void *) quotedVal);
    }

    sql = malloc(sizeof(char) * sqlLen + 1);
    memset(sql, 0, sqlLen + 1);

    strcat(sql, update);
    if(strlen(sql) < strlen(update))
    {
        perror("strcat - update");
        clean_exit(EXIT_FAILURE);
    }

    // build "set = ?" clauses
    for(i = 0; i < colValues->size; i++)
    {
        PGColResult* colResult = (PGColResult*) colValues->data[i];

        char* fname  = colResult->fname;
        int   fsize  = colResult->fsize;
        int   fmod   = colResult->fmod;
        bool  isnull = colResult->isnull;
        char* value  = colResult->value;
        int   length = colResult->length;

        int   allowedLength = 0;

        // alert if data is too long for column
        if ((isnull == false && length > 0)  // not empty string
             && (fsize == -1 && fmod != -1)) // not a text field
        {
            allowedLength = fmod - 4;        // subtract 4 bytes for length in varlena

            if (length > allowedLength)      // over variable length field size
            {
                char* tmp = calloc(sizeof(char), length + 1);
                tmp = strncpy(tmp, value, allowedLength);

                LOGSTDERR(WARNING, "STRING_DATA_RIGHT_TRUNCATION",
                    "Value too long for data type.  Data will be truncated and may be invalid UTF8.\n"
                    "Column: %s, Length: %d\n"
                    "Converted Value Length: %d\n"
                    "Original Value: %s\n"
                    "Converted & Truncated Value: %s\n",
                    fname, allowedLength,
                    strlen(tmp),
                    value,
                    tmp);

                free((void *) tmp);

                // quotedVal will be truncated
                quotedVal = pq_escape(writeCxn, value, allowedLength);
            }
            else
            {
                // quotedVal is not  truncated
                quotedVal = pq_escape(writeCxn, value, length);
            }
        }
        // not an empty string and *is* a text field
        else if ((isnull == false && length > 0)
             && (fsize == -1 && fmod == -1))
        {
            quotedVal = pq_escape(writeCxn, value, length);
        }
        // value is NULL
        else if (isnull == true)
        {
            quotedVal = strdup("NULL");
        }
        // value is empty string
        else if (isnull == false && length == 0)
        {
            quotedVal = strdup("''");
        }

        if (!(asprintf(&column, column_format,
                       fname, quotedVal)))
        {
            perror("asprintf - column");
            clean_exit(EXIT_FAILURE);
        }

        free((void *) quotedVal);

        // tack on a comma if more than one column
        // but not the last column
        if (colValues->size > 1 && i < colValues->size - 1)
        {
            ptr = column + strlen(column) - 1;
            *ptr = comma;
        }

        strcat(sql, column);

        free((void *) column);
    }

    strcat(sql, where);

    if (field.debug)
    {
        fprintf(stderr, "%s\n", sql);
    }

    free((void*) update);
    free((void*) where);

    // free in caller
    return sql;
}

const char* constructReadQuery(const Vector* cbColNames)
{
/* construct read query from character-based column names

    "select <colnames>"
    "  from %s"
    " where (%s) = (%s);";
*/

    int len = 0, i = 0;
    char* sql = NULL;

    for(i = 0; i < cbColNames->size; i++)
        len += strlen((const char*) cbColNames->data[i]);

    len +=  strlen("select ") +
            strlen("  from %s") +
            strlen(" where (%s) = (%s);") +
            ((cbColNames->size - 1) * 2) +   // ", " comma space between cols
            1;

    sql = malloc(sizeof(char) * len);
    memset(sql, 0, len);

    strcat(sql, "select ");

    for(i = 0; i < cbColNames->size; i++)
    {
        if (i > 0) strcat(sql, ", ");
        strcat(sql, (const char*) cbColNames->data[i]);
    }

    strcat(sql, "  from %s");
    strcat(sql, " where (%s) = (%s);");

    if (field.debug)
    {
        LOGSTDERR(DEBUG, "Read SQL Query",
                "%s", sql);
    }

    // free in caller
    return sql;
}

char* getInitUniqueKeyValues(const char* schema, const char* table,
                             const char* uniqueKeyCols)
{
    // query results
    PGresult    *readResult = NULL;

    // rows and columns
    int row = 0;
    int col = 0;

    // records processed
    int readRecCount = 0;
    int readColCount = 0;

    char* uniqueKeyValues = NULL;

    // get starting row with lowest unique index value
    const char initUkValsSql[] =
        "select out_cast_min_uk_values"
        "  from public.get_min_shortest_unique_key_values('%s', '%s');";

    readResult = pq_vaquery(readCxn, initUkValsSql, schema, table);

    if (PQresultStatus(readResult) != PGRES_TUPLES_OK)
    {
        LOGSTDERR(ERROR, PQresStatus(PQresultStatus(readResult)),
            "Initial unique index value query failed: %s", initUkValsSql);
        clean_exit(EXIT_FAILURE);
    }

    if (PQgetisnull(readResult, row, col))
    {
        LOGSTDERR(ERROR, PQresStatus(PQresultStatus(readResult)),
            "Initial unique index value is %s. Cannot proceed.", "NULL");
        clean_exit(EXIT_FAILURE);
    }

    readRecCount = PQntuples(readResult);
    readColCount = PQnfields(readResult);

    if (readRecCount > 1 || readColCount > 1)
    {
        LOGSTDERR(ERROR, PQresStatus(PQresultStatus(readResult)),
            "Initial unique index value query returned too many rows (%d) or columns (%d)",
                readRecCount, readColCount);
        clean_exit(EXIT_FAILURE);
    }

    uniqueKeyValues = strdup(PQgetvalue(readResult, 0, 0));

    PQclear(readResult);

    // free in caller
    return uniqueKeyValues;
}

char* getNextUniqueKeyValues(const char* schema, const char* table,
                             const char* uniqueKeyCols, const char* prevUkValues)
{
    // query results
    PGresult* readResult = NULL;

    // rows and columns
    int row = 0;
    int col = 0;

    // records processed
    int readRecCount = 0;
    int readColCount = 0;

    char* uniqueKeyValues = NULL;

    char *escapedUkValues = pq_escape(readCxn, prevUkValues, strlen(prevUkValues));

    // get next lowest unique index value
    // unique key values is escaped, so don't need E'...' syntax
    const char nextUkValsSql[] =
        "select out_cast_next_uk_values"
        "  from public.get_next_shortest_unique_key_values('%s', '%s', %s);";

    readResult = pq_vaquery(readCxn, nextUkValsSql,
                        schema, table,
                        escapedUkValues);

    PQfreemem((void *) escapedUkValues);

    if (PQresultStatus(readResult) != PGRES_TUPLES_OK)
    {
        LOGSTDERR(ERROR, PQerrorMessage(readCxn),
            "Next lowest unique index value query failed: %s", nextUkValsSql);
        clean_exit(EXIT_FAILURE);
    }

    readRecCount = PQntuples(readResult);
    readColCount = PQnfields(readResult);

    if (readRecCount > 1 || readColCount > 1)
    {
        LOGSTDERR(ERROR, PQresStatus(PQresultStatus(readResult)),
            "Next lowest unique index value query returned too many rows (%d) or columns (%d)",
                readRecCount, readColCount);
        clean_exit(EXIT_FAILURE);
    }

    if (PQgetisnull(readResult, row, col))
    {
        // return NULL if no more unique key values
        PQclear(readResult);
        return NULL;
    }

    uniqueKeyValues = strdup(PQgetvalue(readResult, row, col));

    PQclear(readResult);

    return uniqueKeyValues;
}

const char* transcode(PGColResult* colResult, const char* hint,
            char** encoding, char** lang, int32_t* confidence,
            char* conversion_ts, size_t conversion_ts_size,
            bool* converted, bool* dropped_bytes)
{
    // value is null or empty string nothing to do
    if ((colResult->isnull == true) ||
        (colResult->isnull == false && colResult->length == 0))
    {
        *encoding = "UTF-8";
        *lang = "";
        *confidence = 100;
        *converted = false;
        *dropped_bytes = false;

        return strdup(colResult->value);
    }

    // ICU status error code
    UErrorCode uStatus = U_ZERO_ERROR;

    // ICU variables of holding
    UChar* uBuf = NULL;
    int32_t uBuf_len = 0;

    // dropped bytes going to UTF16?
    bool dropped_bytes_toU = false;

    // dropped bytes going to UTF8?
    bool dropped_bytes_fromU = false;

    // buffer to convert
    const char* buffer = colResult->value;

    // temporary buffer for converted string
    char* converted_buf = NULL;

    // set conversion timestamp
    conversion_ts = currentTimestamp(conversion_ts, conversion_ts_size);

    // detect encoding with ICU
    uStatus = detect_ICU(buffer, hint, encoding, lang, confidence);

    if (U_FAILURE(uStatus) || *encoding == NULL)
    {
        LOGSTDERR(ERROR, u_errorName(uStatus),
            "Charset detection of failed - aborting transcoding.\n", NULL);

        *encoding = NULL;
        *lang = NULL;
        *confidence = 0;
        *converted = false;
        *dropped_bytes = false;

        return strdup(buffer);
    }

    if (field.debug)
    {
        LOGSTDERR(DEBUG, u_errorName(uStatus),
            "ICU detection status: %d\n", uStatus);

        LOGSTDERR(DEBUG, u_errorName(uStatus),
            "Detected encoding: %s, language: %s, confidence: %d\n",
                *encoding, *lang, *confidence);
    }

    // return without attempting a conversion if UTF8 is detected
    if (
        (0 == strcmp("UTF-8", *encoding)) ||
        (0 == strcmp("utf-8", *encoding)) ||
        (0 == strcmp("UTF8",  *encoding)) ||
        (0 == strcmp("utf8",  *encoding))
       )
    {
        if (field.debug)
        {
            if (buffer == NULL)
                LOGSTDERR(DEBUG, u_errorName(uStatus),
                    "ICU detected %s.  No conversion necessary.\n", *encoding);
        }

        *converted = false;
        *dropped_bytes = false;

        return strdup(buffer);
    }
    else
    {
        // UTF8 output can be up to 6 bytes per input byte
        int32_t converted_buf_len = strlen(buffer) * 6 * sizeof(char);
        converted_buf = (char *) malloc(converted_buf_len + 1);
        memset(converted_buf, 0, converted_buf_len + 1);

        // ICU uses UTF16 internally, so need to convert to UTF16 first
        // then convert to UTF8

        if (U_SUCCESS(uStatus))
            uStatus = convert_to_unicode(buffer, (const char*) *encoding,
                &uBuf, (int32_t*) &uBuf_len,
                field.force, &dropped_bytes_toU, field.debug);

        if (field.debug)
            LOGSTDERR(DEBUG, u_errorName(uStatus),
                "ICU conversion to Unicode status: %d\n", uStatus);

        // so far so good. convert from UTF16 to UTF8
        if (U_SUCCESS(uStatus))
            uStatus = convert_to_utf8((const UChar*) uBuf, uBuf_len,
                &converted_buf, (int32_t*) &converted_buf_len,
                field.force, &dropped_bytes_fromU, field.debug);

        free((void *) uBuf);

        if (field.debug)
            LOGSTDERR(DEBUG, u_errorName(uStatus),
                "ICU conversion to UTF8 status: %d\n", uStatus);

        // ICU thinks it worked!
        if (U_SUCCESS(uStatus))
        {
            if (field.debug)
                LOGSTDERR(DEBUG, u_errorName(uStatus),
                    "ICU conversion complete - status: %d\n", uStatus);

            *converted = true;
            *dropped_bytes = (dropped_bytes_toU || dropped_bytes_fromU);

            // return converted buffer
            return (const char*) converted_buf;
        }
        else
        {
            LOGSTDERR(ERROR, u_errorName(uStatus),
                "ICU conversion failed; returning original input - status: %d\n", uStatus);

            *converted = false;
            *dropped_bytes = false;

            // return original buffer
            return strdup(buffer);
        }
    }
}

int printConversionLogHeader()
{
    const char format[] = "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n";

    return fprintf(stdout, format,
            "schemaname",
            "tablename",
            "columnname",
            "unique_key_columns",
            "uk_value",
            "detected_encoding",
            "detected_language",
            "confidence_level",
            "original_bytestream",
            "converted_bytestream",
            "conversion_ts",
            "converted",
            "dropped_bytes");
}

int printConversionLog(const ConversionLog* cl)
{
    const char format[] = "%s,%s,%s,%s,%s,%s,%s,%d,%s,%s,%s,%s,%s\n";

    return fprintf(stdout, format,
            cl->schemaname,
            cl->tablename,
            cl->columnname,
            cl->unique_key_columns,
            cl->uk_value,
            cl->detected_encoding,
            cl->detected_language,
            cl->confidence_level,
            cl->original_bytestream,
            cl->converted_bytestream,
            cl->conversion_ts,
            (cl->converted ? "true" : "false"),
            (cl->dropped_bytes ? "true" : "false"));
}

ConversionLog* newConversionLog()
{
    ConversionLog* cl = malloc(sizeof(ConversionLog));
    memset(cl, 0, sizeof(ConversionLog));

    cl->schemaname           = "";
    cl->tablename            = "";
    cl->columnname           = "";
    cl->unique_key_columns   = "";
    cl->uk_value             = "";
    cl->detected_encoding    = "";
    cl->detected_language    = "";
    cl->confidence_level     = 0;
    cl->original_bytestream  = "";
    cl->converted_bytestream = "";
    cl->conversion_ts        = "";
    cl->converted            = false;
    cl->dropped_bytes        = false;

    return cl;
}

ConversionLog* populateConversionLog(ConversionLog* cl,
            const PGColResult* originalColResult,
            const PGColResult* convertedColResult,
            const char* uniqueKeyCols,
            const char* uniqueKeyValues,
            const char* encoding,
            const char* language,
            const int   confidence_level,
            const char* conversion_ts,
            const bool  converted,
            const bool  dropped_bytes)
{
    cl->schemaname           = strdup(field.schema);
    cl->tablename            = strdup(field.table);
    cl->columnname           = strdup(originalColResult->fname);
    cl->unique_key_columns   = strdup(uniqueKeyCols);
    cl->uk_value             = strdup(uniqueKeyValues);

    if (encoding)
    {
        cl->detected_encoding    = strdup(encoding);
    }
    else
    {
        cl->detected_encoding = NULL;
    }

    if (language)
    {
        cl->detected_language    = strdup(language);
    }
    else
    {
        cl->detected_language = NULL;
    }

    cl->confidence_level     = confidence_level;
    cl->original_bytestream  = convertToHex(originalColResult->value,  originalColResult->isnull,  originalColResult->length);
    cl->converted_bytestream = convertToHex(convertedColResult->value, convertedColResult->isnull, convertedColResult->length);
    cl->conversion_ts        = strdup(conversion_ts);
    cl->converted            = converted;
    cl->dropped_bytes        = dropped_bytes;

    return cl;
}

void freeConversionLog(ConversionLog * cl)
{
    if (cl->schemaname)           free((void *) cl->schemaname);
    if (cl->tablename)            free((void *) cl->tablename);
    if (cl->columnname)           free((void *) cl->columnname);
    if (cl->unique_key_columns)   free((void *) cl->unique_key_columns);
    if (cl->uk_value)             free((void *) cl->uk_value);
    if (cl->detected_encoding)    free((void *) cl->detected_encoding);
    if (cl->detected_language)    free((void *) cl->detected_language);
    if (cl->original_bytestream)  free((void *) cl->original_bytestream);
    if (cl->converted_bytestream) free((void *) cl->converted_bytestream);
    if (cl->conversion_ts)        free((void *) cl->conversion_ts);
}

const char* convertToHex(const char *bytes, bool isnull, int length)
{
    /* >> '\\x{0}'.format(''.join(hex(ord(c))[2:] for c in bytes)) */
    static const char xdigits[] = "0123456789abcdef";
    const char *p;
    char *q, *out_buf;

    if (isnull)
    {
        return strdup("NULL");
    }

    if (!isnull && length == 0)
    {
        return strdup("empty string");
    }

    for (p=bytes; *p; ++p)
        /*empty*/;

    out_buf = malloc(((p - bytes) * 2) + 3);

    if (out_buf == NULL)
    {
        perror("malloc");
        clean_exit(EXIT_FAILURE);
    }

    strcpy(out_buf, "\\x");

    for (p=bytes, q=&out_buf[2]; *p; ++p)
    {
        *q++ = xdigits[(*p & 0xF0) >> 4];
        *q++ = xdigits[*p & 0x0F];
    }

    *q = '\0';

    return out_buf;
}
