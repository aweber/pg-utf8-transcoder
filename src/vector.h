/*
 * vector.h
 *
 * Copyright © 2014-2015 AWeber Communications
 * All rights reserved.
 */

#ifndef _VECTOR_H_
#define _VECTOR_H_

#define VECTOR_INITIAL_CAPACITY 10

// Define a vector type
typedef struct
{
  unsigned int size;      // slots used so far
  unsigned int capacity;  // total available slots
  char* type;             // type of member "data"
  void** data;            // anything at all (careful!)
} Vector;

unsigned int vector_init(Vector *vector, const char* type, unsigned int initialCapacity);

unsigned int vector_append(Vector *vector, void* value);

void* vector_get(Vector *vector, unsigned int index);

void vector_set(Vector *vector, unsigned int index, void* value);

void vector_double_capacity_if_full(Vector *vector);

void vector_free(Vector *vector);

void vector_clear(Vector *vector);

#endif // #ifndef _VECTOR_H_
