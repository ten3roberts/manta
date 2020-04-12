// Magpie is a small and low overhead library for keeping track of allocations and detecting memory leaks
// Stores all allocations from the program in a hashtable
// Stores where allocations came from and how many allocations have been done in the same place
// Checks for buffer overflows and double free
// Checks for leaked memory blocks at the end of the program with mp_terminate
// Leak checking is done by calling mp_terminate
// This will print out the information about the remaining blocks
// -> Where they were allocated as file:line, ctrl+click to follow in vscode
// -> How many bytes where allocated
// -> Which number off allocations from the same place it was, if allocating in a loop, this will show you which iteration of the loop did not get free
// mp_terminate will also free all remaining blocks and all internal resources, can safely be called if no allocations have happened
// =============================================================================

// To build the library do
// #define MP_IMPLEMENTATION in ONE C file to create the function implementations before including the header
// To configure the library, add the defines under =CONFIGURATION= above including the header in the same C file you
// defined MP_IMPLEMENTATION

// ==============================================================================

// CONFIGURATION
// MP_DISABLE to turn off storing and tracking of memory blocks and only keeps track of number of allocations by incremention and decremention
// -> This disabled almost the whole library including checks for leaks, pointer validity, overflow and almost all else
// -> Use in RELEASE builds
// -> Only available features will be message on failed allocation (malloc returns NULL), and allocation count
// -> Allocation size is not tracked since size of pointer cannot be known without tracking
// MP_REPLACE_STD to replace the standard malloc, calloc, realloc, and free
// MP_CHECK_OVERFLOW to be able to validate and detect overflows automatically on free or explicitely
// MP_BUFFER_PAD_LEN (default 5) sets the size of the padding in bytes for detecting overflows
// -> Higher values require a bit more memory and checking but catches sparse overflows better
// MP_BUFFER_PAD_VAL (default '#') sets the character or value to fill the padding with if MP_
// -> This value should be a character not often used to avoid false negatives since overflow can't be detected if the same character is written
// -> DO NOT use '\0' or 0 as it is the most common character to overflow
// MP_FILL_ON_FREE to fill buffer on free with MP_BUFFER_PAD_VAL, this is to avoid reading a pointers data after it has been freed and not overwritten by others
// MP_MESSAGE (default puts) define your own message callback
// MP_WARN_NULL to warn when freeing NULL pointer. This is allowed in the specifications of free, but may be a bug of a value that never got initialized

// MP_CHECK_FULL to define MP_REPLACE_STD, MP_CHECK_OVERFLOW, MP_FILL_ON_FREE

// License is at the end of the file

// https://github.com/ten3roberts/magpie

#ifndef MAGPIE_H
#define MAGPIE_H

#ifdef _STDLIB_H
#error "stdlib.h should not be included before magpie.h"
#endif
#include <stdint.h>
#include <stdlib.h>

#ifdef MP_CHECK_FULL
#ifndef MP_REPLACE_STD
#define MP_REPLACE_STD
#endif
#ifndef MP_CHECK_OVERFLOW
#define MP_CHECK_OVERFLOW
#endif
#ifndef MP_FILL_ON_FREE
#define MP_FILL_ON_FREE
#endif
#endif

#define MP_VALIDATE_OK		 0
#define MP_VALIDATE_INVALID	 -1
#define MP_VALIDATE_OVERFLOW -2

// Returns the total number of allocations made
size_t mp_get_total_count();

// Returns the total number of bytes allocated
size_t mp_get_total_size();

// Returns the current number of blocks allocated
size_t mp_get_count();

// Returns the current number of bytes allocated
size_t mp_get_size();

// Prints the locations of all [c,a,re]allocs and how many allocations was performed there
void mp_print_locations();

// Checks if any blocks remain to be freed
// Should only be run at the end of the program execution
// Uses the msg
// Returns how many blocks of memory that weren't freed
// Frees any remaining blocks
// Releases all internal resources
size_t mp_terminate();

// Checks for buffer overruns and pointer life
// Returns MP_VALIDATE_[OK,INVALID,OVERFLOW]
int mp_validate_internal(void* ptr, const char* file, uint32_t line);

