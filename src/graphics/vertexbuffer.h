#ifndef VERTEXBUFFER_H
#define VERTEXBUFFER_H

#include "math/vec.h"
#include "graphics/commandbuffer.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdint.h>

typedef struct
{
	vec3 position;
	vec2 uv;
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
	// The size of the vertex buffer data
	uint32_t size;
	uint32_t offset;
	VkBuffer buffer;
	VkDeviceMemory memory;

} VertexBuffer;

VertexBuffer* vb_create(Vertex* vertices, uint32_t vertex_count);
VertexBuffer* vb_generate_triangle();
VertexBuffer* vb_generate_square();

// Copies the CPU side data in the vertex buffer to the GPU
void vb_copy_data(VertexBuffer* vb);
void vb_bind(VertexBuffer* vb, CommandBuffer* commandbuffer);

void vb_destroy(VertexBuffer* vb);

// Destroys all VertexBuffer pools in the end of the programs
// The pools were first created implicitly when a UniformBuffer was created
void vb_pools_destroy();

VertexInputDescription vertex_get_description();
#endif
