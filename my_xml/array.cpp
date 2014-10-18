#include "array.h"

#define ELECNT 100

struct array
{
	int element_size;
	int element_cnt;
	void *buffer;
	int buffer_size;
};

struct array *array_create(int element_size)
{
	assert(element_size);

	struct array *p = (struct array *)malloc(sizeof(struct array));
	if (p == NULL)
		return NULL;

	memset(p, 0, sizeof(struct array));
	p->element_size = element_size;
	return p;
}

int array_add(struct array *arr, const void *elem)
{
	int length;

	assert(arr);
	assert(elem);

	length = arr->element_size * (arr->element_cnt + 1);
	if (length > arr->buffer_size)
	{
		arr->buffer_size = length * 2;
		arr->buffer = (char *)realloc(arr->buffer, arr->buffer_size);
		if (arr->buffer == NULL)
			return -1;
	}

	memcpy((char *)arr->buffer + arr->element_cnt*arr->element_size, elem, arr->element_size);
	arr->element_cnt++;
	return 0;
}

int array_delete(struct array*arr, int index)
{
	assert(arr);
	assert(index < arr->element_cnt);

	memcpy((struct array *)arr->buffer + arr->element_size*index, (struct array*)arr->buffer + arr->element_size*(index + 1), arr->element_cnt - index-1);
	arr->element_cnt--;
	return 0;
}

int array_destroy(struct array *arr)
{
	assert(arr);

	if (arr->buffer != NULL)
    {
		free(arr->buffer);
        arr->buffer = NULL;
    }

	if (arr != NULL)
    {
		free(arr);
        arr = NULL;
    }
	return 0;
}

int array_length(struct array *arr)
{
	assert(arr);
	return arr->element_cnt;
}

