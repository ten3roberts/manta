#include "head.h"
#include "log.h"
#include "graphics/vulkan_internal.h"
#include "graphics/vertexbuffer.h"

VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

VkDescriptorSet* descriptor_sets = VK_NULL_HANDLE;

typedef struct
{
	// Describes the currents used size of the buffer/memory
	uint32_t filled_size;
	// Describes the total size of the buffer/memory that was allocated
	uint32_t alloc_size;
	VkBuffer buffers;
	VkDeviceMemory memory;
} UniformBufferPool;

typedef struct
{
	Head head;
	uint32_t size;
	VkBuffer* buffers;
	VkDeviceMemory* memories;
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

int ub_create_descriptor_pool()
{
	VkDescriptorPoolSize poolSize = {0};
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = swapchain_image_count;

	VkDescriptorPoolCreateInfo poolInfo = {0};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = 1;
	poolInfo.pPoolSizes = &poolSize;

	poolInfo.maxSets = swapchain_image_count;

	VkResult result = vkCreateDescriptorPool(device, &poolInfo, NULL, &descriptor_pool);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create descriptor pool - code %d", result);
		return -1;
	}
	return 0;
}

int ub_create_descriptor_sets(UniformBuffer* ub, uint32_t size)
{
	VkDescriptorSetLayout* layouts = malloc(swapchain_image_count * sizeof(VkDescriptorSetLayout));
	for (size_t i = 0; i < swapchain_image_count; i++)
		layouts[i] = descriptorSetLayout;

	VkDescriptorSetAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptor_pool;
	allocInfo.descriptorSetCount = swapchain_image_count;
	allocInfo.pSetLayouts = layouts;
	descriptor_sets = malloc(swapchain_image_count * sizeof(VkDescriptorSet));
	vkAllocateDescriptorSets(device, &allocInfo, descriptor_sets);
	for (size_t i = 0; i < swapchain_image_count; i++)
	{
		VkDescriptorBufferInfo bufferInfo = {0};
		bufferInfo.buffer = ub->buffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = size;

		VkWriteDescriptorSet descriptorWrite = {0};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptor_sets[i];
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

	ub->buffers = malloc(swapchain_image_count * sizeof(VkBuffer));
	ub->memories = malloc(swapchain_image_count * sizeof(VkDeviceMemory));

	for (size_t i = 0; i < swapchain_image_count; i++)
	{
		buffer_create(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
					  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &ub->buffers[i],
					  &ub->memories[i]);
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

void ub_bind(UniformBuffer* ub, VkCommandBuffer command_buffer, int i)
{
	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptor_sets[i],
							0, NULL);
}
