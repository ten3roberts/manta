#ifndef HANDLETABLE_H
#define HANDLETABLE_H
#include <stdint.h>
#include <stdio.h>
#include "handlepool.h"

// Implement a simple dynamic handletable in C
// To build the library do
// #define HANDLETABLE_IMPLEMENTATION in ONE C file to create the function implementations before including the header
// To configure the library, add the define under <CONFIGURATION> above the header include in the C file declaring the implementation
// handletable can be safely included several times, but only one C file can define HANDLETABLE_IMPLEMENTATION

// CONFIGURATION
// HANDLETABLE_SIZE_TOLERANCE (70) sets the tolerance in percent that will cause the table to resize
// -> If the table count is HANDLETABLE_SIZE_TOLERANCE of the total size, it will resize, likewise down
// -> If set to 0, the handletable will not resize
// -> Value is clamped to >= 50 if not 0
// HANDLETABLE_DEFAULT_SIZE (default 16) decides the default size of the handletable
// -> Table will resize up and down in powers of two automatically
// -> Note, must be a power of 2
// HANDLETABLE_MALLOC, HANDLETABLE_CALLOC, and HANDLETABLE_FREE to define your own allocators
// handletable_create to make a wrapper for handletable_create_internal allowing for custom leak detection

// Basic usage
// handletable_t storing arbitrary type with string keys
// handletable_t* table = handletable_create_string();

// Adding data to the handletable
// handletable_insert(table, "Testkey", &mystruct)
// If an entry of that name already exists, it is replaced and returned

// The table only stores a pointer to the key and data stored
// This means that when an entry is removed, it's value and key is not freed
// This means that the data and key can be stack allocated if kept in scope
// Keys can also be literals like "ABC"

// If you want to dynamically allocate a string as key, it is recommended to store it withing the struct
// handletable_insert(table, mystruct.name, &mystruct)
// This also makes insertion of structs easy if they store a name or similar identifier withing them

// Custom key types
// To create a handletable with a custom type of key you need to create a hashfunction and a comparefunction
// The hashfunction takes in a void* of the key and returns a uint32_t as a hash, the more spread the hashfunction is, the better
// The compare function takes a void* of the key and returns 0 if they match

// To then create the handletable with your custom functions, use handletable_create(hashfunc, compfunc)
// Usage is exactly like a handletable storing strings or any other type

// See end of file for license

typedef struct handletable_t Handletable;
typedef struct handletable_t handletable_t;

typedef struct handletable_iterator handletable_iterator;

// Creates a handletable with the specified functionality
// Keyfunc returns a pointer to a key
// hashfunc should be the function that generates a hash from the key returned by keyfunc
// compfunc compares the key returned by keyfunc and a key and returns 0 on match
// Macro can be changed to allow for custom leak detection but needs to call handletable_create_internal
#ifndef handletable_create
#define handletable_create(keyfunc, hashfunc, compfunc) handletable_create_internal(keyfunc, hashfunc, compfunc);
#endif

// Creates a handletable with the string hash function
// Shorthand for handletable_create(handletable_hashfunc_string, handletable_comp_string);
#define handletable_create_string() handletable_create(handletable_hashfunc_string, handletable_comp_string)
#define handletable_create_uint32() handletable_create(handletable_hashfunc_uint32, handletable_comp_uint32)

handletable_t* handletable_create_internal(const void* (*keyfunc)(GenericHandle), uint32_t (*hashfunc)(const void*),
										   int32_t (*compfunc)(const void*, const void*));

// Inserts an item associated with a key into the handletable
// If key already exists, it is returned and replaced
GenericHandle handletable_insert(handletable_t* handletable, GenericHandle handle);

// Finds and returns an item from the handletable
// Returns INVALID(GenericHandle) if not found
GenericHandle handletable_find(handletable_t* handletable, const void* key);

// Removes and returns an item from a handletable
GenericHandle handletable_remove(handletable_t* handletable, const void* key);

// Removes and returns the first element in the handletable
// Returns NUL when table is empty
// Can be used to clear free the stored data before handletable_destroy
GenericHandle handletable_pop(handletable_t* handletable);

// Returns how many items are in the handletable
uint32_t handletable_get_count(handletable_t* handletable);

// Destroys and frees a handletable
// Does not free the stored data
void handletable_destroy(handletable_t* handletable);

// Starts and returns an iterator
// An empty table returns a valid iterator, but handletable_iterator_next will return NULL directly
handletable_iterator* handletable_iterator_begin(handletable_t* handletable);
// Returns data at location and moves to the next
// Bahaviour is undefines if handletable resizes
GenericHandle handletable_iterator_next(handletable_iterator* it);
// Ends and frees an iterator
void handletable_iterator_end(handletable_iterator* it);

// Predefined hash function
uint32_t handletable_hashfunc_uint32(const void* pkey);
// Predefined compare function
int32_t handletable_comp_uint32(const void* pkey1, const void* pkey2);

// Predefined hash function
uint32_t handletable_hashfunc_string(const void* pkey);
// Predefined compare function
int32_t handletable_comp_string(const void* pkey1, const void* pkey2);

