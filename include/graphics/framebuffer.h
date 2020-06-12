#include "vulkan/vulkan.h"
#include "graphics/texture.h"
#include <stdbool.h>

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

// Color attachemnt
#define FRAMEBUFFER_COLOR_ATTACHMENT 1
// Depth attachment
#define FRAMEBUFFER_DEPTH_ATTACHMENT 2
// Attachment with 1 sample that resolve samples for presentation
#define FRAMEBUFFER_RESOLVE_ATTACHMENT	 4
#define FRAMEBUFFER_ATTACHMENT_MAX_COUNT 3

struct FramebufferInfo
{
	// If set to true, the framebuffer width and height will match that of the swapchain
	bool swapchain_target;
	// If swapchain_target, indicates which image in the swapchain to target
	uint32_t swapchain_target_index;
	// Width of all attachments in the framebuffer
	uint32_t width;
	// Height of all attachments in the framebuffer
	uint32_t height;

	// The amount of sampler in the framebuffer
	// Note: if framebuffer is swapchain target, the samples need to be resolved to 1
	uint32_t sampler_count;

	// Bit field of the attachments to create for this framebuffer
	uint32_t attachments;
};

typedef struct Framebuffer
{
	VkFramebuffer vkFramebuffer;
	Texture attachments[FRAMEBUFFER_ATTACHMENT_MAX_COUNT];
	uint32_t attachment_count;
	struct FramebufferInfo info;
} Framebuffer;

// Creates a framebuffer with according to the passed info
// Info is copied and stored in the framebuffer which allows recreation
Framebuffer* framebuffer_create(const struct FramebufferInfo* info);
void framebuffer_destroy(Framebuffer* framebuffer);

Texture framebuffer_get_attachment(Framebuffer* framebuffer, uint32_t attachment);

#endif