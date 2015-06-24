/*
 * colresult.h
 *
 * Copyright © 2014-2015 AWeber Communications
 * All rights reswerved.
 */

#ifndef _COLRESULT_H_
#define _COLRESULT_H_

#include <libpq-fe.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct
{
    char*   fname;          // field name; NULL if column number is out of range
    int     fnumber;        // field number in query; starts at 0
    Oid     ftable;         // table oid for query
    int     ftablecol;      // column number within table; always > 0; 0 if column number is out of range
    int     fformat;        // format code for column; 0 == text, 1 == binary
    Oid     ftype;          // data type of column; references pg_type; built-ins in src/include/catalog/pg_type.h
    int     fmod;           // type-specific, typically precision or size limits; -1 means "no info"
    int     fsize;          // space allocated for column in db row, i.e. internal rep of data type;
                            // negative values indicate data type is variable length
    bool    isnull;         // 1 if NULL, 0 if non-null.  PQgetvalue returns empty string for NULL and
                            // empty string
    int     length;         // actual length of the field value in bytes.  semantics similar to strlen()
                            // NOT THE SAME THING AS FSIZE!!!
    char*   value;          // column value as a char*
} PGColResult;

void freeColResult(PGColResult* cr);

PGColResult* copyColResult(const PGColResult* const src, PGColResult* dest);

bool colResultIsNULL(const PGColResult* const cr);
bool colResultIsEmptyString(const PGColResult* const cr);

char* colResultSetFname(PGColResult* const cr, const char* fname);
char* colResultSetValue(PGColResult* const cr, const char* value);

#endif // #ifndef _COLRESULT_H_