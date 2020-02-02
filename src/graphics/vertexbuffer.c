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
	Vertex vertices[3];
	vertices[0] = (Vertex){(vec2){0, -0.5f}, (vec3){1, 0, 0}};
	vertices[1] = (Vertex){(vec2){0.5, 0.5f}, (vec3){0, 1, 0}};
	vertices[2] = (Vertex){(vec2){-0.5, 0.5f}, (vec3){0, 0, 1}};
	return vb_create(vertices, 3);
}

VertexBuffer* vb_generate_square()
{
	Vertex vertices[3];
	vertices[0] = (Vertex){(vec2){-0.5, -0.5f}, (vec3){0, 0, 1}};
	vertices[1] = (Vertex){(vec2){0.5, 0.5f}, (vec3){1, 0, 1}};
	vertices[2] = (Vertex){(vec2){-0.5, 0.5f}, (vec3){0, 0, 1}};
	return vb_create(vertices, 3);
}
VertexBuffer* vb_create(Vertex* vertices, uint32_t vertex_count)
{
	size_t buffer_size = sizeof(Vertex) * vertex_count;
	VertexBuffer* vb = malloc(buffer_size);
	vb->vertex_count = vertex_count;
	vb->vertices = malloc(buffer_size);
	memcpy(vb->vertices, vertices, sizeof(*vb->vertices) * vb->vertex_count);

	// Create the buffer and memory

	buffer_create(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
				  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vb->buffer, &vb->memory);
	vb_copy_data(vb);

	return vb;
}

int buffer_create(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer* buffer,
				  VkDeviceMemory* buffer_memory)
{
	// Create the vertex buffer
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufferInfo.flags = 0;

	VkResult result = vkCreateBuffer(device, &bufferInfo, NULL, buffer);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create vertex buffer");
		return -1;
	}
	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

	result = vkAllocateMemory(device, &allocInfo, NULL, buffer_memory);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to allocate vertex buffer memory");
		return -2;
	}

	// Associate the memory with the buffer
	vkBindBufferMemory(device, *buffer, *buffer_memory, 0);
	return 0;
}

void buffer_copy(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = command_pool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {0};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);
	vkEndCommandBuffer(commandBuffer);

	// Execute command buffer
	VkSubmitInfo submitInfo = {0};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphics_queue);
	vkFreeCommandBuffers(device, command_pool, 1, &commandBuffer);
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
				  &staging_buffer_memory);
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
	buffer_copy(staging_buffer, vb->buffer, buffer_size);
	vkDestroyBuffer(device, staging_buffer, NULL);
	vkFreeMemory(device, staging_buffer_memory, NULL);
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