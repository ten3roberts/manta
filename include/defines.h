#ifndef DEFINES_H
#define DEFINES_H

// The maximum threads the renderer can use when building command buffers
#define RENDERER_MAX_THREADS 4
// The limit of entities in a tree before it splits
#define RENDER_TREE_LIM 512

#define MAX_FRAMES_IN_FLIGHT 3

// The maximum index of a handle in terms of bits
#define MAX_HANDLE_INDEX_BITS	20
#define MAX_HANDLE_PATTERN_BITS (32 - MAX_HANDLE_INDEX_BITS)
#define HANDLE_INDEX_FREE		-1
#define DEFINE_HANDLE(type)                              \
	struct type##__                                      \
	{                                                    \
		unsigned int index : MAX_HANDLE_INDEX_BITS;      \
		unsigned int pattern : (MAX_HANDLE_PATTERN_BITS); \
	};                                                   \
	typedef struct type##__ type;
#endif