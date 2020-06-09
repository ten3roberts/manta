
#ifndef HANDLE_H
#define HANDLE_H
// The maximum index of a handle in terms of bits
#define MAX_HANDLE_INDEX_BITS	 20
#define MAX_HANDLE_PATTERN_BITS	 (32 - MAX_HANDLE_INDEX_BITS)
#define PUN_HANDLE(handle, type) *(type*)&handle

struct GenericHandle
{
	unsigned int index : MAX_HANDLE_INDEX_BITS;
	unsigned int pattern : (MAX_HANDLE_PATTERN_BITS);
};

// Defines a handle for a type
// All handles, regardless of types are equal and can be type punned
#define DEFINE_HANDLE(type) typedef struct GenericHandle type;

// Returns an invalid handle of the specified type
#define INVALID(handle)           \
	(handle)                      \
	{                             \
		.index = -1, .pattern = 0 \
	}

#define HANDLE_COMPARE(a, b) (a.index == b.index && a.pattern == b.pattern)
// Returns true if a handle is valid
#define HANDLE_VALID(handle) (handle.index != -1)

#endif