void* mp_malloc_internal(size_t size, const char* file, uint32_t line);
void* mp_calloc_internal(size_t num, size_t size, const char* file, uint32_t line);
void* mp_realloc_internal(void* ptr, size_t size, const char* file, uint32_t line);
void mp_free_internal(void* ptr, const char* file, uint32_t line);

#define mp_validate(ptr)	  mp_validate_internal(ptr, __FILE__, __LINE__)
#define mp_malloc(size)		  mp_malloc_internal(size, __FILE__, __LINE__)
#define mp_calloc(num, size)  mp_calloc_internal(num, size, __FILE__, __LINE__)
#define mp_realloc(ptr, size) mp_realloc_internal(ptr, size, __FILE__, __LINE__)
#define mp_free(ptr)		  mp_free_internal(ptr, __FILE__, __LINE__)

// End of header
// Implementation
#ifdef MP_IMPLEMENTATION
#ifdef MP_REPLACE_STD
#undef malloc
#undef calloc
#undef realloc
#undef free
#endif
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#ifndef MP_MSG_LEN
#define MP_MSG_LEN 512
#endif
#ifndef MP_BUFFER_PAD_LEN
#define MP_BUFFER_PAD_LEN 5
#endif

#ifndef MP_BUFFER_PAD_VAL
#define MP_BUFFER_PAD_VAL '#'
#endif

#ifndef MP_MESSAGE
#define MP_MESSAGE(m) puts(m)
#endif

// The total number of allocations for the program
static size_t mp_total_alloc_count = 0;
// The total size of all allocation for the program
static size_t mp_total_alloc_size = 0;
// The current number of allocated blocks of memory
static size_t mp_alloc_count = 0;
// The number of bytes allocated
static size_t mp_alloc_size = 0;

#ifndef MP_DISABLE
// A memory block stored based on line of initial allocation in a binary tree
struct MemBlock
{
	size_t size;
	const char* file;
	uint32_t line;
	uint32_t count;
	struct MemBlock* next;
	char bytes[1];
};

struct MPHashTable
{
	// Describes the allocated amount of buckets in the hash table
	size_t size;
	// Describes how many buckets are in use
	size_t count;
	struct MemBlock** items;
};

// Describes the location of a malloc
// Used to track where allocations come from and how many has been allocated from the same place in the code
struct MPAllocLocation
{
	const char* file;
	uint32_t line;
	// How many allocations have been done at file:line
	// Does not decrement on free
	uint32_t count;
	struct MPAllocLocation *prev, *next;
};

static struct MPHashTable mp_hashtable = {0};
static struct MPAllocLocation* mp_locations = NULL;

// Hash functions from https://gist.github.com/badboy/6267743
#if SIZE_MAX == 0xffffffff // 32 bit
size mp_hash_ptr(void* ptr)
{
	int c2 = 0x27d4eb2d; // a prime or an odd constant
	key = (key ^ 61) ^ (key >>> 16);
	key = key + (key << 3);
	key = key ^ (key >>> 4);
	key = key * c2;
	key = key ^ (key >>> 15);
	return key;
}
#elif SIZE_MAX == 0xffffffffffffffff // 64 bit
size_t mp_hash_ptr(void* ptr)
{

	size_t key = (size_t)ptr;
	key = (~key) + (key << 21); // key = (key << 21) - key - 1;
	key = key ^ (key >> 24);
	key = (key + (key << 3)) + (key << 8); // key * 265
	key = key ^ (key >> 14);
	key = (key + (key << 2)) + (key << 4); // key * 21
	key = key ^ (key >> 28);
	key = key + (key << 31);

	// Fit to table
	// Since size is a power of two, it is faster than modulo
	return key & (mp_hashtable.size - 1);
}
#endif

// Inserts block and correctly resizes the hashtable
// Counts and increases how many allocations have come from the same file and line
void mp_insert(struct MemBlock* block, const char* file, uint32_t line);

// Resizes the list either up (1) or down (-1), does nothing if incorrect value
void mp_resize(int direction);

// Searches for the pointer in the tree
struct MemBlock* mp_search(void* ptr);

