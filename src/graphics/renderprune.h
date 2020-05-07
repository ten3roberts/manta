// This file should only be used by the renderer
#ifndef RENDER_PRUNE_H
#define RENDER_PRUNE_H
#include "graphics/uniforms.h"
#include "graphics/shadertypes.h"
#include "graphics/commandbuffer.h"
#include "entity.h"
#include "defines.h"

// A node of the render pruning octree
typedef struct RenderTreeNode
{
	uint32_t entity_count;
	Entity* entites[RENDER_TREE_LIM];
	// Contains all RENDER_TREE_LIM entities data
	UniformBuffer* entity_data;
	CommandBuffer commandbuffers[3];
	struct RenderTreeNode* children[8];
	// A bit field of which frames should be rebuilt
	uint8_t changed;
} RenderTreeNode;

// Creates a rendertree node for a thread
// Note, only the render thread with the correct index should use this
// All children inherit the thread index
RenderTreeNode* rendertree_create(uint32_t thread_idx);

void rendertree_destroy(RenderTreeNode* node);

// Updates the tree recursively from root
// Re-places entities
// Queues swapchain recreation
void rendertree_update(RenderTreeNode* root);

void rendertree_split(RenderTreeNode* root);
// Joins all children
void rendertree_join(RenderTreeNode* node);

#endif