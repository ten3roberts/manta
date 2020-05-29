#include "graphics/rendertree.h"
#include "defines.h"
#include "mempool.h"
#include "log.h"
#include "graphics/vulkan_members.h"
#include "graphics/renderer.h"
#include <string.h>
#include <assert.h>
#include "stdio.h"

#define ALL_CHANGED (1 | 2 | 4)

static uint32_t node_count = 0;
static mempool_t node_pool = MEMPOOL_INIT(sizeof(RenderTreeNode), 1024);
static VkDescriptorSetLayout entity_data_layout = VK_NULL_HANDLE;
static VkDescriptorSetLayoutBinding entity_data_binding = (VkDescriptorSetLayoutBinding){.binding = 0,
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

static void rendertree_create_shader_data(RenderTreeNode* node)
{
	// Create secondary command buffers
	for (uint8_t i = 0; i < 3; i++)
	{
		// The fence is assigned on render
		node->commandbuffers[i] = commandbuffer_create_secondary(node->thread_idx, i, VK_NULL_HANDLE, renderPass, node->framebuffer->vkFramebuffers[i]);
	}

	// Create uniform buffers for entity data
	node->entity_data = ub_create((sizeof node->entities / sizeof *node->entities) * sizeof(struct EntityData), 0, node->thread_idx);

	// Create and write set=2 for entity data
	node->entity_data_descriptors = descriptorpack_create(rendertree_get_descriptor_layout(), &entity_data_binding, 1);
	descriptorpack_write(node->entity_data_descriptors, &entity_data_binding, 1, &node->entity_data, NULL, NULL);
}

RenderTreeNode* rendertree_create(float halfwidth, vec3 center, uint32_t thread_idx, Framebuffer* framebuffer)
{
	RenderTreeNode* node = mempool_alloc(&node_pool);

	node->parent = NULL;
	node->depth = 0;
	node->center = center;
	node->halfwidth = halfwidth;
	node->changed = ALL_CHANGED;
	node->depth = 0;
	node->thread_idx = 0;
	node->id = node_count++;
	node->framebuffer = framebuffer;

	for (uint8_t i = 0; i < 8; i++)
		node->children[i] = NULL;

	node->entity_count = 0;
	// Create command buffers for eac swapchain image
	for (uint8_t i = 0; i < 3; i++)
	{
		// The fence is assigned on render
		node->commandbuffers[i] = NULL;
	}

	node->entity_data = NULL;
	node->entity_data_descriptors = NULL;

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
		vec3 center = node->center;
		if ((i & 1) == 1)
		{
			center.x += new_width;
		}
		else
		{
			center.x -= new_width;
		}
		if ((i & 2) == 2)
		{
			center.y += new_width;
		}
		else
		{
			center.y -= new_width;
		}
		if ((i & 4) == 4)
		{
			center.z += new_width;
		}
		else
		{
			center.z -= new_width;
		}
		node->children[i] = rendertree_create(new_width, center, node->thread_idx, node->framebuffer);
		node->children[i]->parent = node;
		node->children[i]->depth = node->depth + 1;
		node->changed = ALL_CHANGED;
	}
}

void rendertree_merge(RenderTreeNode* node)
{
	if (node->children[0] == NULL)
		return;
	for (uint32_t i = 0; i < 8; i++)
	{
		// As the first child gets set to NULL, it signifies that there are no children
		RenderTreeNode* child = node->children[i];
		// Merge children recursively if not leaf
		if (child->children[i])
		{
			LOG_E("Cannot merge node with non-leaf children");
			//rendertree_merge(child);
		}

		// 'remove' child
		node->children[i] = NULL;
		// Replace all children entities in this node or higher
		for (uint32_t j = 0; j < child->entity_count; j++)
		{
			Entity* entity = child->entities[j];
			// Place all entities in the parent
			rendertree_place_up(node, entity);
		}
		rendertree_destroy(child);
	}
	node->changed = ALL_CHANGED;
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
			node->entities[node->entity_count - 1] = NULL;
			node->entity_count--;

			// Re-place up
			rendertree_place_up(node, entity);
			node->changed = ALL_CHANGED;
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
					node->entities[node->entity_count - 1] = NULL;
					node->entity_count--;

					// Re-place down into child
					rendertree_place_down(node->children[j], entity);
					node->changed = ALL_CHANGED;

					break;
				}
			}
		}
	}
}

