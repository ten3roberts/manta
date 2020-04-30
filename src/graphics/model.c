#include "graphics/model.h"
#include "graphics/vertexbuffer.h"
#include "graphics/indexbuffer.h"
#include "xmlparser.h"
#include "log.h"
#include <utils.h>

struct Model
{
	char* name;
	char* id;
	VertexBuffer* vb;
	IndexBuffer* ib;
	uint32_t index_count;
	uint32_t vertex_count;
};

struct Face
{
	uint32_t pos_index, normal_index, uv_index;
};

Model* model_load_collada(const char* filepath)
{
	Model* model = malloc(sizeof(Model));
	LOG("Loading model %s", filepath);
	XMLNode* root = xml_loadfile(filepath);
	if (root == NULL)
	{
		LOG_E("Failed to open COLLADA model file %s", filepath);
		return NULL;
	}
	XMLNode* mesh = xml_get_child(root, "library_geometries");
	mesh = xml_get_children(mesh);
	model->name = stringdup(xml_get_attribute(mesh, "name"));
	model->id = stringdup(xml_get_attribute(mesh, "id"));
	mesh = xml_get_children(mesh);
	XMLNode* sources = xml_get_children(mesh);
	XMLNode* triangles_node = xml_get_child(mesh, "triangles");

	char* source_names[3];
	source_names[0] = malloc(strlen(model->id) + strlen("-positions") + 1);
	memcpy(source_names[0], model->id, strlen(model->id) + 1);
	strcat(source_names[0], "-positions");

	source_names[1] = malloc(strlen(model->id) + strlen("-map-0") + 1);
	memcpy(source_names[1], model->id, strlen(model->id) + 1);
	strcat(source_names[1], "-map-0");

	source_names[2] = malloc(strlen(model->id) + strlen("-normals") + 1);
	memcpy(source_names[2], model->id, strlen(model->id) + 1);
	strcat(source_names[2], "-normals");

	float* positions = NULL;
	float* uvs = NULL;
	float* normals = NULL;

	// Load all sources
	while (sources != NULL)
	{

		char* attribute = xml_get_attribute(sources, "id");
		if (attribute == NULL)
		{
			sources = xml_get_next(sources);
			continue;
		};

		// Positions
		if (strcmp(attribute, source_names[0]) == 0)
		{
			XMLNode* array = xml_get_child(sources, "float_array");
			uint32_t count = atoi(xml_get_attribute(array, "count"));
			positions = malloc(count * sizeof(*positions));
			char* cont = xml_get_content(array);
			for (uint32_t i = 0; i < count; i++)
			{
				positions[i] = atof(cont);
				cont = strchr(cont + 1, ' ');
			}
		}

		// Uvs
		else if (strcmp(attribute, source_names[1]) == 0)
		{
			XMLNode* array = xml_get_child(sources, "float_array");
			uint32_t count = atoi(xml_get_attribute(array, "count"));
			uvs = malloc(count * sizeof(*uvs));
			char* cont = xml_get_content(array);
			for (uint32_t i = 0; i < count; i++)
			{
				uvs[i] = atof(cont);
				cont = strchr(cont + 1, ' ');
			}
		}

		// Normals
		if (strcmp(attribute, source_names[2]) == 0)
		{
			XMLNode* array = xml_get_child(sources, "float_array");
			uint32_t count = atoi(xml_get_attribute(array, "count"));
			normals = malloc(count * sizeof(*normals));
			char* cont = xml_get_content(array);
			for (uint32_t i = 0; i < count; i++)
			{
				normals[i] = atof(cont);
				cont = strchr(cont + 1, ' ');
			}
		}
		sources = xml_get_next(sources);
	}
	free(source_names[0]);
	free(source_names[1]);
	free(source_names[2]);

	// Check what got loaded
	if (positions == NULL)
	{
		LOG_W("Model %s contains no vertex position data", filepath);
	}
	if (normals == NULL)
	{
		LOG_W("Model %s contains no normal data", filepath);
	}
	if (uvs == NULL)
	{
		LOG_W("Model %s contains no uv data", filepath);
	}

	// Triangles
	// How many sets there are, each set contains 3 ints
	uint32_t triangle_count = atoi(xml_get_attribute(triangles_node, "count"));
	char* triangle_cont = xml_get_content(xml_get_child(triangles_node, "p"));

	// Max number of sets expected
	// Three vertices per face
	// A set describes the index of the set of positions, normals, and uvs
	struct Face* sets = malloc(triangle_count * sizeof(struct Face) * 3);
	uint32_t set_count = 0;

	// Max number of indices expected if no reusage
	uint32_t* indices = malloc(triangle_count * sizeof(uint32_t) * 3);

	uint32_t index_count = 0;

	for (uint32_t i = 0; i < triangle_count * 3; i++)
	{
		uint32_t pos_index = atoi(triangle_cont);
		triangle_cont = strchr(triangle_cont + 1, ' ');

		uint32_t normal_index = atoi(triangle_cont);
		triangle_cont = strchr(triangle_cont + 1, ' ');

		uint32_t uv_index = atoi(triangle_cont);
		triangle_cont = strchr(triangle_cont + 1, ' ');

		int found = 0;
		for (uint32_t j = 0; j < 1; j++)
		{
			// Already existing pair found
			if (sets[j].pos_index == pos_index && sets[j].uv_index == uv_index)
			{
				// Reuse index
				indices[index_count++] = j;
				found = 1;
				LOG("Reusing index");
				break;
			}
		}
		if (!found)
		{
			// Add new unique index
			indices[index_count++] = set_count;
			sets[set_count++] = (struct Face){pos_index, normal_index, uv_index};
		}
	}

	// Create the vertices
	Vertex* vertices = malloc(set_count * sizeof(Vertex));

	// Loop through all unique faces
	for (uint32_t i = 0; i < set_count; i++)
	{
		vertices[i].position = *(vec3*)&positions[3 * sets[i].pos_index];
		vertices[i].uv = *(vec2*)&uvs[2 * sets[i].uv_index];
	}

	model->vb = vb_create(vertices, set_count);
	model->ib = ib_create(indices, index_count);
	model->index_count = index_count;
	model->vertex_count = set_count;
	xml_destroy(root);

	free(positions);
	free(uvs);
	free(normals);
	free(sets);
	free(indices);
	free(vertices);
	return model;
}

void model_bind(Model* model, VkCommandBuffer command_buffer)
{
	vb_bind(model->vb, command_buffer);
	ib_bind(model->ib, command_buffer);
}

uint32_t model_get_index_count(Model* model)
{
	return model->index_count;
}
uint32_t model_get_vertex_count(Model* model)
{
	return model->vertex_count;
}

void model_destroy(Model* model)
{
	vb_destroy(model->vb);
	ib_destroy(model->ib);
	free(model->name);
	free(model->id);
	free(model);
}