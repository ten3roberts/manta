#include <stdint.h>

typedef struct
{
	char* key;
	void* data;
} strmap_item;

// Associative container mapping a string to data of varying size
typedef struct strmap strmap;

strmap* strmap_create();

// Will insert an entry in the map associated with key
// If key already exists, exisiting data will be replaced
void strmap_insert(strmap* map, const char* key, void* data, uint32_t size);
void* strmap_find(strmap* map, const char* key);
void strmap_remove(strmap* map, const char* key);
void strmap_destroy(strmap* map);