#ifndef HANDLEPOOL_H
#define HANDLEPOOL_H
#include <stdint.h>
#include "handle.h"


DEFINE_HANDLE(GenericHandle)

#define HANDLEPOOL_INDEX(pool, i) ((struct handle_wrapper*)((uint8_t*)(pool)->handles + (pool)->stride * (i)))

struct handle_wrapper
{
	GenericHandle handle;
	// Points to next free element if free
	struct handle_wrapper* next;
	// The data that immediately follows in memory
	uint8_t data[];
};

typedef struct handlepool_t
{
	// The size of each element, does not include the handle
	uint32_t stride;
	// The element size of the array
	uint32_t size;
	// The count of live handles
	uint32_t count;
	// How many elements the pool will grow when out of size
	uint32_t grow_size;

	// Contains the handle and data
	// The handle contains an index and a unique pattern
	struct handle_wrapper* handles;
	// Linked list of the free handles
	struct handle_wrapper* free_handles;
} handlepool_t;

#define HANDLEPOOL_INIT(elem_size)                                                                           \
	(handlepool_t)                                                                                                      \
	{                                                                                                                   \
		.stride = elem_size + sizeof(struct handle_wrapper), .size = 0, .count = 0, .grow_size = 64, .handles = NULL, .free_handles = NULL \
	}

// Returns a wrapper containing both the handle and the raw data
// The handle can be copied and returned
// The raw data pointer should not be stored longterm or between function calls
const struct handle_wrapper* handlepool_alloc(handlepool_t* pool);

// Frees a handle
// Checks if handle is still valid or double free
void handlepool_free(handlepool_t* pool, GenericHandle handle);

// Returns the raw pointer to the data in the handle
// Should not be stored longterm or between function calls
void* handlepool_get_raw(handlepool_t* pool, GenericHandle handle);

#endif