void rendertree_update(RenderTreeNode* node, uint32_t frame)
{
	// Check if need merge
	if (node->children[0] && node->children[0]->children[0] == NULL)
	{
		// Check if it needs to merge
		uint32_t entity_count = node->entity_count;
		bool is_leaf = true;
		for (uint32_t i = 0; node->children[0] && i < 8; i++)
		{
			if (node->children[i]->children[0])
			{
				is_leaf = false;
				break;
			}
			entity_count += node->children[i]->entity_count;
		}

		if (is_leaf && entity_count < RENDER_TREE_LIM * 0.8)
		{
			LOG("Merging node with %d entities in children", entity_count);
			rendertree_merge(node);
			return;
		}
	}

	// Update entities if not empty
	if (node->entity_count != 0)
	{
		//LOG("Updating %d entities", node->entity_count);

		// Map entity info
		if (node->entity_count > RENDER_TREE_LIM)
		{
			LOG_E("Entities in tree node exceeds capacity of %d", RENDER_TREE_LIM);
		}

		// Check and replace necessary entities
		rendertree_check(node);
	}

	// Recursively update children
	for (uint32_t i = 0; node->children[0] && i < 8; i++)
	{
		rendertree_update(node->children[i], frame);
	}
}

void rendertree_render(RenderTreeNode* node, CommandBuffer* primary, Camera* camera, uint32_t frame)
{
	// Render entities if not empty
	if (node->entity_count != 0)
	{
		// Create shader resources if not yet created (rendertree node was previously empty)
		if (node->entity_data == NULL)
		{
			rendertree_create_shader_data(node);
			node->changed = ALL_CHANGED;
		}

		// Update entity shader data
		void* p_entity_data = ub_map(node->entity_data, 0, node->entity_count * sizeof(struct EntityData), frame);
		for (uint32_t i = 0; i < node->entity_count; i++)
		{
			Entity* entity = node->entities[i];
			//entity_set_color(entity, vec4_hsv(node->depth, 1, 1));
			// Update entity normally
			entity_update(entity);
			entity_update_shaderdata(entity, p_entity_data, i);
		}
		ub_unmap(node->entity_data, frame);

		// Assign fence from primary for proper destruction
		node->commandbuffers[frame]->fence = primary->fence;

		// Needs to rerecord secondary
		if (node->changed & (1 << frame))
		{
			//LOG("Recording %d entities", node->entity_count);
			//LOG("Re-recording node at depth %d", node->depth);
			// Begin recording
			commandbuffer_begin(node->commandbuffers[frame]);
			//LOG("Rendering tree with depth %d", node->depth);
			// Map entity info
			for (uint32_t i = 0; i < node->entity_count; i++)
			{
				Entity* entity = node->entities[i];
				entity_render(entity, node->commandbuffers[frame], i, node->entity_data_descriptors->sets[frame]);
			}

			// End recording
			commandbuffer_end(node->commandbuffers[frame]);
			//renderer_draw_cube(node->center, quat_identity, (vec3){1.0f / node->depth, 1.0f / node->depth, 1.0f / node->depth}, vec4_hsv(node->depth, 1, 1));
		}
		//renderer_draw_cube_wire(node->center, quat_identity, (vec3){node->halfwidth, node->halfwidth, node->halfwidth}, vec4_hsv(node->depth, 1, 1));
		// Record into primary

		vkCmdExecuteCommands(primary->cmd, 1, &node->commandbuffers[frame]->cmd);

		// Remove changed bit for this frame
		node->changed = node->changed & ~(1 << frame);
	}

	// For debug mode, show tree
	// Recurse children
	for (uint32_t i = 0; node->children[0] && i < 8; i++)
	{
		rendertree_render(node->children[i], primary, camera, frame);
	}
}

