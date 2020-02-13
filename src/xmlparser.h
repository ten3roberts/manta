typedef struct XMLNode
{
	struct XMLNode* parent;
	char* content;
	size_t child_count;
	struct XMLNode* children;
} XMLNode;

// Loads an xml file recusively from the disk
XMLNode* xml_loadfile(const char* filepath);

char* xml_load(XMLNode* node, char* str);