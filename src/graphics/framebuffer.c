#include "graphics/framebuffer.h"
#include "graphics/vulkan_members.h"
#include "magpie.h"
#include "log.h"

Framebuffer* framebuffer_create(Texture** attachments, uint32_t attachment_count)
{
	Framebuffer* framebuffer = malloc(sizeof(Framebuffer));

	framebuffer->attachment_count = attachment_count;

	VkImageView* attachment_views = malloc(attachment_count * sizeof(VkImageView));
	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		framebuffer->attachments[i] = malloc(attachment_count * sizeof *framebuffer->attachments);

		for (uint32_t j = 0; j < attachment_count; j++)
		{
			framebuffer->attachments[i][j] = attachments[i][j];
			attachment_views[j] = texture_get_image_view(attachments[i][j]);
		}

		VkFramebufferCreateInfo framebufferInfo = {0};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = attachment_count;
		framebufferInfo.pAttachments = attachment_views;
		framebufferInfo.width = swapchain_extent.width;
		framebufferInfo.height = swapchain_extent.height;
		framebufferInfo.layers = 1;

		VkResult result = vkCreateFramebuffer(device, &framebufferInfo, NULL, &framebuffer->vkFramebuffers[i]);
		if (result != VK_SUCCESS)
		{
			LOG_E("Failed to create frambuffer %d - code %d", i, result);
			free(framebuffer);
			return NULL;
		}
	}

	free(attachment_views);

	return framebuffer;
}

void framebuffer_destroy(Framebuffer* framebuffer)
{

	texture_destroy(framebuffer->attachments[0][0]);
	texture_destroy(framebuffer->attachments[0][1]);

	texture_destroy(framebuffer->attachments[0][2]);
	texture_destroy(framebuffer->attachments[1][2]);
	texture_destroy(framebuffer->attachments[2][2]);

	for (uint32_t i = 0; i < swapchain_image_count; i++)
	{
		vkDestroyFramebuffer(device, framebuffer->vkFramebuffers[i], NULL);
		free(framebuffer->attachments[i]);
	}

	free(framebuffer);
}