bool rendertree_fits(RenderTreeNode* node, Entity* entity)
{
	SphereCollider* e_bound = (SphereCollider*)entity_get_boundingsphere(entity);
	/*// Left bound (-x)
	if (e_bound->base.transform->position.x < node->center.x - node->halfwidth)
	{
		return false;
	}
	// Right bound (+x)
	if (e_bound->base.transform->position.x > node->center.x + node->halfwidth)
	{
		return false;
	}
	// Bottom bound (-y)
	if (e_bound->base.transform->position.y < node->center.y - node->halfwidth)
	{
		return false;
	}
	// Top bound (+y)
	if (e_bound->base.transform->position.y > node->center.y + node->halfwidth)
	{
		return false;
	}

	// Front bound (z)
	if (e_bound->base.transform->position.z < node->center.z - node->halfwidth)
	{
		return false;
	}
	// back bound (-z)
	if (e_bound->base.transform->position.z > node->center.z + node->halfwidth)
	{
		return false;
	}*/

	if (e_bound->base.transform->position.x + e_bound->radius < node->center.x - node->halfwidth)
	{
		return false;
	}
	// Right bound (+x)
	if (e_bound->base.transform->position.x + e_bound->radius > node->center.x + node->halfwidth)
	{
		return false;
	}
	// Bottom bound (-y)
	if (e_bound->base.transform->position.y + e_bound->radius < node->center.y - node->halfwidth)
	{
		return false;
	}
	// Top bound (+y)
	if (e_bound->base.transform->position.y + e_bound->radius > node->center.y + node->halfwidth)
	{
		return false;
	}

	// Front bound (z)
	if (e_bound->base.transform->position.z + e_bound->radius < node->center.z - node->halfwidth)
	{
		return false;
	}
	// back bound (-z)
	if (e_bound->base.transform->position.z + e_bound->radius > node->center.z + node->halfwidth)
	{
		return false;
	}

	return true;
}
bool rendertree_place_down(RenderTreeNode* node, Entity* entity)
{
	// Check if it fits in current node
	if (rendertree_fits(node, entity) == false)
	{
		if (node->parent == NULL)
		{
			LOG_E("Entity %s does not fit in the bounds of the tree %3v", entity_get_name(entity), entity_get_transform(entity)->position);
			rendertree_fits(node, entity);
		}
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
		return true;
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
	node->changed = ALL_CHANGED;
	return true;
}

bool rendertree_place_up(RenderTreeNode* node, Entity* entity)
{
	// Check if it fits in current node, if not, move to parent node
	while (rendertree_fits(node, entity) == false)
	{
		node = node->parent;
		// At root element without fit
		if (node == NULL)
		{
			LOG_E("Entity %s does not fit in the bounds of the tree", entity_get_name(entity));
			return false;
		}
	}

	// Fits in this node
	// Place down as far as possible
	rendertree_place_down(node, entity);
	return true;
}

void rendertree_destroy(RenderTreeNode* node)
{
	for (uint32_t i = 0; node->children[0] && i < 8; i++)
	{
		rendertree_destroy(node->children[i]);
	}

	if (node->entity_data)
	{
		for (uint32_t i = 0; i < 3; i++)
		{
			commandbuffer_destroy(node->commandbuffers[i]);
		}
		ub_destroy(node->entity_data);
		descriptorpack_destroy(node->entity_data_descriptors);
	}

	mempool_free(&node_pool, node);

	// Last node
	if (node_pool.alloc_count == 0)
		vkDestroyDescriptorSetLayout(device, entity_data_layout, NULL);
}