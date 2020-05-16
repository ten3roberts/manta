#include "graphics/rendertree.h"
#include "defines.h"
#include "mempool.h"
#include "log.h"
#include "graphics/vulkan_members.h"
#include "graphics/renderer.h"
#include <string.h>
#include <assert.h>
#include "stdio.h"
static mempool_t* node_pool = NULL;
static VkDescriptorSetLayout entity_data_layout = VK_NULL_HANDLE;
static VkDescriptorSetLayoutBinding entity_data_binding =
	(VkDescriptorSetLayoutBinding){.binding = 0,
								   .descriptorCount = 1,
								   .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
								   .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
								   .pImmutableSamplers = 0};

VkDescriptorSetLayoutBinding* rendertree_get_descriptor_bindings(void)
{
	return &entity_data_binding;
}

uint32_t rendertree_get_descriptor_binding_count(void)
{
	return 1;
}

VkDescriptorSetLayout rendertree_get_descriptor_layout(void)
{
	// Create entity data layout if it doesn't exist
	if (entity_data_layout == VK_NULL_HANDLE)
	{
		descriptorlayout_create(&entity_data_binding, 1, &entity_data_layout);
	}

	return entity_data_layout;
}

RenderTreeNode* rendertree_create(float halfwidth, vec3 center, uint32_t thread_idx)
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
	node->center = center;
	node->halfwidth = halfwidth;
	node->changed = 0;
	node->depth = 0;
	node->thread_idx = 0;

	for (uint8_t i = 0; i < 8; i++)
		node->children[i] = NULL;

	node->entity_count = 0;

	// Create command buffers for eac swapchain image
	for (uint8_t i = 0; i < 3; i++)
	{
		node->commandbuffers[i] = commandbuffer_create_secondary(thread_idx, i, renderPass, framebuffers[i]);
		node->commandbuffers[i].frame = i;
	}

	// Create shader data
	node->entity_data = ub_create(RENDER_TREE_LIM * sizeof(struct EntityData), 0, node->thread_idx);

	descriptorpack_create(rendertree_get_descriptor_layout(), &entity_data_binding, 1, &node->entity_data, NULL, &node->entity_data_descriptors);

	return node;
}

void rendertree_subdivide(RenderTreeNode* node)
{
	float new_width = (node->halfwidth) / 2;

	for (uint32_t i = 0; i < 8; i++)
	{
		// 000 ---
		// 001 +--
		// 010 -+-
		// 011 ++-
		// 100 --+
		// 101 +-+
		// 110 -++
		// 111 +++
		vec3 center = vec3_zero;
		if ((i & 1) == 1)
		{
			printf("+");
			center.x = new_width;
		}
		else
		{
			printf("-");
			center.x = -new_width;
		}
		if ((i & 2) == 2)
		{
			printf("+");
			center.y = new_width;
		}
		else
		{
			printf("-");
			center.y = -new_width;
		}
		if ((i & 4) == 4)
		{
			printf("+");
			center.z = new_width;
		}
		else
		{
			printf("-");

			center.z = -new_width;
		}
		printf("\n");
		node->children[i] = rendertree_create(new_width, center, node->thread_idx);
		node->children[i]->parent = node;
		node->children[i]->depth = node->depth + 1;
	}
}

// Checks and replaces necessary entities if they don't fit
static void rendertree_check(RenderTreeNode* node)
{
	for (uint32_t i = 0; i < node->entity_count; i++)
	{
		Entity* entity = node->entities[i];

		// If entity no longer fits
		// Try to place in parent
		if (rendertree_fits(node, entity) == false)
		{
			// Remove entity
			memmove(node->entities + i, node->entities + i + 1, (node->entity_count - i - 1) * sizeof *node->entities);
			node->entity_count--;

			// Re-place up
			rendertree_place_up(node->parent, entity);
		}
		// Entity still fits, check if it fits in any child j (if subdivided)
		else
		{
			for (uint32_t j = 0; node->children[0] && j < 8; j++)
			{
				if (rendertree_fits(node->children[j], entity))
				{
					// Remove entity
					memmove(node->entities + i, node->entities + i + 1, (node->entity_count - i - 1) * sizeof *node->entities);
					node->entity_count--;

					// Re-place down into child
					rendertree_place_down(node->children[j], entity);
					break;
				}
				if (j == 7)
					LOG("Did not fit in any child");
			}
		}
	}
}

void rendertree_update(RenderTreeNode* node, uint32_t frame)
{
	// Empty child
	if (node->entity_count == 0)
	{
		return;
	}
	// Map entity info
	if (node->entity_count > RENDER_TREE_LIM)
	{
		LOG_E("Entities in tree node exceeds capcity of %d", RENDER_TREE_LIM);
	}
	void* p_entity_data = ub_map(node->entity_data, 0, node->entity_count * sizeof(struct EntityData), frame);

	// Check and replace necessary entities
	rendertree_check(node);

	// Update all entities
	for (uint32_t i = 0; i < node->entity_count; i++)
	{
		Entity* entity = node->entities[i];
		// Update entity normally
		entity_update(entity);
		entity_update_shaderdata(entity, p_entity_data, i);
	}

	ub_unmap(node->entity_data, frame);

	// Recursively update children
	for (uint32_t i = 0; node->children[0] && i < 8; i++)
	{
		rendertree_update(node->children[i], frame);
	}
}

