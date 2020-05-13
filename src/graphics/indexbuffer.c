#include "indexbuffer.h"
#include "graphics/vertexbuffer.h"
#include "vulkan_members.h"
#include "buffer.h"
#include "log.h"
#include "magpie.h"

IndexBuffer* ib_create(uint32_t* indices, uint32_t index_count)
{
	IndexBuffer* ib = malloc(sizeof(IndexBuffer));
	ib->index_count = index_count;
	size_t buffer_size = sizeof(*ib->indices) * ib->index_count;

	ib->indices = malloc(buffer_size);
	memcpy(ib->indices, indices, buffer_size);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	buffer_create(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				  &stagingBuffer, &stagingBufferMemory, NULL, NULL);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, buffer_size, 0, &data);
	memcpy(data, ib->indices, buffer_size);
	vkUnmapMemory(device, stagingBufferMemory);

	buffer_create(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &ib->buffer,
				  &ib->memory, NULL, NULL);

	buffer_copy(stagingBuffer, ib->buffer, buffer_size, 0, 0);

	vkDestroyBuffer(device, stagingBuffer, NULL);
	vkFreeMemory(device, stagingBufferMemory, NULL);
	return ib;
}

void ib_bind(IndexBuffer* ib, CommandBuffer* commandbuffer)
{
	vkCmdBindIndexBuffer(commandbuffer->buffer, ib->buffer, 0, VK_INDEX_TYPE_UINT32);
}

void ib_destroy(IndexBuffer* ib)
{
	LOG_S("Destroying index buffer");
	vkDestroyBuffer(device, ib->buffer, NULL);
	vkFreeMemory(device, ib->memory, NULL);
	ib->index_count = 0;
	free(ib->indices);
	free(ib);
}