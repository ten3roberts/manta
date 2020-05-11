#include "graphics/rendertree.h"
#include "defines.h"
#include "mempool.h"
#include "log.h"
#include <string.h>

static mempool_t* node_pool = NULL;

RenderTreeNode* rendertree_create(float halfwidth, vec3 origin, uint32_t thread_idx)
{
	// Pool allocations for faster deletion and creation without fragmenting
	// memory
	if (node_pool == 0)
	{
		node_pool = mempool_create(sizeof(RenderTreeNode), 64);
	}

	RenderTreeNode* node = mempool_alloc(node_pool);

	node->parent = NULL;
	node->depth = 0;
	node->left = -halfwidth + origin.x;
	node->right = halfwidth + origin.x;
	node->top = halfwidth + origin.y;
	node->bottom = -halfwidth + origin.y;

	node->changed = 0;
	node->depth = 0;

	for (uint8_t i = 0; i < 8; i++)
		node->children[i] = NULL;

	node->entity_count = 0;

	// Create command buffers
	for (uint8_t i = 0; i < 3; i++)
	{
		node->commandbuffers[i] = commandbuffer_create_secondary(thread_idx);
		node->commandbuffers[i].frame = i;
	}

	return node;
}

void rendertree_split(RenderTreeNode* node)
{
	float new_width = (node->right - node->left) / 2;

	for (uint32_t i = 0; i < 8; i++)
	{
		vec3 origin = vec3_zero;
		if (i & 2)
		{
			origin.x = new_width;
		}
		else
		{
			origin.x = -new_width;
		}
		if (i & 4)
		{
			origin.y = new_width;
		}
		else
		{
			origin.y = -new_width;
		}
		if (i & 8)
		{
			origin.z = new_width;
		}
		else
		{
			origin.z -= -new_width;
		}

		node->children[i] = rendertree_create(new_width, origin, node->thread_idx);
		node->children[i]->parent = node;
		node->children[i]->depth = node->depth + 1;
	}
}

void rendertree_update(RenderTreeNode* node)
{
	for (uint32_t i = 0; i < node->entity_count; i++)
	{
		Entity* entity = node->entities[i];

		// Check if entity still fits
		if (rendertree_fits(node, entity) == false)
		{
			// Remove entity
			memmove(node->entities + i, node->entities + i + 1, (node->entity_count - i - 1) * sizeof *node->entities);
			node->entity_count--;

			// Replace
			rendertree_place(node, entity);
		}
		// Check if entity can fit in any child node
		else
		{
			for (uint32_t i = 0; node->children[0] && i < 8; i++)
			{
				if (rendertree_fits(node->children[i], entity))
				{
					// Remove entity
					memmove(node->entities + i, node->entities + i + 1, (node->entity_count - i - 1) * sizeof *node->entities);
					node->entity_count--;

					// Replace
					rendertree_place(node, entity);
					break;
				}
			}
		}

		// Update entity normally
		entity_update(entity);
	}

	// Recurse children
	for (uint32_t i = 0; node->children[0] && i < 8; i++)
	{
		rendertree_update(node->children[i]);
	}
}

bool rendertree_fits(RenderTreeNode* node, Entity* entity)
{
	const SphereCollider* e_bound = entity_get_boundingsphere(entity);
	// Left bound (-x)
	if (e_bound->base.transform->position.x + e_bound->radius < node->left)
	{
		return false;
	}
	// Right bound (+x)
	if (e_bound->base.transform->position.x + e_bound->radius > node->right)
	{
		return false;
	}
	// Bottom bound (-y)
	if (e_bound->base.transform->position.y + e_bound->radius < node->bottom)
	{
		return false;
	}
	// Top bound (+y)
	if (e_bound->base.transform->position.y + e_bound->radius > node->top)
	{
		return false;
	}

	return true;
}
void rendertree_place(RenderTreeNode* node, Entity* entity)
{
	if (rendertree_fits(node, entity))
	{
		// Check if it could fit in  children

		for (uint32_t i = 0; node->children[0] && i < 8; i++)
		{
			// Fits in a child node as well, go down
			if (rendertree_fits(node->children[i], entity))
			{
				rendertree_place(node, entity);
				return;
			}
		}
		node->entities[node->entity_count++] = entity;

		// Split leaf node if full
		if (node->children[0] == NULL && node->entity_count == RENDER_TREE_LIM)
		{
			rendertree_split(node);
			rendertree_update(node);
		}
	}

	else
	{
		LOG_E("Entity outside octree bounds");
		// Doesn't fit in current node, go up
		rendertree_place(node->parent, entity);
	}
}
