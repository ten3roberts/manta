#include "graphics/model.h"
#include "utils.h"
#include "graphics/vertexbuffer.h"
#include "graphics/indexbuffer.h"
#include "xmlparser.h"
#include "log.h"
#include "utils.h"
#include "hashtable.h"

hashtable_t* model_table = NULL;

struct Mesh
{
	// The mesh id
	char name[256];
	VertexBuffer* vb;
	IndexBuffer* ib;
	uint32_t index_count;
	uint32_t vertex_count;
	float max_distance;
	Model* model_parent;
};

struct Model
{
	// The filename without extension
	char name[256];
	Mesh** meshes;
	uint32_t mesh_count;
};

struct Face
{
	uint32_t pos_index, normal_index, uv_index;
};

void model_load_collada(const char* filepath)
{
	LOG("Loading model %s", filepath);

	XMLNode* root = xml_loadfile(filepath);
	if (root == NULL)
	{
		LOG_E("Failed to open COLLADA model file %s", filepath);
		return;
	}

	Model* model = malloc(sizeof(Model));
	model->meshes = NULL;
	model->mesh_count = 0;
	// Load the name from the filename
	get_filename(filepath, model->name, sizeof model->name);

	// Insert into table
	// Create table if it doesn't exist
	if (model_table == NULL)
	{
		model_table = hashtable_create_string();
	}
	// Insert material into tracking table after name is acquired
	if (hashtable_find(model_table, model->name) != NULL)
	{
		LOG_W("Duplicate model %s", model->name);
		model_destroy(model);
		return;
	}
	// Insert into table
	hashtable_insert(model_table, model->name, model);

	XMLNode* lib_geometries = xml_get_child(root, "library_geometries");
	XMLNode* geometries = xml_get_children(lib_geometries);

	XMLNode* xml_up_axis = xml_get_child(xml_get_child(root, "asset"), "up_axis");
	vec3 (*axis_swap)(vec3) = vec3_swap_identity;
	if (xml_up_axis)
	{
		const char* up_axis = xml_get_content(xml_up_axis);
		if (strcmp(up_axis, "Z_UP") == 0)
			axis_swap = vec3_swap_yz;
		if (strcmp(up_axis, "X_UP") == 0)
			axis_swap = vec3_swap_xy;
	}
	// Load all meshes/geometries
	while (geometries)
	{
		const char* name = xml_get_attribute(geometries, "name");
		const char* id = xml_get_attribute(geometries, "id");
		size_t id_len = strlen(id);
		XMLNode* xml_mesh = xml_get_child(geometries, "mesh");

		geometries = xml_get_next(geometries);

		// Load the sources
		XMLNode* sources = xml_get_children(xml_mesh);
		XMLNode* triangles_node = xml_get_child(xml_mesh, "triangles");

		char* source_names[3];
		source_names[0] = malloc(id_len + strlen("-positions") + 1);
		memcpy(source_names[0], id, strlen(id) + 1);
		strcat(source_names[0], "-positions");

		source_names[1] = malloc(id_len + strlen("-map-0") + 1);
		memcpy(source_names[1], id, strlen(id) + 1);
		strcat(source_names[1], "-map-0");

		source_names[2] = malloc(id_len + strlen("-normals") + 1);
		memcpy(source_names[2], id, strlen(id) + 1);
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
			LOG_W("Mesh %s:%s contains no vertex position data", model->name, name);
		}
		if (normals == NULL)
		{
			LOG_W("Mesh %s:%s contains no normal data", model->name, name);
		}
		if (uvs == NULL)
		{
			LOG_W("Mesh %s:%s contains no uv data", model->name, name);
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
			// Construct the vertex and swap axes to get the correct up axis
			vertices[i].position = axis_swap(*(vec3*)&positions[3 * sets[i].pos_index]);
			vertices[i].uv = *(vec2*)&uvs[2 * sets[i].uv_index];
		}

		Mesh* mesh = mesh_create(name, vertices, set_count, indices, index_count);
		model_add_mesh(model, mesh);

		free(positions);
		free(uvs);
		free(normals);
		free(sets);
		free(indices);
		free(vertices);
	}

	xml_destroy(root);
}