// Searches and removes a memblock storing the ptr from the hashmap
// Returns the memblock, or NULL if failed
struct MemBlock* mp_remove(void* ptr);
#endif

size_t mp_get_total_count()
{
	return mp_total_alloc_count;
}

size_t mp_get_total_size()
{
	return mp_total_alloc_size;
}

size_t mp_get_count()
{
	return mp_alloc_count;
}

size_t mp_get_size()
{
	return mp_alloc_size;
}

// Remove print locations
// Terminate function does nothing
// Remove validation function
// Make allocation functions simple wrappers that increment count
// Do not build hash table functions if MP_DISABLE is defined
#ifdef MP_DISABLE
void mp_print_locations()
{
	MP_MESSAGE("Failed to fetch locations since magpie is disabled in build");
}

size_t mp_terminate()
{
	MP_MESSAGE("Failed to fetch remaining blocks since magpie is disabled in build");
	return 0;
}

int mp_validate_internal(void* ptr, const char* file, uint32_t line)
{
	MP_MESSAGE("Failed to validate pointer since magpie is disabled in build");
	return MP_VALIDATE_OK;
}

void* mp_malloc_internal(size_t size, const char* file, uint32_t line)
{
	void* ptr = malloc(size);
	if (ptr == NULL)
	{
		char msg[MP_MSG_LEN];
		snprintf(msg, sizeof msg, "%s:%d Failed to allocate memory for %zu bytes", file, line, size);
		MP_MESSAGE(msg);
		return NULL;
	}
	mp_total_alloc_count++;
	mp_total_alloc_size += size;
	mp_alloc_count++;
	return ptr;
}

void* mp_calloc_internal(size_t num, size_t size, const char* file, uint32_t line)
{
	void* ptr = calloc(num, size);
	if (ptr == NULL)
	{
		char msg[MP_MSG_LEN];
		snprintf(msg, sizeof msg, "%s:%d Failed to allocate memory for %zu bytes", file, line, size);
		MP_MESSAGE(msg);
		return NULL;
	}
	mp_total_alloc_count++;
	mp_total_alloc_size += num * size;
	mp_alloc_count++;
	return ptr;
}

void* mp_realloc_internal(void* ptr, size_t size, const char* file, uint32_t line);

void mp_free_internal(void* ptr, const char* file, uint32_t line)
{
#ifdef MP_WARN_NULL
	if (ptr == NULL)
	{
		char msg[MP_MSG_LEN];
		snprintf(msg, sizeof msg, "%s:%u Freeing NULL pointer", file, line);
		MP_MESSAGE(msg);
		return;
	}
#endif
	mp_alloc_count--;
	free(ptr);
}

#else
void mp_print_locations()
{
	struct MPAllocLocation* it = mp_locations;
	while (it)
	{
		char msg[MP_MSG_LEN];
		snprintf(msg, sizeof msg, "Allocator at %s:%u made %u allocations", it->file, it->line, it->count);
		MP_MESSAGE(msg);
		it = it->next;
	}
}

size_t mp_terminate()
{
	char msg[MP_MSG_LEN];
	size_t remaining_blocks = 0;

	// Free remaining blocks
	for (size_t i = 0; i < mp_hashtable.size; i++)
	{
		struct MemBlock* it = mp_hashtable.items[i];
		struct MemBlock* next = NULL;
		while (it)
		{
			remaining_blocks++;
			next = it->next;
			snprintf(msg, sizeof msg,
					 "Memory block allocated at %s:%u with a size of %zu bytes has not been freed. Block was "
					 "allocation num %u",
					 it->file, it->line, it->size, it->count);
			MP_MESSAGE(msg);
			// Validate directly
			// Check integrity of buffer padding to detect overflows/overruns
			size_t i = 0;
			char* p = it->bytes + it->size;
			for (i = 0; i < MP_BUFFER_PAD_LEN; i++, p++)
			{
				if (*p != MP_BUFFER_PAD_VAL)
				{
					char msg[MP_MSG_LEN];
					snprintf(msg, sizeof msg, "Buffer overflow after %zu bytes on pointer %p allocated at %s:%u",
							 it->size, it->bytes, it->file, it->line);
					MP_MESSAGE(msg);
					return MP_VALIDATE_OVERFLOW;
				}
			}
			//free(it);
			it = next;
		}
	}
	snprintf(msg, sizeof msg, "A total of %zu memory blocks remain to be freed after program execution",
			 remaining_blocks);
	MP_MESSAGE(msg);
	if (mp_hashtable.items)
	{
		free(mp_hashtable.items);
		mp_hashtable.items = NULL;
		mp_hashtable.count = 0;
		mp_hashtable.size = 0;
	}

	// Free the location list
	struct MPAllocLocation* it = mp_locations;
	struct MPAllocLocation* next = NULL;
	while (it)
	{
		next = it->next;
		free(it);
		it = next;
	}
	mp_locations = NULL;
	return remaining_blocks;
}

