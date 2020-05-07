#include "graphics/renderprune.h"
#include "mempool.h"

static mempool_t* node_pool = NULL;

RenderTreeNode* rendertree_create(uint32_t thread_idx)
{
	// Pool allocations for faster deletion and creation without fragmenting memory
	if (node_pool == 0)
	{
		node_pool = mempool_create(sizeof(RenderTreeNode), 64);
	}
	RenderTreeNode* node = mempool_alloc(node_pool);
	node->changed		 = 0;

	for (uint8_t i = 0; i < 8; i++)
		node->children[i] = NULL;

	node->entity_count = 0;

	// Create command buffers
	for (uint8_t i = 0; i < 3; i++)
	{
		node->commandbuffers[i]		  = commandbuffer_create_secondary(thread_idx);
		node->commandbuffers[i].frame = i;
	}

	// Create uniform buffer for binding 0 on set 3
	node->entity_data = ub_create(RENDER_TREE_LIM * sizeof(struct EntityData), 0);
	return node;
}

void rendertree_update(RenderTreeNode* root)
{
	//for (uint32_t i = 0; i < root->)
}