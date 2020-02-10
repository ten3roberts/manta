#include "head.h"
#include "log.h"
#include "graphics/vulkan_internal.h"
#include "texture.h"
#include "graphics/buffer.h"
#include <stdlib.h>
#include <stb_image.h>
// Tmp

typedef struct Texture
{
	int width;
	int height;
	int channels;
	stbi_uc* pixels;
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkSampler sampler;
} Texture;

VkDescriptorSetLayout* descriptor_layouts = NULL;
uint32_t* layout_binding_map = NULL;
VkDescriptorType* layout_type_map = NULL;
uint32_t descriptor_layout_count = 0;

VkDescriptorSetLayout* ub_get_layouts()
{
	return descriptor_layouts;
}
uint32_t ub_get_layout_count()
{
	return descriptor_layout_count;
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

DescriptorPool* sampler_descriptor_pools = NULL;
uint32_t sampler_descriptor_pool_count = 0;

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

int ub_create_descriptor_set_layout(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stage)
{
	descriptor_layouts = realloc(descriptor_layouts, ++descriptor_layout_count * sizeof(VkDescriptorSetLayout));
	layout_binding_map = realloc(layout_binding_map, descriptor_layout_count * sizeof(uint32_t));
	layout_type_map = realloc(layout_type_map, descriptor_layout_count * sizeof(VkDescriptorType));
	layout_type_map[descriptor_layout_count - 1] = type;
	// Update the map that corresponds the binding with the layout
	layout_binding_map[descriptor_layout_count - 1] = binding;
	VkDescriptorSetLayoutBinding uboLayoutBinding = {0};
	uboLayoutBinding.binding = binding;
	uboLayoutBinding.descriptorType = type;
	uboLayoutBinding.descriptorCount = 1;

	uboLayoutBinding.stageFlags = stage;

	uboLayoutBinding.pImmutableSamplers = NULL; // Optional

	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = 1;
	layoutInfo.pBindings = &uboLayoutBinding;

	VkResult result =
		vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, &descriptor_layouts[descriptor_layout_count - 1]);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create descriptor set layout - code %d", result);
		return -1;
	}
	return 0;
}

int ub_descriptor_pool_create(VkDescriptorType type)
{
	// Resize array
	LOG_S("Creating new descriptor pool");

	DescriptorPool* descriptor_pool = NULL;
	if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
	{
		descriptor_pools = realloc(descriptor_pools, ++descriptor_pool_count * sizeof(DescriptorPool));
		descriptor_pool = &descriptor_pools[descriptor_pool_count - 1];
	}
	else if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
	{
		sampler_descriptor_pools =
			realloc(sampler_descriptor_pools, ++sampler_descriptor_pool_count * sizeof(DescriptorPool));
		descriptor_pool = &sampler_descriptor_pools[sampler_descriptor_pool_count - 1];
	}
	descriptor_pool->filled_count = 0;
	descriptor_pool->alloc_count = swapchain_image_count * 100;

	VkDescriptorPoolSize poolSize = {0};
	poolSize.type = type;
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

int ub_create_descriptor_sets(VkDescriptorSet* dst_descriptors, uint32_t binding, VkBuffer* buffers, uint32_t* offsets,
							  uint32_t size, VkImageView image_view, VkSampler sampler)
{
	VkDescriptorSetLayout* layouts = malloc(swapchain_image_count * sizeof(VkDescriptorSetLayout));
	VkDescriptorSetLayout layout = NULL;
	VkDescriptorType type = 0;
	for (uint32_t i = 0; i < descriptor_layout_count; i++)
	{
		if (layout_binding_map[i] == binding)
		{
			layout = descriptor_layouts[i];
			type = layout_type_map[i];
			break;
		}
	}
	if (layout == NULL)
	{
		LOG_E("Attempting to create descriptor set from binding %d which has not been created", binding);
	}
	for (size_t i = 0; i < swapchain_image_count; i++)
		layouts[i] = layout;

	VkDescriptorSetAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	if (descriptor_pool_count == 0)
	{
		ub_descriptor_pool_create(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	}

	if (sampler_descriptor_pool_count == 0)
	{
		ub_descriptor_pool_create(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	}
	// Iterate and find a pool that is not full
	if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
	{
		for (uint32_t i = 0; i < sampler_descriptor_pool_count; i++)
		{
			if (sampler_descriptor_pools[i].filled_count + 3 >= sampler_descriptor_pools[i].alloc_count)
			{
				// At last pool
				if (i == sampler_descriptor_pool_count - 1)
				{
					ub_descriptor_pool_create(type);
				}
				continue;
			}
			// Found a good pool
			allocInfo.descriptorPool = sampler_descriptor_pools[i].pool;
			descriptor_pools[i].filled_count += 3;
		}
	}
	if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
	{

		for (uint32_t i = 0; i < descriptor_pool_count; i++)
		{
			if (descriptor_pools[i].filled_count + 3 >= descriptor_pools[i].alloc_count)
			{
				// At last pool
				if (i == descriptor_pool_count - 1)
				{
					ub_descriptor_pool_create(type);
				}
				continue;
			}
			// Found a good pool
			allocInfo.descriptorPool = descriptor_pools[i].pool;
			descriptor_pools[i].filled_count += 3;
		}
	}
	allocInfo.descriptorSetCount = swapchain_image_count;
	allocInfo.pSetLayouts = layouts;

	vkAllocateDescriptorSets(device, &allocInfo, dst_descriptors);
	for (size_t i = 0; i < swapchain_image_count; i++)
	{

		VkWriteDescriptorSet descriptorWrite = {0};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = dst_descriptors[i];
		descriptorWrite.dstBinding = binding;
		descriptorWrite.dstArrayElement = 0;

		if (type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			VkDescriptorBufferInfo bufferInfo = {0};
			bufferInfo.buffer = buffers[i];
			bufferInfo.offset = offsets[i];
			bufferInfo.range = size;

			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1;

			descriptorWrite.pBufferInfo = &bufferInfo;
			descriptorWrite.pImageInfo = NULL;		 // Optional
			descriptorWrite.pTexelBufferView = NULL; // Optional
		}
		else if (type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			VkDescriptorImageInfo imageInfo = {0};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = image_view;
			imageInfo.sampler = sampler;

			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.descriptorCount = 1;

			descriptorWrite.pBufferInfo = NULL;
			descriptorWrite.pImageInfo = &imageInfo; // Optional
			descriptorWrite.pTexelBufferView = NULL; // Optional
		}
		vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, NULL);
	}

	free(layouts);
	return 0;
}

UniformBuffer* ub_create(uint32_t size, uint32_t binding)
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
	ub_create_descriptor_sets(ub->descriptor_sets, binding, ub->buffers, ub->offsets, ub->size, NULL, NULL);
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

	/*for (uint32_t i = 0; i < descriptor_layout_count; i++)
	{
		vkDestroyDescriptorSetLayout(device, descriptor_layouts[i], NULL);
	}
	free(descriptor_layouts);
	free(layout_binding_map);
	descriptor_layout_count = 0;*/
}
