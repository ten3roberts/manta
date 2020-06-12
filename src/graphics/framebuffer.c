#include "graphics/framebuffer.h"
#include "graphics/vulkan_internal.h"
#include "magpie.h"
#include "log.h"
#include "handlepool.h"

#include <math.h>

#define FRAMEBUFFER_COLOR_INDEX	  (uint32_t) log2(FRAMEBUFFER_COLOR_ATTACHMENT)
#define FRAMEBUFFER_DEPTH_INDEX	  (uint32_t) log2(FRAMEBUFFER_DEPTH_ATTACHMENT)
#define FRAMEBUFFER_RESOLVE_INDEX (uint32_t) log2(FRAMEBUFFER_RESOLVE_ATTACHMENT)

typedef struct Framebuffer_raw
{
	VkFramebuffer vkFramebuffer;
	Texture attachments[FRAMEBUFFER_ATTACHMENT_MAX_COUNT];
	uint32_t attachment_count;
	struct FramebufferInfo info;
} Framebuffer_raw;

static handlepool_t framebuffer_pool = HANDLEPOOL_INIT(sizeof(Framebuffer_raw), "Framebuffer");

Framebuffer framebuffer_create(const struct FramebufferInfo* info)
{
	const struct handle_wrapper* wrapper = handlepool_alloc(&framebuffer_pool);
	Framebuffer_raw* raw = (Framebuffer_raw*)wrapper->data;
	Framebuffer handle = PUN_HANDLE(wrapper->handle, Framebuffer);

	// Copy info to framebuffer
	raw->info = *info;

	// Repoint info pointer to not confuse copied info with passed info
	info = &raw->info;

	VkFormat color_format = VK_FORMAT_R8G8B8A8_SRGB;
	VkImageView attachment_views[FRAMEBUFFER_ATTACHMENT_MAX_COUNT];

	if (info->swapchain_target)
	{
		raw->info.width = swapchain_extent.width;
		raw->info.height = swapchain_extent.height;
		color_format = swapchain_image_format;
	}

	for (int i = 0; i < FRAMEBUFFER_ATTACHMENT_MAX_COUNT; i++)
	{
		raw->attachments[i] = INVALID(Texture);
	}
	raw->attachment_count = 0;

	// Create attachments
	if (info->attachments & FRAMEBUFFER_COLOR_ATTACHMENT)
	{
		raw->attachments[FRAMEBUFFER_COLOR_INDEX] = texture_create(NULL, info->width, info->height, color_format, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, msaa_samples, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_ASPECT_COLOR_BIT);

		attachment_views[raw->attachment_count] = texture_get_image_view(raw->attachments[FRAMEBUFFER_COLOR_INDEX]);

		raw->attachment_count++;
	}

	if (info->attachments & FRAMEBUFFER_DEPTH_ATTACHMENT)
	{
		raw->attachments[FRAMEBUFFER_DEPTH_INDEX] = texture_create(NULL, info->width, info->height, find_depth_format(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, msaa_samples, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_ASPECT_DEPTH_BIT);

		attachment_views[raw->attachment_count] = texture_get_image_view(raw->attachments[FRAMEBUFFER_DEPTH_INDEX]);

		raw->attachment_count++;
	}

	if (info->attachments & FRAMEBUFFER_RESOLVE_ATTACHMENT)
	{
		if (info->sampler_count != VK_SAMPLE_COUNT_1_BIT)
		{
			raw->attachments[FRAMEBUFFER_RESOLVE_INDEX] = texture_create_existing(NULL, swapchain_extent.width, swapchain_extent.height, swapchain_image_format, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_UNDEFINED, swapchain_images[info->swapchain_target_index], VK_IMAGE_ASPECT_COLOR_BIT);

			attachment_views[raw->attachment_count] = texture_get_image_view(raw->attachments[FRAMEBUFFER_RESOLVE_INDEX]);

			raw->attachment_count++;
		}
		// No resolve is needed
		// Resolve is the same as color attachment
		else
		{
			raw->attachments[FRAMEBUFFER_RESOLVE_INDEX] = raw->attachments[FRAMEBUFFER_COLOR_INDEX];
		}
	}

	VkFramebufferCreateInfo framebufferInfo = {0};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.renderPass = renderPass;
	framebufferInfo.attachmentCount = raw->attachment_count;
	framebufferInfo.pAttachments = attachment_views;
	framebufferInfo.width = swapchain_extent.width;
	framebufferInfo.height = swapchain_extent.height;
	framebufferInfo.layers = 1;

	VkResult result = vkCreateFramebuffer(device, &framebufferInfo, NULL, &raw->vkFramebuffer);
	if (result != VK_SUCCESS)
	{
		LOG_E("Failed to create frambuffer - code %d", result);
		free(raw);
		return INVALID(Framebuffer);
	}
	return handle;
}

void framebuffer_destroy(Framebuffer framebuffer)
{
	Framebuffer_raw* raw = (Framebuffer_raw*)handlepool_get_raw(&framebuffer_pool, framebuffer);

	for (int i = 0; i < FRAMEBUFFER_ATTACHMENT_MAX_COUNT; i++)
	{
		if (HANDLE_VALID(raw->attachments[i]))
			texture_destroy(raw->attachments[i]);
	}

	vkDestroyFramebuffer(device, raw->vkFramebuffer, NULL);

	handlepool_free(&framebuffer_pool, framebuffer);
}

Texture framebuffer_get_attachment(Framebuffer framebuffer, uint32_t attachment)
{
	Framebuffer_raw* raw = (Framebuffer_raw*)handlepool_get_raw(&framebuffer_pool, framebuffer);

	if ((raw->info.attachments & attachment) == 0)
	{
		LOG_E("Framebuffer does not have attachment %d", attachment);
		return INVALID(Texture);
	}
	return raw->attachments[(uint32_t)log2(attachment)];
}

VkFramebuffer framebuffer_vk(Framebuffer framebuffer)
{
	Framebuffer_raw* raw = (Framebuffer_raw*)handlepool_get_raw(&framebuffer_pool, framebuffer);
	return raw->vkFramebuffer;
}
