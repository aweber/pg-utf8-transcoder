/*
 * colresult.c
 *
 * Copyright © 2014-2015 AWeber Communications
 * All rights reserved.
 */

#include <string.h>
#include "colresult.h"

// deep copy of one PGColResult to another
PGColResult* copyColResult(const PGColResult* const src, PGColResult* dest)
{
    dest = malloc(sizeof(PGColResult));

    dest->fname     = strdup(src->fname);
    dest->fnumber   = src->fnumber;
    dest->ftable    = src->ftable;
    dest->ftablecol = src->ftablecol;
    dest->fformat   = src->fformat;
    dest->ftype     = src->ftype;
    dest->fmod      = src->fmod;
    dest->fsize     = src->fsize;
    dest->isnull    = src->isnull;
    dest->length    = src->length;
    dest->value     = strdup(src->value);

    return dest;
}

// free a PGColResult
void freeColResult(PGColResult* cr)
{
    free((void *) cr->fname);
    free((void *) cr->value);
    free((void *) cr);
}

// set char* fname.  free it first in case its already populated
char* colResultSetFname(PGColResult* const cr, const char* fname)
{
    free(cr->fname);
    cr->fname = strdup(fname);
    return cr->fname;
}

// set char* value.  free it first in case its already populated
char* colResultSetValue(PGColResult* const cr, const char* value)
{
    free(cr->value);
    cr->value = strdup(value);
    cr->length = strlen(value);
    return cr->value;
}

// convenience function to see if a column is null
// probably unnecessary encapsulation
// this is what happens when you write too much C++
// (or python, or java, or ...)
bool colResultIsNULL(const PGColResult* const cr)
{
    return cr->isnull;
}

// convenience function to see if a column holds an empty string
bool colResultIsEmptyString(const PGColResult* const cr)
{
    return (cr->isnull == false && cr->length == 0);
}