#include "head.h"
#include "log.h"
#include "graphics/vulkan_internal.h"
#include "texture.h"
#include "graphics/buffer.h"
#include <stdlib.h>
#include <stb_image.h>

// The different flavors of descriptor pools
struct DescriptorPool
{
	// How many uniform type descriptors are left in the pool
	uint32_t uniform_count;
	// How many sampler type descriptors are left in the pool
	uint32_t sampler_count;
	VkDescriptorPool pool;
};

struct DescriptorPool* descriptor_pools;
uint32_t descriptor_pool_count;

static BufferPoolArray ub_pools = {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 3 * 256 * 10, 0, NULL};

typedef struct
{
	Head head;
	// The size of one frame of the command buffer
	uint32_t size;
	uint32_t offsets[3];
	VkBuffer buffers[3];
	VkDeviceMemory memories[3];
} UniformBuffer;

// Attempts to find and return a descriptor pool with a minimum of the supplied descriptor types remaining
// If no qualifying pool was found, a new one is created
VkDescriptorPool descriptorpool_get(uint32_t uniform_count, uint32_t sampler_count)
{

	for (uint32_t i = 0; i < descriptor_pool_count; i++)
	{
		// Qualifying pool
		if (descriptor_pools[i].uniform_count >= uniform_count && descriptor_pools[i].sampler_count >= sampler_count)
		{
			// Remove thee requested amount of descriptors from the pool
			descriptor_pools[i].uniform_count -= uniform_count;
			descriptor_pools[i].sampler_count -= sampler_count;

			return descriptor_pools[i].pool;
		}
	}

	// No pool was found
	// Allocate a new one
	LOG_S("Creating new descriptor pool");

	descriptor_pool_count += 1;
	descriptor_pools = realloc(descriptor_pools, descriptor_pool_count * sizeof(struct DescriptorPool));

	// Reference to the new pool struct
	struct DescriptorPool* new_pool = &descriptor_pools[descriptor_pool_count - 1];

	// Allocate a larger block of memory than the minimum to make room for more similar allocations and reduce memory
	// allocations
	new_pool->uniform_count = uniform_count * 100;
	new_pool->sampler_count = sampler_count * 100;

	VkDescriptorPoolSize poolSizes[2] = {0};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = new_pool->uniform_count;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = new_pool->sampler_count;

	VkDescriptorPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = sizeof(poolSizes) / sizeof(*poolSizes);
	poolInfo.pPoolSizes = poolSizes;

	poolInfo.maxSets = new_pool->uniform_count + new_pool->sampler_count;

	VkResult result = vkCreateDescriptorPool(device, &poolInfo, NULL, &new_pool->pool);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create descriptor pool - code %d", result);
		return NULL;
	}
	return new_pool->pool;
}

int descriptorlayout_create(VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count,
							VkDescriptorSetLayout* dst_layout)
{
	VkDescriptorSetLayoutCreateInfo layoutInfo = {0};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = binding_count;
	layoutInfo.pBindings = bindings;

	VkResult result = vkCreateDescriptorSetLayout(device, &layoutInfo, NULL, dst_layout);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create descriptor set layout - code %d", result);
		return -1;
	}
	return 0;
}

int descriptorset_create(VkDescriptorSetLayout layout, VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count,
						 UniformBuffer** uniformbuffers, Texture** textures, VkDescriptorSet* dst_descriptors)
{
	// Fill the layouts for all swapchain image count
	VkDescriptorSetLayout* layouts = malloc(swapchain_image_count * sizeof(VkDescriptorSetLayout));
	for (size_t i = 0; i < swapchain_image_count; i++)
		layouts[i] = layout;

	// Find out how many of each type of descriptor type is required
	uint32_t uniform_count = 0;
	uint32_t sampler_count = 0;
	for (uint32_t i = 0; i < binding_count; i++)
	{
		if (bindings[i].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		{
			uniform_count += bindings[i].descriptorCount;
		}
		else if (bindings[i].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
		{
			sampler_count += bindings[i].descriptorCount;
		}
		else
		{
			LOG_W("Descriptor set creation: Unsupported descriptor type");
		}
	}

	// Request uniform and sampler types for each frame in flight
	uniform_count *= swapchain_image_count;
	sampler_count *= swapchain_image_count;

	VkDescriptorSetAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorpool_get(uniform_count, sampler_count);
	allocInfo.descriptorSetCount = swapchain_image_count;
	allocInfo.pSetLayouts = layouts;
	allocInfo.pNext = 0;

	vkAllocateDescriptorSets(device, &allocInfo, dst_descriptors);

	VkWriteDescriptorSet* descriptorWrites = malloc(binding_count * sizeof(VkWriteDescriptorSet));
	VkDescriptorBufferInfo* buffer_infos = malloc(uniform_count * sizeof(VkDescriptorBufferInfo));
	VkDescriptorImageInfo* image_infos = malloc(sampler_count * sizeof(VkDescriptorImageInfo));

	// Write descriptors, repeat for each frame in flight
	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		uint32_t buffer_it = 0;
		uint32_t sampler_it = 0;
		// Iterate and fill a descriptor info for each binding
		for (uint32_t j = 0; j < binding_count; j++)
		{
			// Uniform buffer
			if (bindings[j].descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
			{
				buffer_infos[buffer_it].buffer = uniformbuffers[buffer_it]->buffers[i];
				buffer_infos[buffer_it].offset = uniformbuffers[buffer_it]->offsets[i];
				buffer_infos[buffer_it].range = uniformbuffers[buffer_it]->size;

				descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[j].dstSet = dst_descriptors[i];
				descriptorWrites[j].dstBinding = bindings[j].binding;
				descriptorWrites[j].dstArrayElement = 0;
				descriptorWrites[j].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[j].descriptorCount = 1;
				descriptorWrites[j].pBufferInfo = &buffer_infos[buffer_it];
				descriptorWrites[j].pImageInfo = NULL;
				descriptorWrites[j].pTexelBufferView = NULL;
				descriptorWrites[j].pNext = NULL;
				buffer_it++;
			}

			// Sampler
			else if (bindings[j].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				image_infos[sampler_it].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				image_infos[sampler_it].imageView = texture_get_image_view(textures[sampler_it]);
				image_infos[sampler_it].sampler = texture_get_sampler(textures[sampler_it]);

				descriptorWrites[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[j].dstSet = dst_descriptors[i];
				descriptorWrites[j].dstBinding = bindings[j].binding;
				descriptorWrites[j].dstArrayElement = 0;
				descriptorWrites[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[j].descriptorCount = 1;
				descriptorWrites[j].pBufferInfo = NULL;
				descriptorWrites[j].pImageInfo = &image_infos[sampler_it];
				descriptorWrites[j].pTexelBufferView = NULL;
				descriptorWrites[j].pNext = NULL;
				sampler_it++;
			}
			else
			{
				LOG_W("Descriptor set writing: Unsupported descriptor type");
			}
		}
		vkUpdateDescriptorSets(device, binding_count, descriptorWrites, 0, NULL);
	}

	free(layouts);
	free(buffer_infos);
	free(image_infos);
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