Mesh* mesh_create(const char* name, Vertex* vertices, uint32_t vertex_count, uint32_t* indices, uint32_t index_count)
{
	Mesh* mesh = malloc(sizeof(Mesh));

	snprintf(mesh->name, sizeof mesh->name, "%s", name);

	mesh->max_distance = 0;
	// Find max distance
	for (uint32_t i = 0; i < vertex_count; i++)
	{
		if (vec3_sqrmag(vertices[i].position) > mesh->max_distance * mesh->max_distance)
		{
			mesh->max_distance = vec3_mag(vertices[i].position);
		}
	}

	mesh->vb = vb_create(vertices, vertex_count);
	mesh->ib = ib_create(indices, index_count);
	mesh->index_count = index_count;
	mesh->vertex_count = vertex_count;
	mesh->model_parent = NULL;

	return mesh;
}

Model* model_get(const char* name)
{
	// No materials loaded
	if (model_table == NULL)
		return NULL;
	return hashtable_find(model_table, name);
}

Mesh* model_get_mesh(Model* model, uint32_t index)
{
	if (index >= model->mesh_count)
	{
		return NULL;
	}
	return model->meshes[index];
}

Mesh* model_find_mesh(Model* model, const char* name)
{
	for (uint32_t i = 0; i < model->mesh_count; i++)
	{
		if (strcmp(model->meshes[i]->name, name) == 0)
		{
			return model->meshes[i];
		}
	}
	return NULL;
}

// Adds a mesh to a model
void model_add_mesh(Model* model, Mesh* mesh)
{
	model->meshes = realloc(model->meshes, ++model->mesh_count * sizeof(Mesh*));
	model->meshes[model->mesh_count - 1] = mesh;
	mesh->model_parent = model;
}

void model_destroy(Model* model)
{
	// Remove from table if it exists
	hashtable_remove(model_table, model->name);
	// Last texture was removed
	if (hashtable_get_count(model_table) == 0)
	{
		hashtable_destroy(model_table);
		model_table = NULL;
	}

	for (uint32_t i = 0; i < model->mesh_count; i++)
	{
		mesh_destroy(model->meshes[i]);
	}
	model->mesh_count = 0;
	free(model);
}

void model_destroy_all()
{
	Model* model = NULL;
	while (model_table && (model = hashtable_pop(model_table)))
	{
		model_destroy(model);
	}
}

void mesh_bind(Mesh* mesh, CommandBuffer* commandbuffer)
{
	vb_bind(mesh->vb, commandbuffer);
	ib_bind(mesh->ib, commandbuffer);
}

float mesh_max_distance(Mesh* mesh)
{
	return mesh->max_distance;
}

uint32_t mesh_get_index_count(Mesh* mesh)
{
	return mesh->index_count;
}
uint32_t mesh_get_vertex_count(Mesh* mesh)
{
	return mesh->vertex_count;
}

Mesh* mesh_find(const char* name)
{
	char* delimiter = strchr(name, ':');
	char modelname[256];
	char meshname[256];
	size_t len = sizeof modelname;
	len = delimiter ? (delimiter - name + 1) : sizeof modelname;
	len = len > sizeof modelname ? sizeof modelname : len;

	snprintf(modelname, len, "%s", name);

	Model* model = model_get(modelname);
	if (model == NULL)
	{
		LOG_E("Unknown model %s", modelname);
		return NULL;
	}
	// Get first mesh
	if (delimiter == NULL)
	{
		return model_get_mesh(model, 0);
	}

	snprintf(meshname, sizeof meshname, "%s", delimiter + 1);

	Mesh* mesh = model_find_mesh(model, meshname);
	if (mesh == NULL)
	{
		LOG_E("Unknown mesh %s:%s", modelname, meshname);
	}
	return mesh;
}

void mesh_destroy(Mesh* mesh)
{
	vb_destroy(mesh->vb);
	ib_destroy(mesh->ib);
	free(mesh);
}