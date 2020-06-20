#include "graphics/uniforms.h"
#include "graphics/vulkan_internal.h"
#include "graphics/texture.h"
#include "graphics/buffer.h"
#include "graphics/renderer.h"
#include "log.h"
#include "magpie.h"
#include <stb_image.h>
#include "mempool.h"
#include "defines.h"
#include <assert.h>

// The different flavors of descriptor pools
// If pool is VK_NULL_HANDLE, it is a hole from a destroyed pool
struct DescriptorPool
{
	// How many uniform type descriptors are left in the pool
	uint32_t uniform_count;
	// How many sampler type descriptors are left in the pool
	uint32_t sampler_count;
	// Describes how many descriptors currently use the pool
	uint32_t alloc_count;
	VkDescriptorPool vkpool;
};

struct DescriptorPool* descriptor_pools = NULL;
// How many active pools there are
uint32_t descriptor_pool_count = 0;
// How large the list of pools is, including holes
uint32_t descriptor_pool_size = 0;

static BufferPool ub_pool[RENDERER_MAX_THREADS] = {0};

struct UniformBuffer
{
	// The size of one frame of the command buffer
	uint32_t size;
	uint32_t offsets[3];
	VkBuffer buffers[3];
	VkDeviceMemory memories[3];
	uint8_t thread_idx;
};

// Attempts to find and return a descriptor pool with a minimum of the supplied descriptor types remaining
// If no qualifying pool was found, a new one is created
// Returns the index of the pool
// Does not return the pointer since it is liable to change with realloc when new pools are added
uint32_t descriptorpool_get(uint32_t uniform_count, uint32_t sampler_count)
{
	for (uint32_t i = 0; i < descriptor_pool_size; i++)
	{
		// Hole from destroyed pool
		if (descriptor_pools[i].vkpool == VK_NULL_HANDLE)
			continue;
		// Qualifying pool
		if (descriptor_pools[i].uniform_count >= uniform_count && descriptor_pools[i].sampler_count >= sampler_count)
		{
			// Remove the requested amount of descriptors from the pool
			descriptor_pools[i].uniform_count -= uniform_count;
			descriptor_pools[i].sampler_count -= sampler_count;
			++descriptor_pools[i].alloc_count;

			return i;
		}
	}

	// No pool was found
	// Allocate a new one
	DescriptorPool* new_pool = NULL;
	uint32_t index = 0;
	// Search for a free hole in the list
	for (uint32_t i = 0; i < descriptor_pool_size; i++)
	{
		// Reuse space
		if (descriptor_pools[i].vkpool == VK_NULL_HANDLE)
		{
			LOG("Reusing free slot for descriptor pool");
			new_pool = &descriptor_pools[i];
			index = i;
		}
	}

	// No free space was found
	if (new_pool == NULL)
	{
		++descriptor_pool_count;
		++descriptor_pool_size;
		descriptor_pools = realloc(descriptor_pools, descriptor_pool_size * sizeof(struct DescriptorPool));
		index = descriptor_pool_size - 1;
		new_pool = &descriptor_pools[index];
	}

	// Allocate a larger block of memory than the minimum to make room for more similar allocations and reduce memory
	// allocations
	new_pool->uniform_count = uniform_count * 256;
	new_pool->sampler_count = sampler_count * 256;
	// Make one allocation for the pool before returning
	new_pool->alloc_count = 0;

	LOG_S("Creating new descriptor pool with capacity for %d uniform types and %d sampler types", new_pool->uniform_count, new_pool->sampler_count);

	VkDescriptorPoolSize poolSizes[2] = {0};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = new_pool->uniform_count;
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[1].descriptorCount = new_pool->sampler_count;

	VkDescriptorPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = uniform_count && sampler_count ? 2 : 1;
	poolInfo.pPoolSizes = uniform_count ? poolSizes : sampler_count ? &poolSizes[1] : NULL;

	poolInfo.maxSets = new_pool->uniform_count + new_pool->sampler_count;
	poolInfo.flags = 0; //VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	VkResult result = vkCreateDescriptorPool(device, &poolInfo, NULL, &new_pool->vkpool);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create descriptor pool - code %d", result);
		return (uint32_t)-1;
	}

	// 'allocate'
	new_pool->uniform_count -= uniform_count;
	new_pool->sampler_count -= sampler_count;
	new_pool->alloc_count = 1;
	return index;
}

