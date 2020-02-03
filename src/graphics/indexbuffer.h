#include <stdint.h>
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
IndexBuffer* ib_create();
void ib_bind(IndexBuffer* ib, VkCommandBuffer command_buffer);
void ib_destroy(IndexBuffer* ib);