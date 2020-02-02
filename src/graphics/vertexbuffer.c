#include "vertexbuffer.h"
#include "vulkan_members.h"
#include "log.h"

uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physical_device, &memProperties);
	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (type_filter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}
	LOG_E("Could not find a suitable memory type");
	return 0;
}

VertexBuffer* vb_generate_triangle()
{
	VertexBuffer* vb = malloc(sizeof(VertexBuffer));
	vb->vertex_count = 3;
	vb->vertices = malloc(sizeof(Vertex) * 3);
	vb->vertices[0] = (Vertex){(vec2){0, -0.5f}, (vec3){1, 0, 0}};
	vb->vertices[1] = (Vertex){(vec2){0.5, 0.5f}, (vec3){0, 1, 0}};
	vb->vertices[2] = (Vertex){(vec2){-0.5, 0.5f}, (vec3){1, 1, 1}};

	// Create the vertex buffer
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(*vb->vertices) * vb->vertex_count;

	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.flags = 0;

	VkResult result = vkCreateBuffer(device, &bufferInfo, NULL, &vb->buffer);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create vertex buffer");
		free(vb);
		return NULL;
	}
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, vb->buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = find_memory_type(
		memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	result = vkAllocateMemory(device, &allocInfo, NULL, &vb->memory);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to allocate vertex buffer memory");
		free(vb);
		return NULL;
	}

	// Associate the memory with the buffer
	vkBindBufferMemory(device, vb->buffer, vb->memory, 0);
	vb_copy_data(vb);
	return vb;
}

// Copies the CPU side data to the GPU
void vb_copy_data(VertexBuffer* vb)
{
	size_t size = sizeof(*vb->vertices) * vb->vertex_count;
	// Copy the vertex data to the buffer
	void* data = NULL;
	vkMapMemory(device, vb->memory, 0, size, 0, &data);
	if(data == NULL)
	{
		LOG_E("Failed to map vertex buffer memory");
		return;
	}
	memcpy(data, vb->vertices, size);
	vkUnmapMemory(device, vb->memory);
}

void vb_bind(VertexBuffer* vb, VkCommandBuffer command_buffer)
{
	VkBuffer vertex_buffers[] = {vb->buffer};
	VkDeviceSize offsets[] = {0};
	vkCmdBindVertexBuffers(command_buffer, 0, 1, vertex_buffers, offsets);
}

void vb_destroy(VertexBuffer* vb)
{
	LOG_S("Destroying vertex buffer");
	vkDestroyBuffer(device, vb->buffer, NULL);
	vkFreeMemory(device, vb->memory, NULL);
	vb->vertex_count = 0;
	free(vb->vertices);
	free(vb);
}

VertexInputDescription vertex_get_description()
{
	VertexInputDescription description;
	description.binding_description.binding = 0;
	description.binding_description.stride = sizeof(Vertex);
	description.binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	description.attribute_count = 2;
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

	return description;
}