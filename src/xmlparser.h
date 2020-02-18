#include <stdint.h>

typedef struct XMLNode XMLNode;

// Loads an xml file recusively from the disk
XMLNode* xml_loadfile(const char* filepath);

// Loads an xml node and all children recursively from a string
// Returns a pointer to the same string but move to the end of the tag it read
// Returns NULL when str does not contain a tag
// Using the function again will thus read the next tag
char* xml_load(XMLNode* node, char* str);

XMLNode* xml_get_child(XMLNode* parent, char* node_name);
char* xml_get_attribute(XMLNode* node, char* attribute_name);
char* xml_get_content(XMLNode* node);
void xml_set_content(XMLNode* node, char* new_content);
// Destroys and frees all memory of a xml structure recursively
void xml_destroy(XMLNode* node);