#include "vulkan/vulkan.h"

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

struct FramebufferAttachment
{
	VkImage image;
	VkDeviceMemory image_memory;
};

// Contains framebuffers for all swapchain images
typedef struct Framebuffer
{
	VkFramebuffer vkFramebuffers[3];
	VkImage color_image;
	VkDeviceMemory color_image_memory;
	VkImageView color_image_view;

	VkImage depth_image;
	VkDeviceMemory depth_image_memory;
	VkImageView depth_image_view;
	VkFormat depth_image_format;
} Framebuffer;

// Creates a framebuffer with attachments
Framebuffer* framebuffer_create();
void framebuffer_destroy(Framebuffer* framebuffer);

#endif