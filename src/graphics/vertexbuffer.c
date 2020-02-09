#include "vertexbuffer.h"
#include "vulkan_members.h"
#include "buffer.h"
#include "log.h"

static BufferPoolArray vb_pools = {VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 1024, 0, NULL};

VertexBuffer* vb_generate_triangle()
{
	Vertex vertices[4];
	vertices[0] = (Vertex){(vec2){-0.5, -0.5f}, (vec3){0, 0, 1}, (vec2){1, 0}};
	vertices[1] = (Vertex){(vec2){0.5, -0.5f}, (vec3){1, 0, 1}, (vec2){0, 0}};
	vertices[2] = (Vertex){(vec2){0.5, 0.5f}, (vec3){1, 0, 0}, (vec2){0, 1}};
	vertices[3] = (Vertex){(vec2){-0.5, 0.5f}, (vec3){0, 0, 1}, (vec2){1, 1}};
	return vb_create(vertices, 4);
}

VertexBuffer* vb_generate_square()
{
	Vertex vertices[4];
	vertices[0] = (Vertex){(vec2){-0.5, -0.5f}, (vec3){0, 0, 1}, (vec2){1, 0}};
	vertices[1] = (Vertex){(vec2){0.5, -0.5f}, (vec3){0, 1, 0}, (vec2){0, 0}};
	vertices[2] = (Vertex){(vec2){0.5, 0.5f}, (vec3){1, 0, 0}, (vec2){0, 1}};
	vertices[3] = (Vertex){(vec2){-0.5, 0.5f}, (vec3){0, 1, 1}, (vec2){1, 1}};
	return vb_create(vertices, 4);
}
VertexBuffer* vb_create(Vertex* vertices, uint32_t vertex_count)
{
	VertexBuffer* vb = malloc(sizeof(VertexBuffer));

	size_t buffer_size = sizeof(*vb->vertices) * vertex_count;

	vb->vertex_count = vertex_count;
	vb->vertices = malloc(buffer_size);
	memcpy(vb->vertices, vertices, sizeof(*vb->vertices) * vb->vertex_count);

	// Create the buffer and memory

	buffer_pool_array_get(&vb_pools, buffer_size, &vb->buffer, &vb->memory, &vb->offset);

	vb_copy_data(vb);

	return vb;
}

// Copies the CPU side data to the GPU
void vb_copy_data(VertexBuffer* vb)
{
	// Temporary staging buffer
	size_t buffer_size = sizeof(*vb->vertices) * vb->vertex_count;
	VkBuffer staging_buffer;
	VkDeviceMemory staging_buffer_memory;
	buffer_create(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
				  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer,
				  &staging_buffer_memory, NULL, NULL);
	// Copy the vertex data to the buffer
	void* data = NULL;
	vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &data);
	if (data == NULL)
	{
		LOG_E("Failed to map vertex buffer memory");
		return;
	}
	memcpy(data, vb->vertices, buffer_size);
	vkUnmapMemory(device, staging_buffer_memory);

	// Copy the data from the staging buffer to the local vertex buffer
	buffer_copy(staging_buffer, vb->buffer, buffer_size, 0, vb->offset);
	vkDestroyBuffer(device, staging_buffer, NULL);
	vkFreeMemory(device, staging_buffer_memory, NULL);
}

void vb_bind(VertexBuffer* vb, VkCommandBuffer command_buffer)
{
	VkBuffer vertex_buffers[] = {vb->buffer};
	VkDeviceSize offsets[] = {vb->offset};
	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
}

void vb_destroy(VertexBuffer* vb)
{
	LOG_S("Destroying vertex buffer");
	//vkDestroyBuffer(device, vb->buffer, NULL);
	//vkFreeMemory(device, vb->memory, NULL);
	vb->vertex_count = 0;
	free(vb->vertices);
	free(vb);
}

void vb_pools_destroy()
{
	buffer_pool_array_destroy(&vb_pools);
}

VertexInputDescription vertex_get_description()
{
	VertexInputDescription description;
	description.binding_description.binding = 0;
	description.binding_description.stride = sizeof(Vertex);
	description.binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	description.attribute_count = 3;
	// Position
	description.attributes[0].binding = 0;
	description.attributes[0].location = 0;
	description.attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
	description.attributes[0].offset = offsetof(Vertex, position);

	// Color
	description.attributes[1].binding = 0;
	description.attributes[1].location = 1;
	description.attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	description.attributes[1].offset = offsetof(Vertex, color);
	// UV

	description.attributes[2].binding = 0;
	description.attributes[2].location = 2;
	description.attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
	description.attributes[2].offset = offsetof(Vertex, uv);

	return description;
}