int mp_validate_internal(void* ptr, const char* file, uint32_t line)
{
	struct MemBlock* block = mp_search(ptr);
	if (block == NULL)
	{
		char msg[MP_MSG_LEN];
		snprintf(msg, sizeof msg, "%s:%u Validation of invalid or already freed pointer with adress %p", file, line,
				 ptr);
		MP_MESSAGE(msg);
		return MP_VALIDATE_INVALID;
	}
#ifdef MP_CHECK_OVERFLOW
	// Check integrity of buffer padding to detect overflows/overruns
	size_t i = 0;
	char* p = block->bytes + block->size;
	for (i = 0; i < MP_BUFFER_PAD_LEN; i++, p++)
	{
		if (*p != MP_BUFFER_PAD_VAL)
		{
			char msg[MP_MSG_LEN];
			snprintf(msg, sizeof msg, "Buffer overflow after %zu bytes on pointer %p allocated at %s:%u", block->size,
					 ptr, block->file, block->line);
			MP_MESSAGE(msg);
			return MP_VALIDATE_OVERFLOW;
		}
	}
#endif
	return MP_VALIDATE_OK;
}

void* mp_malloc_internal(size_t size, const char* file, uint32_t line)
{
	// Allocate size for the block info and the buffer requested
	struct MemBlock* new_block = malloc(sizeof(struct MemBlock) + size - 1 + MP_BUFFER_PAD_LEN);

// Fill the padding with MP_BUFFER_PAD_VAL
#ifdef MP_CHECK_OVERFLOW
	memset(new_block->bytes + size, MP_BUFFER_PAD_VAL, MP_BUFFER_PAD_LEN);
#endif

	// Allocate request
	if (new_block == NULL)
	{
		char msg[MP_MSG_LEN];
		snprintf(msg, sizeof msg, "%s:%d Failed to allocate memory for %zu bytes", file, line, size);
		MP_MESSAGE(msg);
		return NULL;
	}
	mp_total_alloc_count++;
	mp_total_alloc_size += size;
	mp_alloc_count++;
	mp_alloc_size += size;
	new_block->size = size;
	new_block->file = file;
	new_block->line = line;
	new_block->next = NULL;

	// Insert
	mp_insert(new_block, file, line);

	return new_block->bytes;
}
void* mp_calloc_internal(size_t num, size_t size, const char* file, uint32_t line)
{
	// Allocate size for the block info and the buffer requested
	struct MemBlock* new_block = calloc(1, sizeof(struct MemBlock) + num * size - 1 + MP_BUFFER_PAD_LEN);

	// Fill the padding with MP_BUFFER_PAD_VAL
#ifdef MP_CHECK_OVERFLOW
	memset(new_block->bytes + num * size, MP_BUFFER_PAD_VAL, MP_BUFFER_PAD_LEN);
#endif

	// Allocate request

	if (new_block == NULL)
	{
		char msg[MP_MSG_LEN];
		snprintf(msg, sizeof msg, "%s:%u Failed to allocate memory for %zu bytes", file, line, size * num);
		MP_MESSAGE(msg);
		return NULL;
	}
	mp_total_alloc_count++;
	mp_total_alloc_size += num * size;
	mp_alloc_count++;
	mp_alloc_size += num * size;
	new_block->size = num * size;
	new_block->file = file;
	new_block->line = line;
	new_block->next = NULL;
	// Insert
	mp_insert(new_block, file, line);

	return new_block->bytes;
}
void* mp_realloc_internal(void* ptr, size_t size, const char* file, uint32_t line)
{
	// Allocate if ptr is NULL
	if (ptr == NULL)
		return mp_malloc_internal(size, file, line);

	// Free if size is 0
	if (ptr && size == 0)
	{
		mp_free_internal(ptr, file, line);
		return NULL;
	}
	struct MemBlock* block = mp_remove(ptr);
	if (block == NULL)
	{
		char msg[MP_MSG_LEN];
		snprintf(msg, sizeof msg, "%s:%u Reallocating invalid or already freed pointer with adress %p", file, line,
				 ptr);
		MP_MESSAGE(msg);
		return NULL;
	}
	mp_total_alloc_size -= block->size;
	mp_alloc_size -= block->size;
	struct MemBlock* new_block = realloc(block, sizeof(struct MemBlock) + size - 1 + MP_BUFFER_PAD_LEN);
	if (new_block == NULL)
	{

		char msg[MP_MSG_LEN];
		snprintf(msg, sizeof msg, "%s:%u Failed to reallocate memory from %zu to %zu bytes", file, line, block->size,
				 size);
		MP_MESSAGE(msg);
		return NULL;
	}
	mp_insert(new_block, file, line);
	mp_total_alloc_size += size;
	new_block->size = size;
	mp_alloc_size += size;
#ifdef MP_CHECK_OVERFLOW
	memset(new_block->bytes + size, MP_BUFFER_PAD_VAL, MP_BUFFER_PAD_LEN);
#endif
	return new_block->bytes;
}

