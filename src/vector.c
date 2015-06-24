/*
 * vector.h
 *
 * Copyright © 2014-2015 AWeber Communications
 * All rights reserved.
 */

// pick up vasprintf
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include "vector.h"
#include "colresult.h"

unsigned int vector_init(Vector *vector, const char* type, unsigned int initialCapacity) {
  // initialize size and capacity
  vector->size = 0;
  vector->type = strdup(type);
  if (initialCapacity == 0)
    vector->capacity = VECTOR_INITIAL_CAPACITY;
  else
    vector->capacity = initialCapacity;

  // allocate memory for vector->data
  vector->data = calloc(vector->capacity, sizeof(void *));
  return vector->capacity;
}

unsigned int vector_append(Vector *vector, void* value) {
  // make sure there's room to expand into
  vector_double_capacity_if_full(vector);

  // append the value and increment vector->size
  vector->data[vector->size++] = value;
  return vector->size;
}

void* vector_get(Vector *vector, unsigned int index) {
  if (index >= vector->size) {
    printf("Index %d out of bounds for vector of size %d\n", index, vector->size);
    fflush(NULL);
    raise (SIGABRT);  // dump core
  }
  return vector->data[index];
}

void vector_set(Vector *vector, unsigned int index, void* value) {
  // zero fill the vector up to the desired index
  while (index >= vector->size) {
    vector_append(vector, NULL);
  }

  // set the value at the desired index
  vector->data[index] = value;
}

void vector_double_capacity_if_full(Vector *vector) {
  if (vector->size >= vector->capacity) {
    // double vector->capacity and resize the allocated memory accordingly
    vector->capacity *= 2;
    vector->data = realloc(vector->data, sizeof(void*) * vector->capacity);
  }
}

void vector_free(Vector *vector) {
  vector_clear(vector);
  if (vector->data)
    free(vector->data);
  free((void *) vector->type);
}

void vector_clear(Vector *vector)
{
    unsigned int index = 0;
    for(index = 0; index < vector->size; index++)
    {
        if (vector->data[index])
        {
            if (strcmp(vector->type, "PGColResult*") == 0)
                freeColResult(vector->data[index]);
            else
                free(vector->data[index]);

            vector->data[index] = NULL;
        }
    }
}
