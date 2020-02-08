#include "head.h"
#include "log.h"
#include "graphics/vulkan_internal.h"
#include "graphics/buffer.h"
#include <stdlib.h>

VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;

VkDescriptorSetLayout* ub_get_layouts()
{
	return &descriptorSetLayout;
}

typedef struct
{
	// Describes the currently used count of allocated descriptors
	uint32_t filled_count;
	// Describes the total count that was allocated
	uint32_t alloc_count;
	VkDescriptorPool pool;
} DescriptorPool;

DescriptorPool* descriptor_pools = NULL;
uint32_t descriptor_pool_count = 0;

static BufferPoolArray ub_pools = {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 3 * 256 * 10, 0, NULL};

typedef struct
{
	Head head;
	// The size of one frame of the command buffer
	uint32_t size;
	uint32_t offsets[3];
	VkBuffer buffers[3];
	VkDeviceMemory memories[3];
	VkDescriptorSet descriptor_sets[3];
} UniformBuffer;

int ub_create_descriptor_set_layout(uint32_t binding)
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
	uboLayoutBinding.binding = binding;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;

	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	uboLayoutBinding.pImmutableSamplers = NULL; // Optional

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptorSetLayout);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create descriptor set layout - code %d", result);
		return -1;
	}
	return 0;
}

int ub_descriptor_pool_create()
{
	// Resize array
	LOG_S("Creating new descriptor pool");
	descriptor_pools = realloc(descriptor_pools, ++descriptor_pool_count * sizeof(DescriptorPool));
	DescriptorPool* descriptor_pool = &descriptor_pools[descriptor_pool_count - 1];
	descriptor_pool->filled_count = 0;
	descriptor_pool->alloc_count = swapchain_image_count * 100;
	VkDescriptorPoolSize poolSize = {0};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = descriptor_pool->alloc_count;

	VkDescriptorPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;

	poolInfo.maxSets = descriptor_pool->alloc_count;

	VkResult result = vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptor_pool->pool);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create descriptor pool - code %d", result);
		return -1;
	}
	return 0;
}

int ub_create_descriptor_sets(UniformBuffer* ub)
{
	VkDescriptorSetLayout* layouts = malloc(swapchain_image_count * sizeof(VkDescriptorSetLayout));
	for (size_t i = 0; i < swapchain_image_count; i++)
		layouts[i] = descriptorSetLayout;

	VkDescriptorSetAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;

	if (descriptor_pool_count == 0)
	{
		ub_descriptor_pool_create();
	}
	// Iterate and find a pool that is not full
	for (uint32_t i = 0; i < descriptor_pool_count; i++)
	{
		if (descriptor_pools[i].filled_count + 3 >= descriptor_pools[i].alloc_count)
		{
			// At last pool
			if (i == descriptor_pool_count - 1)
			{
				ub_descriptor_pool_create();
			}
			continue;
		}
		// Found a good pool
		allocInfo.descriptorPool = descriptor_pools[i].pool;
		descriptor_pools[i].filled_count += 3;
	}

	allocInfo.descriptorSetCount = swapchain_image_count;
	allocInfo.pSetLayouts = layouts;
	// descriptor_sets = malloc(swapchain_image_count * sizeof(VkDescriptorSet));
	vkAllocateDescriptorSets(device, &allocInfo, ub->descriptor_sets);
	for (size_t i = 0; i < swapchain_image_count; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {0};
		bufferInfo.buffer = ub->buffers[i];
		bufferInfo.offset = ub->offsets[i];
		bufferInfo.range = ub->size;

		VkWriteDescriptorSet descriptorWrite = {0};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = ub->descriptor_sets[i];
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;

		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;

		descriptorWrite.pBufferInfo = &bufferInfo;
		descriptorWrite.pImageInfo = NULL;		 // Optional
		descriptorWrite.pTexelBufferView = NULL; // Optional
		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);
	}

	free(layouts);
	return 0;
}

UniformBuffer* ub_create(uint32_t size)
{
	LOG_S("Creating uniform buffer");
	UniformBuffer* ub = malloc(sizeof(UniformBuffer));
	ub->head.type = RT_UNIFORMBUFFER;
	ub->head.id = 0;
	ub->size = size;

	// Find a free pool
	for (int i = 0; i < swapchain_image_count; i++)
	{
		buffer_pool_array_get(&ub_pools, size, &ub->buffers[i], &ub->memories[i], &ub->offsets[i]);
	}

	// Create descriptor sets
	ub_create_descriptor_sets(ub);
	return ub;
}

void ub_update(UniformBuffer* ub, void* data, uint32_t i)
{
	void* data_map = NULL;
	vkMapMemory(device, ub->memories[i], ub->offsets[i], ub->size, 0, &data_map);
	if (data_map == NULL)
	{
		LOG_E("Failed to map memory for uniform buffer");
		return;
	}
	memcpy(data_map, data, ub->size);
	vkUnmapMemory(device, ub->memories[i]);
}

void ub_destroy(UniformBuffer* ub)
{
	LOG_S("Destroying uniform buffer");
	for (size_t i = 0; i < swapchain_image_count; i++)
	{
		// vkDestroyBuffer(device, ub->buffers[i], NULL);
		// vkFreeMemory(device, ub->memories[i], NULL);
	}
	free(ub);
}

void ub_bind(UniformBuffer* ub, VkCommandBuffer command_buffer, int i)
{
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1,
							&ub->descriptor_sets[i], 0, NULL);
}

void ub_pools_destroy()
{
	buffer_pool_array_destroy(&ub_pools);
	for (uint32_t i = 0; i < descriptor_pool_count; i++)
	{
		vkDestroyDescriptorPool(device, descriptor_pools[i].pool, NULL);
	}
	free(descriptor_pools);
	descriptor_pool_count = 0;
}