void rendertree_render(RenderTreeNode* node, CommandBuffer* primary, Camera* camera, uint32_t frame)
{
	// Begin recording
	commandbuffer_begin(&node->commandbuffers[frame]);

	// Map entity info
	for (uint32_t i = 0; i < node->entity_count; i++)
	{
		Entity* entity = node->entities[i];
		entity_render(entity, &node->commandbuffers[frame], i, node->entity_data_descriptors.sets[frame]);
	}

	// End recording
	commandbuffer_end(&node->commandbuffers[frame]);

	// Record into primary

	vkCmdExecuteCommands(primary->buffer, 1, &node->commandbuffers[frame].buffer);

	// For debug mode, show tree

	renderer_draw_cube_wire(node->center, quat_identity, (vec3){node->halfwidth, node->halfwidth, node->halfwidth}, vec4_hsv(node->depth, 1, 1));

	// Recurse children
	for (uint32_t i = 0; node->children[0] && i < 8; i++)
	{
		rendertree_render(node->children[i], primary, camera, frame);
	}
}

bool rendertree_fits(RenderTreeNode* node, Entity* entity)
{
	SphereCollider* e_bound = (SphereCollider*)entity_get_boundingsphere(entity);
	e_bound->radius = 0.0f;
	// Left bound (-x)
	if (e_bound->base.transform->position.x + e_bound->radius < node->center.x - node->halfwidth)
	{
		LOG("Not fit left");
		return false;
	}
	// Right bound (+x)
	if (e_bound->base.transform->position.x + e_bound->radius > node->center.x + node->halfwidth)
	{
		LOG("Not fit right");
		return false;
	}
	// Bottom bound (-y)
	if (e_bound->base.transform->position.y + e_bound->radius < node->center.y - node->halfwidth)
	{
		LOG("Not fit bottom");

		return false;
	}
	// Top bound (+y)
	if (e_bound->base.transform->position.y + e_bound->radius > node->center.y + node->halfwidth)
	{
		LOG("Not fit top");

		return false;
	}

	// Front bound (z)
	if (e_bound->base.transform->position.z + e_bound->radius < node->center.z - node->halfwidth)
	{
		LOG("Not fit front");

		return false;
	}
	// back bound (-z)
	if (e_bound->base.transform->position.z + e_bound->radius > node->center.z + node->halfwidth)
	{
		LOG("Not fit back");

		return false;
	}

	return true;
}
bool rendertree_place_down(RenderTreeNode* node, Entity* entity)
{
	// Check if it fits in current node
	if (rendertree_fits(node, entity) == false)
	{
		return false;
	}

	// Check if node is still full when subdivided
	if (node->children[0] && node->entity_count >= RENDER_TREE_LIM)
	{
		LOG_E("Subdivided node it still full");
	}
	// Check if the tree needs to be subdivided
	if (node->children[0] == NULL && node->entity_count >= RENDER_TREE_LIM)
	{
		rendertree_subdivide(node);
		rendertree_check(node);
		LOG_S("Subdivided tree");
	}

	// Check if it fits in any children
	for (uint32_t i = 0; node->children[0] && i < 8; i++)
	{
		if (rendertree_place_down(node->children[i], entity))
			return true;
	}

	// Fits only in this node
	// Insert
	node->entities[node->entity_count++] = entity;
	return true;
}

bool rendertree_place_up(RenderTreeNode* node, Entity* entity)
{
	RenderTreeNode* parent = node->parent;
	// Check if it fits in current node, if not, move to parent node
	while (rendertree_fits(node, entity) == false)
	{
		// At root element without fit
		if (parent == NULL)
		{
			LOG_E("Entity %s does not fit in the bounds of the tree", entity_get_name(entity));
			return false;
		}
		parent = node->parent;
	}

	// Fits in this node
	// Place down as far as possible
	rendertree_place_down(parent, entity);
	return true;
}

void rendertree_destroy(RenderTreeNode* node)
{
	for (uint32_t i = 0; node->children[0] && i < 8; i++)
	{
		rendertree_destroy(node->children[i]);
	}

	ub_destroy(node->entity_data);
	descriptorpack_destroy(&node->entity_data_descriptors);
	for (uint32_t i = 0; i < 3; i++)
	{
		commandbuffer_destroy(&node->commandbuffers[i]);
	}
	mempool_free(node_pool, node);
	if (mempool_get_count(node_pool) == 0)
	{
		vkDestroyDescriptorSetLayout(device, entity_data_layout, NULL);
		entity_data_layout = VK_NULL_HANDLE;
		mempool_destroy(node_pool);
		node_pool = 0;
	}
}