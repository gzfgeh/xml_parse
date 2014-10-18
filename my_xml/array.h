#ifndef ARRAY_H_
#define AEEAY_H_

#include <string.h>
#include <stdlib.h>
#include <assert.h>

extern struct array *array_create(int cnt);
extern int array_add(struct array *arr, const void *elem);
extern int array_delete(struct array*arr, int index);
extern int array_destroy(struct array *arr);
extern int array_length(struct array *arr);

#endif