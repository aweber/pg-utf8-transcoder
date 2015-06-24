/*
 * transcoder.h
 *
 * Copyright © 2014-2015 AWeber Communications
 * All rights reserved.
 */

#ifndef _TRANSCODER_H_
#define _TRANSCODER_H_

// pick up vasprintf
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "transcoder-utils.h"
#include "vector.h"
#include "convert.h"
#include "colresult.h"

typedef struct
{
    const char* schemaname;
    const char* tablename;
    const char* columnname;
    const char* unique_key_columns;
    const char* uk_value;
    const char* detected_encoding;
    const char* detected_language;
    int32_t     confidence_level;
    const char* original_bytestream;
    const char* converted_bytestream;
    const char* conversion_ts;
    bool        converted;
    bool        dropped_bytes;
} ConversionLog;

ConversionLog* populateConversionLog(
            ConversionLog* cl,
            const PGColResult* originalColResult,
            const PGColResult* convertedColResult,
            const char* uniqueKeyCols,
            const char* uniqueKeyValues,
            const char* encoding,
            const char* language,
            const int   confidence_level,
            const char* conversion_ts,
            const bool  converted,
            const bool  dropped_bytes);

PGconn* openDbConnection(const char* dsn);
void clean_exit(int exitStatus);

const char* getShortestUniqueIndex(const char* schema, const char* table,
                                    char** shortestUniqueIndexCast,
                                    char** shortestUniqueIndexCols,
                                    char** shortestUniqueIndexDataTypes);

char* getInitUniqueKeyValues(const char* schema, const char* table,
                             const char* uniqueKeyCols);

char* getNextUniqueKeyValues(const char* schema, const char* table,
                          const char* uniqueKeyCols, const char* prevUkValues);

const char* constructReadQuery(const Vector* cbColNames);

void getCBColNames(Vector *cn, const char* schema, const char* table);
void getCBColValues(Vector *cv, const char* readQuery,
                       const char* fullTableName,
                       const char* uniqueKeyCols,
                       const char* uniqueKeyValues);

char* constructWriteQuery(const char* fullTableName,
                     const Vector* colValues,
                     const char* uniqueKeyCols,
                     const char* uniqueKeyValues);

const char* transcode(PGColResult* colResult, const char* hint,
         char** encoding, char** lang, int32_t* confidence,
         char* conversion_ts, size_t conversion_ts_size,
         bool* converted, bool* dropped_bytes);

int printConversionLogHeader();

int printConversionLog(const ConversionLog* cl);

ConversionLog* newConversionLog();

void freeConversionLog(ConversionLog * cl);

const char* convertToHex(const char* bytes, const bool isnull, const int length);

#endif // #ifndef _TRANSCODER_H_
