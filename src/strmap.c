#include "strmap.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>

static strmap_item STRMAP_DELETED_ITEM = {NULL, NULL};

struct strmap
{
	uint32_t size;
	uint32_t count;
	strmap_item** items;
};

// Creates a new pair
// Copies the data provided
// Size is the size of the arbitrary data in bytes
strmap_item* strmap_item_create(const char* key, void* data, uint32_t size)
{
	strmap_item* item = malloc(sizeof(strmap_item));
	item->key = malloc(strlen(key));
	memcpy(item->key, key, strlen(key));
	item->data = malloc(size);
	memcpy(item->data, data, size);
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
		hash += (uint32_t)pow(prime, s_len - (i + 1) )* key[i];
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

strmap* strmap_create()
{
	strmap* map = malloc(sizeof(strmap));
	map->size = 53;
	map->count = 0;
	map->items = calloc(map->size, sizeof(strmap_item*));
	return map;
}

void strmap_insert(strmap* map, const char* key, void* data, uint32_t size)
{
	strmap_item* item = strmap_item_create(key, data, size);
	uint32_t index = get_hash(item->key, map->size, 0);

	// What is currently colliding the hash
	strmap_item* curr_item = map->items[index];
	uint32_t i = 1;
	// Jump over if curr_item is not NULL or if curr_item is deleted
	while (curr_item != NULL && curr_item != &STRMAP_DELETED_ITEM)
	{
		// Key is duplicate - update
		if(strcmp(curr_item->key, key) == 0)
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

/*
struct Map
{
	uint32_t (*hash_func)(void* in, uint32_t prime, uint32_t num_buckets);
	uint32_t size;
	uint32_t count;
	map_item** items;
};

map_item* map_item_create(void* key, void* data, uint32_t size)
{
	map_item* item = malloc(sizeof(map_item));
item->data = malloc(size);
	memcpy(item->key, data, size);
	item->key = malloc(size);
	memcpy(item->data, data, size);
	return item;
}

void map_item_destroy(map_item* item)
{
	free(item->data);
	free(item);
}

Map* map_create(uint32_t (*hash_func)(void* key, uint32_t prime, uint32_t num_buckets))
{
	Map* map = malloc(sizeof(Map));
	map->hash_func = hash_func;
	map->size = 53;
	map->count = 0;
	map->items = calloc(map->size, sizeof(map_item));
	return map;
}

void map_insert(Map* map, void* data)
{
	// Attempt to insert
	for (uint32_t i = 0;; i++)
	{

		uint32_t hash_a = map->hash_func(data, 163, map->size);
		uint32_t hash_b = map->hash_func(data, 173, map->size);
		(hash_a + (i * (hash_b + 1))) % map->size;
	}
}

void map_destroy(Map* map)
{
	for (uint32_t i = 0; i < map->size; i++)
	{
		if (map->items[i] != NULL)
		{
			map_item_destroy(map->items[i]);
		}
	}
	free(map->items);
	map->size = 0;
	map->count = 0;
	map->items = NULL;
	free(map);
}

uint32_t map_hash_string(void* key, uint32_t prime, uint32_t num_buckets)
{
	uint32_t hash = 0;
	const int s_len = strlen((char*)key);
	for (uint32_t i = 0; i < s_len; i++)
	{
		hash += (uint32_t)pow(prime, s_len - (i + 1) * ((char*)key)[i]);
		hash = hash % num_buckets;
	}
	return hash;
}*/