#ifdef HANDLETABLE_IMPLEMENTATION

#ifndef HANDLETABLE_DEFAULT_SIZE
#define HANDLETABLE_DEFAULT_SIZE 16
#endif
#if HANDLETABLE != 0 && HANDLETABLE_SIZE_TOLERANCE < 50
#define HANDLETABLE_SIZE_TOLERANCE 50
#endif

#ifndef HANDLETABLE_SIZE_TOLERANCE
#define HANDLETABLE_SIZE_TOLERANCE 70
#endif

#ifndef HANDLETABLE_MALLOC
#define HANDLETABLE_MALLOC(s) malloc(s)
#endif

#ifndef HANDLETABLE_CALLOC
#define HANDLETABLE_CALLOC(n, s) calloc(n, s)
#endif

#ifndef HANDLETABLE_FREE
#define HANDLETABLE_FREE(p) free(p)
#endif

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct handletable_item
{
	GenericHandle handle;
	// For collision chaining
	struct handletable_item* next;
};

struct handletable_t
{
	const void* (*keyfunc)(GenericHandle);
	uint32_t (*hashfunc)(const void*);
	int32_t (*compfunc)(const void*, const void*);

	// The amount of buckets in the list
	uint32_t size;

	// How many items are in the table, including
	uint32_t count;
	struct handletable_item** items;
};

struct handletable_iterator
{
	handletable_t* table;
	struct handletable_item* item;
	uint32_t index;
};

handletable_t* handletable_create_internal(const void* (*keyfunc)(GenericHandle), uint32_t (*hashfunc)(const void*),
										   int32_t (*compfunc)(const void*, const void*))
{
	handletable_t* handletable = HANDLETABLE_MALLOC(sizeof(handletable_t));
	handletable->keyfunc = keyfunc;
	handletable->hashfunc = hashfunc;
	handletable->compfunc = compfunc;
	handletable->count = 0;
	handletable->size = HANDLETABLE_DEFAULT_SIZE;
	handletable->items = HANDLETABLE_CALLOC(HANDLETABLE_DEFAULT_SIZE, sizeof(struct handletable_item*));
	return handletable;
}

// Inserts an already allocated item struct into the hash table
// Does not resize the hash table
// Does no increase count
static GenericHandle handletable_insert_internal(handletable_t* handletable, struct handletable_item* item)
{
	// Discard the next since collision chain will be reevaluated
	item->next = NULL;
	// Calculate the hash with the provided hash function to determine index
	uint32_t index = handletable->hashfunc(handletable->keyfunc(item->handle));
	// Make sure hash fits inside table
	index = index & (handletable->size - 1);
	// Slot is empty, no collision
	struct handletable_item* cur = handletable->items[index];
	if (cur == NULL)
	{
		handletable->items[index] = item;
	}
	// Insert at tail if collision
	// Check for duplicate
	else
	{
		struct handletable_item* prev = NULL;
		while (cur)
		{
			// Duplicate
			if (handletable->compfunc(handletable->keyfunc(cur->handle), handletable->keyfunc(item->handle)) == 0)
			{
				// Handle beginning
				if (prev == NULL)
					handletable->items[index] = item;
				else
					prev->next = item;

				item->next = cur->next;

				GenericHandle retdata = cur->handle;
				HANDLETABLE_FREE(cur);
				return retdata;
			}
			prev = cur;
			cur = cur->next;
		}
		// Insert at tail
		prev->next = item;
	}
	return INVALID(GenericHandle);
}

// Resizes the list either up (1) or down (-1)
// Internal function
static void handletable_resize(handletable_t* handletable, int32_t direction)
{
	// Save the old values
	uint32_t old_size = handletable->size;
	struct handletable_item** old_items = handletable->items;

	if (direction == 1)
		handletable->size = handletable->size << 1;
	else if (direction == -1)
		handletable->size = handletable->size >> 1;
	else
		return;

	// Allocate the larger list
	handletable->items = HANDLETABLE_CALLOC(handletable->size, sizeof(struct handletable_item*));

	for (uint32_t i = 0; i < old_size; i++)
	{
		struct handletable_item* cur = old_items[i];
		struct handletable_item* next = NULL;
		while (cur)
		{
			// Save the next since it will be changed with insert
			next = cur->next;

			// Reinsert into the new list
			// This won't resize the table again
			handletable_insert_internal(handletable, cur);

			cur = next;
		}
	}
	HANDLETABLE_FREE(old_items);
}

GenericHandle handletable_insert(handletable_t* handletable, GenericHandle handle)
{
	// Check if table needs to be resized before calculating the hash as it will change
	if (++handletable->count * 100 >= handletable->size * HANDLETABLE_SIZE_TOLERANCE)
		handletable_resize(handletable, 1);

	struct handletable_item* item = HANDLETABLE_MALLOC(sizeof(struct handletable_item));
	item->handle = handle;
	item->next = NULL;
	return handletable_insert_internal(handletable, item);
}