void mp_free_internal(void* ptr, const char* file, uint32_t line)
{
#ifndef MP_WARN_NULL
	if (ptr == NULL)
	{
		return;
	}
#endif
	struct MemBlock* block = mp_remove(ptr);
	if (block == NULL)
	{
		char msg[MP_MSG_LEN];
		snprintf(msg, sizeof msg, "%s:%u Freeing invalid or already freed pointer with adress %p", file, line, ptr);
		MP_MESSAGE(msg);
		return;
	}
	mp_alloc_count--;
	mp_alloc_size -= block->size;

#ifdef MP_CHECK_OVERFLOW
	// Check integrity of buffer padding to detect overflows/overruns
	size_t i = 0;
	char* p = block->bytes + block->size;
	for (i = 0; i < MP_BUFFER_PAD_LEN; i++, p++)
	{
		if (*p != MP_BUFFER_PAD_VAL)
		{
			char msg[MP_MSG_LEN];
			snprintf(msg, sizeof msg, "Buffer overflow after %zu bytes on pointer %p allocated at %s:%u", block->size,
					 ptr, block->file, block->line);
			MP_MESSAGE(msg);
			break;
		}
	}
#endif
#ifdef MP_FILL_ON_FREE
	memset(block->bytes, MP_BUFFER_PAD_VAL, block->size);
#endif
	free(block);
}

