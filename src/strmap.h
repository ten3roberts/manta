#include <stdint.h>

typedef struct
{
	char* key;
	void* data;
	uint32_t data_size;
} strmap_item;

// Associative container mapping a string to data of varying size
typedef struct strmap strmap;

strmap* strmap_create();

// Will insert an entry in the map associated with key
// If key already exists, exisiting data will be replaced
void strmap_insert(strmap* map, const char* key, void* data, uint32_t data_size);

// Will return the item associated with key
void* strmap_find(strmap* map, const char* key);
void strmap_remove(strmap* map, const char* key);
void strmap_destroy(strmap* map);

// Returns the item at index
// Changes when inserting into array
// Use only for iterating when not modifying
// Returns NULL when out of bounds (safe)
strmap_item* strmap_index(strmap* map, uint32_t index);

// Returns the amount of items in the map
uint32_t strmap_count(strmap* map);