GenericHandle handletable_find(handletable_t* handletable, const void* key)
{
	uint32_t index = handletable->hashfunc(key);
	// Make sure hash fits inside table
	index = index & (handletable->size - 1);
	struct handletable_item* cur = handletable->items[index];

	while (cur)
	{
		// Match
		if (handletable->compfunc(handletable->keyfunc(cur->handle), key) == 0)
		{
			return cur->handle;
		}
		cur = cur->next;
	}

	return INVALID(GenericHandle);
}

// Removes and returns an item from a handletable
GenericHandle handletable_remove(handletable_t* handletable, const void* key)
{
	uint32_t index = handletable->hashfunc(key);
	// Make sure hash fits inside table
	index = index & (handletable->size - 1);

	struct handletable_item* cur = handletable->items[index];
	struct handletable_item* prev = NULL;

	while (cur)
	{
		// Match
		if (handletable->compfunc(handletable->keyfunc(cur->handle), key) == 0)
		{
			// Handle beginning
			if (prev == NULL)
				handletable->items[index] = cur->next;
			else
				prev->next = cur->next;

			GenericHandle cur_handle = cur->handle;
			HANDLETABLE_FREE(cur);

			// Check if table needs to be resized down after removing item
			if (--handletable->count * 100 < handletable->size * (100 - HANDLETABLE_SIZE_TOLERANCE))
				handletable_resize(handletable, -1);

			return cur_handle;
		}
		prev = cur;
		cur = cur->next;
	}
	return INVALID(GenericHandle);
}

GenericHandle handletable_pop(handletable_t* handletable)
{
	for (uint32_t i = 0; i < handletable->size; i++)
	{
		struct handletable_item* cur = handletable->items[i];
		if (cur != NULL)
		{
			handletable->items[i] = cur->next;
			GenericHandle cur_handle = cur->handle;
			HANDLETABLE_FREE(cur);

			// Check if table needs to be resized down after removing item
			if (--handletable->count * 100 <= handletable->size * (100 - HANDLETABLE_SIZE_TOLERANCE))
				handletable_resize(handletable, -1);

			return cur_handle;
		}
	}

	return INVALID(GenericHandle);
}

uint32_t handletable_get_count(handletable_t* handletable)
{
	return handletable->count;
}

void handletable_destroy(handletable_t* handletable)
{
	for (uint32_t i = 0; i < handletable->size; i++)
	{
		struct handletable_item* cur = handletable->items[i];
		struct handletable_item* next = NULL;
		while (cur)
		{
			next = cur->next;
			HANDLETABLE_FREE(cur);
			cur = next;
		}
	}
	HANDLETABLE_FREE(handletable->items);
	HANDLETABLE_FREE(handletable);
}

// Starts and returns an iterator
handletable_iterator* handletable_iterator_begin(handletable_t* handletable)
{
	handletable_iterator* it = HANDLETABLE_MALLOC(sizeof(handletable_iterator));
	it->table = handletable;
	it->item = NULL;
	// Find first slot/bucket with data
	for (it->index = 0; it->index < handletable->size; it->index++)
	{
		if (handletable->items[it->index] != NULL)
		{
			it->item = handletable->items[it->index];
			break;
		}
	}
	return it;
}
// Returns data at location and moves to the next
GenericHandle handletable_iterator_next(handletable_iterator* it)
{
	// Iterator has reached end
	if (it->item == NULL)
		return INVALID(GenericHandle);

	GenericHandle prev_handle = it->item->handle;
	// Move to next in chain
	if (it->item->next)
	{
		it->item = it->item->next;
		return prev_handle;
	}

	// Look for next slot/bucket
	handletable_t* table = it->table;
	for (++it->index; it->index < table->size; it->index++)
	{
		if (table->items[it->index] != NULL)
		{
			it->item = table->items[it->index];
			return prev_handle;
		}
	}
	// At end
	it->item = NULL;
	return prev_handle;
}
// Ends and frees an iterator
void handletable_iterator_end(handletable_iterator* iterator)
{
	HANDLETABLE_FREE(iterator);
}

// Common Hash functions
uint32_t handletable_hashfunc_uint32(const void* pkey)
{
	uint32_t key = *(uint32_t*)pkey;
	key = ((key >> 16) ^ key) * 0x45d9f3b;
	key = ((key >> 16) ^ key) * 0x45d9f3b;
	key = (key >> 16) ^ key;
	return key;
}

int32_t handletable_comp_uint32(const void* pkey1, const void* pkey2)
{
	return (*(uint32_t*)pkey1 == *(uint32_t*)pkey2) == 0;
}

uint32_t handletable_hashfunc_string(const void* pkey)
{
	char* key = (char*)pkey;
	uint64_t value = 0;
	uint32_t i = 0;
	uint32_t lkey = strlen(key);

	for (; i < lkey; i++)
	{
		value = value * 37 + key[i];
	}
	return value;
}

int32_t handletable_comp_string(const void* pkey1, const void* pkey2)
{
	return strcmp(pkey1, pkey2);
}

#endif
#endif

// ========LICENSE========
// MIT License
//
// Copyright (c) 2020 Tim Roberts
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.