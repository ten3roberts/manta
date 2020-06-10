#include "handlepool.h"
#include <stdlib.h>
#include "log.h"

// Returns a wrapper containing both the handle and the raw data
// The handle can be copied and returned
// The raw data pointer should not be stored longterm or between function calls
const struct handle_wrapper* handlepool_alloc(handlepool_t* pool)
{
	if (pool->free_handles)
	{
		struct handle_wrapper* wrapper = pool->free_handles;
		pool->free_handles = pool->free_handles->next;
		wrapper->next = NULL;
		wrapper->handle.pattern += 1;
		pool->count++;
		return wrapper;
	}

	// Pool is not full
	if (pool->count < pool->size)
	{
		struct handle_wrapper* wrapper = HANDLEPOOL_INDEX(pool, pool->count);
		pool->count++;
		wrapper->next = NULL;
		return wrapper;
	}

	// Resize pool
	uint32_t old_size = pool->size;
	pool->size += pool->grow_size;
	if (pool->size >= 2 << MAX_HANDLE_INDEX_BITS)
	{
		LOG_E("Maximum number of handles in pool reached");
		return NULL;
	}
	pool->handles = realloc(pool->handles, pool->size * pool->stride);

	// Initialize new handles
	for (uint32_t i = old_size; i < pool->size; i++)
	{
		struct handle_wrapper* wrapper = HANDLEPOOL_INDEX(pool, i);
		wrapper->next = NULL;
		wrapper->handle.index = i;
		wrapper->handle.pattern = 0;
	}

	pool->count++;
	return HANDLEPOOL_INDEX(pool, pool->count - 1);
}

// Frees a handle
// Checks if handle is still valid or double free
void handlepool_free(handlepool_t* pool, GenericHandle handle)
{
	if(HANDLE_COMPARE(handle, INVALID(GenericHandle)))
	{
		LOG_E("Handle is invalid");
		return;
	}

	if (handle.index >= pool->size)
	{
		LOG_E("Handle %d is not a valid slot index", handle.index);
		return;
	}

	struct handle_wrapper* wrapper = HANDLEPOOL_INDEX(pool, handle.index);

	if (wrapper->next != NULL || !HANDLE_COMPARE(wrapper->handle, handle))
	{
		LOG_E("Handle %d has already been freed", handle.index);
		return;
	}

	// Insert into linked list of free handles
	wrapper->next = pool->free_handles;
	pool->free_handles = wrapper;

	pool->count--;
	if (pool->count == 0)
	{
		free(pool->handles);
		pool->handles = NULL;
		pool->size = 0;
		pool->count = 0;
		pool->free_handles = NULL;
	}
}

void* handlepool_get_raw(handlepool_t* pool, GenericHandle handle)
{
	if(HANDLE_COMPARE(handle, INVALID(GenericHandle)))
	{
		LOG_E("Handle is invalid");
		return NULL;
	}

	if (handle.index >= pool->size)
	{
		LOG_E("Handle %d is not a valid slot index", handle.index);
		return NULL;
	}

	struct handle_wrapper* wrapper = HANDLEPOOL_INDEX(pool, handle.index);

	if (wrapper->next != NULL || !HANDLE_COMPARE(wrapper->handle, handle))
	{
		LOG_E("Handle %d has already been freed", handle.index);
		return NULL;
	}

	return wrapper->data;
}