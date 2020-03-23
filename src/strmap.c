#include "strmap.h"
#include <stdlib.h>
#include <string.h>
#include "math/prime.h"
#include <math.h>

static strmap_item STRMAP_DELETED_ITEM = {NULL, NULL};
#define STRMAP_BASE_SIZE 53

struct strmap
{
	uint32_t base_size;
	uint32_t size;
	uint32_t count;
	strmap_item** items;
};

// Creates a new pair
// Copies the data provided
// Size is the size of the arbitrary data in bytes
strmap_item* strmap_item_create(const char* key, void* data, uint32_t data_size)
{
	strmap_item* item = malloc(sizeof(strmap_item));
	item->key = malloc(strlen(key) + 1);
	strcpy(item->key, key);
	item->data = malloc(data_size);
	memcpy(item->data, data, data_size);
	item->data_size = data_size;
	return item;
}

void strmap_item_destroy(strmap_item* item)
{
	free(item->key);
	free(item->data);
	free(item);
}

static uint32_t hash_string(const char* key, uint32_t prime, uint32_t num_buckets)
{
	uint32_t hash = 0;
	const int s_len = strlen((char*)key);
	for (uint32_t i = 0; i < s_len; i++)
	{
		hash += (uint32_t)pow(prime, s_len - (i + 1)) * key[i];
		hash = hash % num_buckets;
	}
	return hash;
}

static uint32_t get_hash(const char* key, uint32_t num_buckets, uint32_t attempt)
{
	uint32_t hash_a = hash_string(key, 163, num_buckets);
	uint32_t hash_b = hash_string(key, 173, num_buckets);
	return (hash_a + (attempt * (hash_b + 1))) % num_buckets;
}

static strmap* strmap_create_sized(uint32_t size)
{
	strmap* map = malloc(sizeof(strmap));
	map->base_size = size;
	map->size = prime_next(size);
	map->count = 0;
	map->items = calloc(map->size, sizeof(strmap_item*));
	return map;
}

strmap* strmap_create()
{
	return strmap_create_sized(STRMAP_BASE_SIZE);
}

// Resizes the map passed as argument
static void strmap_resize(strmap* map, uint32_t new_size)
{
	if (new_size < STRMAP_BASE_SIZE)
		return;
	strmap* new_map = strmap_create_sized(new_size);
	for (uint32_t i = 0; i < map->size; i++)
	{
		strmap_item* item = map->items[i];
		if (item != NULL && item != &STRMAP_DELETED_ITEM)
		{
			strmap_insert(new_map, item->key, item->data, item->data_size);
		}
	}
	map->base_size = new_map->base_size;
	map->count = new_map->count;
	const uint32_t old_size = map->size;
	map->size = new_map->size;

	strmap_item** old_items = map->items;
	map->items = new_map->items;

	new_map->size = old_size;
	new_map->items = old_items;
	strmap_destroy(new_map);
}

static void strmap_resize_up(strmap* map)
{
	const uint32_t new_size = map->base_size * 2;
	strmap_resize(map, new_size);
}
static void strmap_resize_down(strmap* map)
{
	const uint32_t new_size = map->base_size / 2;
	strmap_resize(map, new_size);
}

void strmap_insert(strmap* map, const char* key, void* data, uint32_t size)
{
	// Table is full
	if ((map->count * 100 / map->size) > 50)
	{
		strmap_resize_up(map);
	}
	strmap_item* item = strmap_item_create(key, data, size);
	uint32_t index = get_hash(key, map->size, 0);

	// What is currently colliding the hash
	strmap_item* curr_item = map->items[index];
	uint32_t i = 1;
	// Jump over if curr_item is not NULL or if curr_item is deleted
	while (curr_item != NULL && curr_item != &STRMAP_DELETED_ITEM)
	{
		// Key is duplicate - update
		if (strcmp(curr_item->key, key) == 0)
		{
			strmap_item_destroy(curr_item);
			map->items[index] = item;
			return;
		}
		index = get_hash(item->key, map->size, i);
		curr_item = map->items[index];
		i++;
	}

	// Store item
	map->items[index] = item;
	map->count++;
}

void* strmap_find(strmap* map, const char* key)
{
	int index = get_hash(key, map->size, 0);
	strmap_item* item = map->items[index];
	int i = 1;
	// If item exists, go down collision chain
	while (item != NULL)
	{
		// Check if key matches
		if (item != &STRMAP_DELETED_ITEM)
		{
			if (strcmp(item->key, key) == 0)
			{
				return item->data;
			}
		}

		// Go to next item in chain
		index = get_hash(key, map->size, i);
		item = map->items[index];
		i++;
	}
	return NULL;
}

void strmap_remove(strmap* map, const char* key)
{
	int index = get_hash(key, map->size, 0);
	strmap_item* item = map->items[index];
	int i = 1;
	while (item != NULL)
	{
		if (item != &STRMAP_DELETED_ITEM)
		{
			if (strcmp(item->key, key) == 0)
			{
				strmap_item_destroy(item);
				map->items[index] = &STRMAP_DELETED_ITEM;
			}
		}
		index = get_hash(key, map->size, i);
		item = map->items[index];
		i++;
	}
	map->count--;
	if ((map->count * 100 / map->size) < 10)
	{
		strmap_resize_down(map);
	}
}

void strmap_destroy(strmap* map)
{
	for (uint32_t i = 0; i < map->size; i++)
	{
		if (map->items[i] != NULL)
		{
			strmap_item_destroy(map->items[i]);
		}
	}
	free(map->items);
	map->size = 0;
	map->count = 0;
	map->items = NULL;
	free(map);
}

strmap_item* strmap_index(strmap* map, uint32_t index)
{
	if (index >= map->count)
		return NULL;
	strmap_item* item = map->items[0];
	uint32_t i = 0, j = 0;
	while (item == NULL || item == &STRMAP_DELETED_ITEM || j <= index)
	{
		if (map->items[i] == NULL || map->items[i] == &STRMAP_DELETED_ITEM)
		{
			i++;
			continue;
		}
		j++;
		item = map->items[i];
		i++;
	}
	return item;
}

// Returns the amount of items in the map
uint32_t strmap_count(strmap* map)
{
	return map->count;
}