#include <stdint.h>
#include "graphics/commandbuffer.h"

#define GLFW_INCLUDE_VULKAN
#include "GLFW/glfw3.h"

typedef struct
{
	uint32_t index_count;
	uint32_t* indices;
	VkBuffer buffer;
	VkDeviceMemory memory;
} IndexBuffer;

// Creates and allocates an index buffer
IndexBuffer* ib_create(uint32_t* indices, uint32_t index_count);
void ib_bind(IndexBuffer* ib, CommandBuffer* commandbuffer);
void ib_destroy(IndexBuffer* ib);