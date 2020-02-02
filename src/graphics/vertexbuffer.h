#include "math/vec.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdint.h>

typedef struct
{
	vec2 position;
	vec3 color;
} Vertex;

typedef struct
{
	VkVertexInputBindingDescription binding_description;
	uint32_t attribute_count;
	VkVertexInputAttributeDescription attributes[2];
} VertexInputDescription;

typedef struct
{
	uint32_t vertex_count;
	Vertex* vertices;
	VkBuffer buffer;
	VkDeviceMemory memory;

} VertexBuffer;

VertexBuffer* vb_create(Vertex* vertices, uint32_t vertex_count);
VertexBuffer* vb_generate_triangle();
VertexBuffer* vb_generate_square();

// Creates and allocates memory for a buffer
int buffer_create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer,
				  VkDeviceMemory* buffer_memory);

// Can be used to copy data to staging buffers
void buffer_copy(VkBuffer src, VkBuffer dst, VkDeviceSize size);
// Copies the CPU side data in the vertex buffer to the GPU
void vb_copy_data(VertexBuffer* vb);
void vb_bind(VertexBuffer* vb, VkCommandBuffer command_buffer);

void vb_destroy(VertexBuffer* vb);

VertexInputDescription vertex_get_description();