void descriptorpool_destroy(DescriptorPool* pool)
{
	vkDeviceWaitIdle(device);
	LOG_S("Destroying descriptor pool");

	if (pool->vkpool == VK_NULL_HANDLE)
	{
		LOG_E("Attempting to destroy already destroyed pool");
		return;
	}

	vkDestroyDescriptorPool(device, pool->vkpool, NULL);
	--descriptor_pool_count;
	pool->vkpool = VK_NULL_HANDLE;

	// Last pool is being destroyed
	if (descriptor_pool_count == 0)
	{
		free(descriptor_pools);
		descriptor_pools = NULL;
		descriptor_pool_size = 0;
	}
}

int descriptorlayout_create(VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count, VkDescriptorSetLayout* dst_layout)
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

DescriptorPack* descriptorpack_create(VkDescriptorSetLayout layout, VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count)
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

	DescriptorPack* pack = malloc(sizeof(DescriptorPack));

	pack->pool_index = descriptorpool_get(uniform_count, sampler_count);
	if (pack->pool_index == (uint32_t)-1)
	{
		LOG_E("Failed to get descriptor pool");
		return NULL;
	}
	pack->uniform_count = uniform_count;
	pack->sampler_count = sampler_count;
	pack->count = swapchain_image_count;

	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptor_pools[pack->pool_index].vkpool;
	allocInfo.descriptorSetCount = pack->count;
	allocInfo.pSetLayouts = layouts;
	allocInfo.pNext = 0;

	vkAllocateDescriptorSets(device, &allocInfo, pack->sets);
	free(layouts);
	return pack;
}

void descriptorpack_write(DescriptorPack* pack, VkDescriptorSetLayoutBinding* bindings, uint32_t binding_count, UniformBuffer** uniformbuffers, Texture* textures, Sampler* samplers)
{
	// vkDeviceWaitIdle(device);
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

	VkWriteDescriptorSet* descriptor_writes = malloc(binding_count * sizeof(VkWriteDescriptorSet));
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

				descriptor_writes[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptor_writes[j].dstSet = pack->sets[i];
				descriptor_writes[j].dstBinding = bindings[j].binding;
				descriptor_writes[j].dstArrayElement = 0;
				descriptor_writes[j].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptor_writes[j].descriptorCount = 1;
				descriptor_writes[j].pBufferInfo = &buffer_infos[buffer_it];
				descriptor_writes[j].pImageInfo = NULL;
				descriptor_writes[j].pTexelBufferView = NULL;
				descriptor_writes[j].pNext = NULL;
				buffer_it++;
			}

			// Sampler
			else if (bindings[j].descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
			{
				image_infos[sampler_it].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				image_infos[sampler_it].imageView = texture_get_image_view(textures[sampler_it]);
				image_infos[sampler_it].sampler = sampler_get_vksampler(samplers[sampler_it]);

				descriptor_writes[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptor_writes[j].dstSet = pack->sets[i];
				descriptor_writes[j].dstBinding = bindings[j].binding;
				descriptor_writes[j].dstArrayElement = 0;
				descriptor_writes[j].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptor_writes[j].descriptorCount = 1;
				descriptor_writes[j].pBufferInfo = NULL;
				descriptor_writes[j].pImageInfo = &image_infos[sampler_it];
				descriptor_writes[j].pTexelBufferView = NULL;
				descriptor_writes[j].pNext = NULL;
				sampler_it++;
			}
			else
			{
				LOG_W("Descriptor set writing: Unsupported descriptor type");
			}
		}

		vkUpdateDescriptorSets(device, binding_count, descriptor_writes, 0, NULL);
	}
	free(descriptor_writes);
	free(buffer_infos);
	free(image_infos);
	return;
}

