#include "graphics/framebuffer.h"
#include "magpie.h"

/*static int create_color_buffer()
{
	VkFormat colorFormat = swapchain_image_format;

	image_create(swapchain_extent.width, swapchain_extent.height, colorFormat, VK_IMAGE_TILING_OPTIMAL,
				 VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &color_image, &color_image_memory,
				 msaa_samples);
	color_image_view = image_view_create(color_image, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	return 0;
}

static int create_depth_buffer()
{
	depth_image_format = find_depth_format();

	image_create(swapchain_extent.width, swapchain_extent.height, depth_image_format, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
				 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth_image, &depth_image_memory, msaa_samples);

	depth_image_view = image_view_create(depth_image, depth_image_format, VK_IMAGE_ASPECT_DEPTH_BIT);

	// Transition image layout explicitely
	transition_image_layout(depth_image, depth_image_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	return 0;
}*/

Framebuffer* framebuffer_create()
{
	Framebuffer* framebuffer = malloc(sizeof(Framebuffer));

	/*for (size_t i = 0; i < 3; i++)
	{
		VkImageView attachments[] = {color_image_view, depth_image_view, swapchain_image_views[i]};

		VkFramebufferCreateInfo framebufferInfo = {0};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = sizeof(attachments) / sizeof(*attachments);
		framebufferInfo.pAttachments = attachments;
		framebufferInfo.width = swapchain_extent.width;
		framebufferInfo.height = swapchain_extent.height;
		framebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffers[i]);
		if (result != VK_SUCCESS)
		{
			LOG_E("Failed to create frambuffer %d - code %d", i, result);
			return -i;
		}
	}*/
	return framebuffer;
}
void framebuffer_destroy(Framebuffer* framebuffer);