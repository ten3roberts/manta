// This file should only be used by the renderer
#ifndef RENDER_PRUNE_H
#define RENDER_PRUNE_H
#include "graphics/uniforms.h"
#include "graphics/shadertypes.h"
#include "graphics/commandbuffer.h"
#include "entity.h"
#include "defines.h"
#include "graphics/camera.h"

// A node of the render pruning octree
// Handled by scene on update
typedef struct RenderTreeNode
{
	// Tree data
	// The bounds for the tree
	float halfwidth;
	vec3 center;
	uint8_t depth;
	uint32_t id;

	struct RenderTreeNode* children[8];
	struct RenderTreeNode* parent;

	// Entity data
	uint32_t entity_count;
	Entity* entities[RENDER_TREE_LIM];

	// Contains all RENDER_TREE_LIM entities data
	CommandBuffer* commandbuffers[3];
	UniformBuffer* entity_data;
	// Set 2
	DescriptorPack* entity_data_descriptors;

	// A bit field of which frames should be rebuilt
	int changed : 3;
	uint8_t thread_idx;
} RenderTreeNode;

// Returns the single binding for the descriptor set
VkDescriptorSetLayoutBinding* rendertree_get_descriptor_bindings(void);

uint32_t rendertree_get_descriptor_binding_count(void);

// Returns a descriptor layout
VkDescriptorSetLayout rendertree_get_descriptor_layout(void);

// Creates a rendertree root node for a thread
// Note, only the render thread with the correct index should use this
// All children inherit the thread index
RenderTreeNode* rendertree_create(float halfwidth, vec3 center, uint32_t thread_idx);

void rendertree_destroy(RenderTreeNode* node);

// Updates the tree recursively from node(root)
// Re-places entities
// Queues swapchain recreation
void rendertree_update(RenderTreeNode* node, uint32_t frame);

// Records secondary command buffers if necessary for the node and all children recursively if they're in view
// Records secondary into primary command buffers
void rendertree_render(RenderTreeNode* node, CommandBuffer* primary, Camera* camera, uint32_t frame);

// Splits the node into 8 children
// If the node is root, the children are assigned separate threads
void rendertree_subdivide(RenderTreeNode* node);
// Joins all children
void rendertree_merge(RenderTreeNode* node);

// Returns true if an enitty is fully contained given node
// Entity cannot exist in any node when performing calling this function
bool rendertree_fits(RenderTreeNode* node, Entity* entity);

// Destroys a node and all children
void rendertree_destroy(RenderTreeNode* node);

// Places an entity into the tree
// Check if it fits in the start node
// If not it goes up until it fits
// It then goes down to the child it fits in recursively
// Inserts into the smallest node that fully contains the entity
// May split the tree

// Places an entity into the tree in the smallest node that fits from the given start node
// Tree can be split
// Note: the entity cannot exist in the tree when running this function
bool rendertree_place_down(RenderTreeNode* node, Entity* entity);

// Places an entity into the parent of node
// Loops up until it fits, and then down with rendertree_place_down
bool rendertree_place_up(RenderTreeNode* node, Entity* entity);

#endif