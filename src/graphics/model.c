#include "model.h"
#include "graphics/vertexbuffer.h"
#include "graphics/indexbuffer.h"
#include "xmlparser.h"
#include "log.h"
#include <utils.h>

struct Model
{
	char* name;
	VertexBuffer* vb;
	IndexBuffer* ib;
};

Model* model_load_collada(const char* filepath)
{
	Model* model = malloc(sizeof(Model));
	XMLNode* root = xml_loadfile(filepath);
	if(root == NULL)
	{
		LOG_E("Failed to open COLLADA model file %s", filepath);
		return NULL;
	}
	XMLNode* mesh = xml_get_child(root, "library_geometries");
	mesh = xml_get_children(mesh);
	model->name = string_dup(xml_get_attribute(mesh, "name"));

	xml_destroy(root);
	return model;
}