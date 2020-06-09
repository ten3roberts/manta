
#ifndef HANDLE_H
#define HANDLE_H
// The maximum index of a handle in terms of bits
#define MAX_HANDLE_INDEX_BITS	20
#define MAX_HANDLE_PATTERN_BITS (32 - MAX_HANDLE_INDEX_BITS)
#define PUN_HANDLE(handle, type) *(type*)&handle
// Defines a handle for a type
// All handles, regardless of types are equal and can be type punned
#define DEFINE_HANDLE(type)                               \
	typedef struct type                                   \
	{                                                     \
		unsigned int index : MAX_HANDLE_INDEX_BITS;       \
		unsigned int pattern : (MAX_HANDLE_PATTERN_BITS); \
	} type;

#endif