void descriptorpack_destroy(DescriptorPack* pack)
{
	struct DescriptorPool* pool = &descriptor_pools[pack->pool_index];
	if (pool == NULL)
	{
		LOG_E("Invalid pool");
	}
	//vkFreeDescriptorSets(device, pool->vkpool, pack->count, pack->sets);

	// Put back the available descriptor types to the pool
	//pool->uniform_count += pack->uniform_count;
	//pool->sampler_count += pack->sampler_count;

	// Descrease size of pool
	--pool->alloc_count;
	// Free descriptor set in pool
	// Pool is empty
	if (pool->alloc_count == 0)
	{
		descriptorpool_destroy(pool);
	}

	free(pack);
}
static mempool_t ub_ptr_pool = MEMPOOL_INIT(sizeof(UniformBuffer), 128);

UniformBuffer* ub_create(uint32_t size, uint8_t thread_idx)
{
	//LOG_S("Creating uniform buffer");
	UniformBuffer* ub = mempool_alloc(&ub_ptr_pool);
	if (size > memory_limits.maxUniformBufferRange)
	{
		LOG_E("Uniform buffer size %d is larger than device limit %d", size, memory_limits.maxUniformBufferRange);
	}
	ub->size = size;
	ub->thread_idx = thread_idx;

	// Find a free pool
	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		// Sets buffer pool usage if not set
		if (ub_pool[thread_idx].usage == 0)
			ub_pool[thread_idx].usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		buffer_pool_alloc(&ub_pool[thread_idx], size, &ub->buffers[i], &ub->memories[i], &ub->offsets[i]);
	}

	return ub;
}

// Maps the uniform buffer data for specified frame and returns a pointer to it
void* ub_map(UniformBuffer* ub, uint32_t offset, uint32_t size, uint32_t frame)
{
	if (frame == (uint32_t)-1)
		frame = renderer_get_frameindex();

	if (size == CS_WHOLE_SIZE)
		size = ub->size - offset;

#ifdef DEBUG
	if (offset + size > ub->size)
		LOG_W("Size and offset of uniform update exceeds capacity. Uniform buffer of %d bytes is being updated with %d "
			  "bytes at an offset of %d",
			  ub->size, size, offset);
#endif

	void* ptr = NULL;
	vkMapMemory(device, ub->memories[frame], ub->offsets[frame] + offset, size, 0, &ptr);
	if (ptr == NULL)
	{
		LOG_E("Failed to map memory for uniform buffer");
		return NULL;
	}
	return ptr;
}

// Unmaps a uniform buffer
void ub_unmap(UniformBuffer* ub, uint32_t frame)
{
	vkUnmapMemory(device, ub->memories[frame]);
}

void ub_update(UniformBuffer* ub, void* data, uint32_t offset, uint32_t size, uint32_t frame)
{
	if (frame == UB_CURRENT_FRAME)
		frame = renderer_get_frameindex();

	if (size == CS_WHOLE_SIZE)
		size = ub->size - offset;

#ifdef DEBUG
	if (offset + size > ub->size)
		LOG_W("Size and offset of uniform update exceeds capacity. Uniform buffer of %d bytes is being updated with %d "
			  "bytes at an offset of %d",
			  ub->size, size, offset);
#endif

	void* data_map = NULL;
	vkMapMemory(device, ub->memories[frame], ub->offsets[frame] + offset, size, 0, &data_map);
	if (data_map == NULL)
	{
		LOG_E("Failed to map memory for uniform buffer");
		return;
	}
	memcpy(data_map, data, size);
	vkUnmapMemory(device, ub->memories[frame]);
}

void ub_destroy(UniformBuffer* ub)
{
	//LOG_S("Destroying uniform buffer");
	for (size_t i = 0; i < swapchain_image_count; i++)
	{
		buffer_pool_free(&ub_pool[ub->thread_idx], ub->size, ub->buffers[i], ub->memories[i], ub->offsets[i]);
	}
	mempool_free(&ub_ptr_pool, ub);
}

void ub_pools_destroy()
{
	for (uint8_t i = 0; i < RENDERER_MAX_THREADS; i++)
	{
		if (ub_pool[i].usage != 0)
			buffer_pool_array_destroy(&ub_pool[i]);
	}
}
