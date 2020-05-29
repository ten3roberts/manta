#include "vulkan/vulkan.h"
#include "graphics/texture.h"

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

typedef struct Framebuffer
{
	VkFramebuffer vkFramebuffers[3];
	Texture** attachments[3];
	uint32_t attachment_count;
} Framebuffer;

// Creates a framebuffer with attachments
// Takes in a list of textures for each frame in the swapchain
// The textures can be the same in the different frames except if the image is to be presented directly
// Each texture works as an attachment for the framebuffer
Framebuffer* framebuffer_create(Texture** attachments[3], uint32_t attachment_count);
void framebuffer_destroy(Framebuffer* framebuffer);

#endif