void mp_insert(struct MemBlock* block, const char* file, uint32_t line)
{
	block->next = NULL;
	// Hash the pointer
	if (mp_hashtable.size == 0)
	{
		mp_hashtable.size = 16;
		mp_hashtable.items = calloc(mp_hashtable.size, sizeof(*mp_hashtable.items));
	}
	if (mp_hashtable.count + 1 >= mp_hashtable.size * 0.7)
	{
		mp_resize(1);
	}
	{
		// Takes the hash of the bytes pointer of the block
		size_t hash = mp_hash_ptr(block->bytes);
		struct MemBlock* it = mp_hashtable.items[hash];

		if (it == NULL)
		{
			mp_hashtable.items[hash] = block;
			mp_hashtable.count++;
		}

		// Chain if hash collision
		else
		{
			while (it->next)
			{
				it = it->next;
			}
			it->next = block;
		}
	}
	// Items are being reinserted
	if (file == NULL)
	{
		return;
	}
	// Location
	if (mp_locations == NULL)
	{
		mp_locations = malloc(sizeof(struct MPAllocLocation));
		mp_locations->count = 0;
		mp_locations->file = file;
		mp_locations->line = line;
		mp_locations->prev = NULL;
		mp_locations->next = NULL;
		block->count = mp_locations->count++;
		return;
	}
	struct MPAllocLocation* it = mp_locations;
	while (it)
	{

		if (it->file == file && it->line == line)
		{
			block->count = it->count++;

			// Make sure it is always sorted by biggest on head
			while (it->prev && it->prev->count < it->count)
			{
				struct MPAllocLocation* prev = it->prev;
				// Head is changing
				if (prev->prev == NULL)
				{
					mp_locations->prev = it;
					mp_locations->next = it->next;
					if (it->next)
						it->next->prev = mp_locations;
					it->next = mp_locations;
					mp_locations = it;
					it->prev = NULL;
					break;
				}
				prev->next = it->next;
				it->prev = prev->prev;
				prev->prev->next = it;
				prev->prev = it;

				it->next = prev;
			}
			return;
		}
		// At end
		if (it->next == NULL)
		{
			struct MPAllocLocation* new_location = malloc(sizeof(struct MPAllocLocation));
			new_location->count = 0;
			new_location->file = file;
			new_location->line = line;
			new_location->prev = it;
			new_location->next = NULL;
			it->next = new_location;
			block->count = new_location->count++;

			return;
		}
		it = it->next;
	}
}

void mp_resize(int direction)
{
	size_t old_size = mp_hashtable.size;
	if (direction == 1)
		mp_hashtable.size *= 2;
	else if (direction == -1)
		mp_hashtable.size /= 2;
	else
		return;

	struct MemBlock** old_items = mp_hashtable.items;
	mp_hashtable.items = calloc(mp_hashtable.size, sizeof(struct MemBlock*));

	// Count will be reincreased when reinserting items
	mp_hashtable.count = 0;
	// Rehash and insert
	for (size_t i = 0; i < old_size; i++)
	{
		struct MemBlock* it = old_items[i];
		// Save the next since it will be cleared in the iterator when inserting
		struct MemBlock* next = NULL;
		while (it)
		{
			next = it->next;
			mp_insert(it, NULL, 0);
			it = next;
		}
	}
	free(old_items);
}

struct MemBlock* mp_search(void* ptr)
{
	size_t hash = mp_hash_ptr(ptr);
	struct MemBlock* it = mp_hashtable.items[hash];

	// Search chain for the correct pointer
	while (it)
	{
		if (it->bytes == ptr)
		{
			return it;
		}
		it = it->next;
	}
	return NULL;
}

struct MemBlock* mp_remove(void* ptr)
{
	size_t hash = mp_hash_ptr(ptr);
	struct MemBlock* it = mp_hashtable.items[hash];
	struct MemBlock* prev = NULL;

	// Search chain for the correct pointer
	while (it)
	{
		if (it->bytes == ptr)
		{
			// Bucket gets removed, no more left in chain
			if (it->next == NULL)
				mp_hashtable.count--;
			if (prev) // Has a parent remove and reconnect chain
			{
				prev->next = it->next;
			}
			else // First one one chain, change head
			{
				mp_hashtable.items[hash] = it->next;
			}

			// Check for resize down
			if (mp_hashtable.count - 1 <= mp_hashtable.size * 0.4)
			{
				mp_resize(-1);
			}

			return it;
		}
		prev = it;
		it = it->next;
	}
	return NULL;
}
#endif
#endif

#ifdef MP_REPLACE_STD
#define malloc(size)	   mp_malloc_internal(size, __FILE__, __LINE__);
#define calloc(num, size)  mp_calloc_internal(num, size, __FILE__, __LINE__);
#define realloc(ptr, size) mp_realloc_internal(ptr, size, __FILE__, __LINE__);
#define free(ptr)		   mp_free_internal(ptr, __FILE__, __LINE__);
#endif

#endif

/*--------------------------------------
LICENSE
----------------------------------------
MIT License

Copyright (c) 2020 Tim Roberts

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
