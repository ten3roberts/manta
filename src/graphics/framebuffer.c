#include "graphics/framebuffer.h"
#include "graphics/vulkan_internal.h"
#include "magpie.h"
#include "log.h"
#include <math.h>

#define FRAMEBUFFER_COLOR_INDEX	  (uint32_t) log2(FRAMEBUFFER_COLOR_ATTACHMENT)
#define FRAMEBUFFER_DEPTH_INDEX	  (uint32_t) log2(FRAMEBUFFER_DEPTH_ATTACHMENT)
#define FRAMEBUFFER_RESOLVE_INDEX (uint32_t) log2(FRAMEBUFFER_RESOLVE_ATTACHMENT)

Framebuffer* framebuffer_create(const struct FramebufferInfo* info)
{
	Framebuffer* framebuffer = malloc(sizeof(Framebuffer));

	// Copy info to framebuffer
	framebuffer->info = *info;

	// Repoint info pointer to not confuse copied info with passed info
	info = &framebuffer->info;

	VkFormat color_format = VK_FORMAT_R8G8B8A8_SRGB;
	VkImageView attachment_views[FRAMEBUFFER_ATTACHMENT_MAX_COUNT];

	if (info->swapchain_target)
	{
		framebuffer->info.width = swapchain_extent.width;
		framebuffer->info.height = swapchain_extent.height;
		color_format = swapchain_image_format;
	}

	for (int i = 0; i < FRAMEBUFFER_ATTACHMENT_MAX_COUNT; i++)
	{
		framebuffer->attachments[i] = INVALID(Texture);
	}
	framebuffer->attachment_count = 0;

	// Create attachments
	if (info->attachments & FRAMEBUFFER_COLOR_ATTACHMENT)
	{
		framebuffer->attachments[FRAMEBUFFER_COLOR_INDEX] = texture_create(NULL, info->width, info->height, color_format, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, msaa_samples, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_ASPECT_COLOR_BIT);

		attachment_views[framebuffer->attachment_count] = texture_get_image_view(framebuffer->attachments[FRAMEBUFFER_COLOR_INDEX]);

		framebuffer->attachment_count++;
	}

	if (info->attachments & FRAMEBUFFER_DEPTH_ATTACHMENT)
	{
		framebuffer->attachments[FRAMEBUFFER_DEPTH_INDEX] = texture_create(NULL, info->width, info->height, find_depth_format(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, msaa_samples, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

		attachment_views[framebuffer->attachment_count] = texture_get_image_view(framebuffer->attachments[FRAMEBUFFER_DEPTH_INDEX]);

		framebuffer->attachment_count++;
	}

	if (info->attachments & FRAMEBUFFER_RESOLVE_ATTACHMENT)
	{
		if (info->sampler_count != VK_SAMPLE_COUNT_1_BIT)
		{
			framebuffer->attachments[FRAMEBUFFER_RESOLVE_INDEX] = texture_create_existing(NULL, swapchain_extent.width, swapchain_extent.height, swapchain_image_format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, swapchain_images[info->swapchain_target_index], VK_IMAGE_ASPECT_COLOR_BIT);

			attachment_views[framebuffer->attachment_count] = texture_get_image_view(framebuffer->attachments[FRAMEBUFFER_RESOLVE_INDEX]);

			framebuffer->attachment_count++;
		}
		// No resolve is needed
		// Resolve is the same as color attachment
		else
		{
			framebuffer->attachments[FRAMEBUFFER_RESOLVE_INDEX] = framebuffer->attachments[FRAMEBUFFER_COLOR_INDEX];
		}
	}

	VkFramebufferCreateInfo framebufferInfo = {0};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = framebuffer->attachment_count;
	framebufferInfo.pAttachments = attachment_views;
	framebufferInfo.width = swapchain_extent.width;
	framebufferInfo.height = swapchain_extent.height;
	framebufferInfo.layers = 1;

	VkResult result = vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffer->vkFramebuffer);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create frambuffer - code %d", result);
		free(framebuffer);
		return NULL;
	}
	return framebuffer;
}

void framebuffer_destroy(Framebuffer* framebuffer)
{

	for (int i = 0; i < FRAMEBUFFER_ATTACHMENT_MAX_COUNT; i++)
	{
		if (HANDLE_VALID(framebuffer->attachments[i]))
			texture_destroy(framebuffer->attachments[i]);
	}

	vkDestroyFramebuffer(device, framebuffer->vkFramebuffer, NULL);

	free(framebuffer);
}

Texture framebuffer_get_attachment(Framebuffer* framebuffer, uint32_t attachment)
{
	if ((framebuffer->info.attachments & attachment) == 0)
	{
		LOG_E("Framebuffer does not have attachment %d", attachment);
		return INVALID(Texture);
	}
	return framebuffer->attachments[(uint32_t)log2(attachment)];
}