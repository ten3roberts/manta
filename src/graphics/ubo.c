#include "head.h"
#include "log.h"
#include "graphics/vulkan_internal.h"
#include "graphics/vertexbuffer.h"

typedef struct
{
	Head head;
	uint32_t size;
	VkBuffer* buffers;
	VkDeviceMemory* memories;
} UniformBuffer;

UniformBuffer* ub_create(uint32_t size)
{
	LOG_S("Creating uniform buffer");
	UniformBuffer* ub = malloc(sizeof(UniformBuffer));
	ub->head.type = RT_UNIFORMBUFFER;
	ub->head.id = 0;
	ub->size = size;

	ub->buffers = malloc(swapchain_image_count * sizeof(VkBuffer));
	ub->memories = malloc(swapchain_image_count * sizeof(VkDeviceMemory));

	for (size_t i = 0; i < swapchain_image_count; i++) {
		buffer_create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ub->buffers[i], &ub->memories[i]);
	}
	return ub;
}

void ub_update(UniformBuffer* ub, void* data, uint32_t size, uint32_t offset, uint32_t i)
{
	if (size == (uint32_t)-1)
	{
		size = ub->size;
	}
	else if (size > ub->size)
	{
		size = ub->size;
		LOG_W("Request to update uniform buffer of %d bytes with %d bytes of data", ub->size, size);
	}
	void* data_map = NULL;
	vkMapMemory(device, ub->memories[i], offset, size, 0, &data_map);
	if (data_map == NULL)
	{
		LOG_E("Failed to map memory for uniform buffer");
		return;
	}
	memcpy(data_map, data, size);
	vkUnmapMemory(device, ub->memories[i]);
}

void ub_destroy(UniformBuffer* ub)
{
	LOG_S("Destroying uniform buffer");
	for (size_t i = 0; i < swapchain_image_count; i++)
	{
		vkDestroyBuffer(device, ub->buffers[i], NULL);
		vkFreeMemory(device, ub->memories[i], NULL);
	}
	free(ub->buffers);
	free(ub->memories);
	free(ub);
}

void** ub_get_buffer(UniformBuffer* ub)
{
	return (void*)ub->buffers;
}