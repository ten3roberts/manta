#include <stdint.h>

typedef struct XMLNode XMLNode;

// Loads an xml file recusively from the disk
XMLNode* xml_loadfile(const char* filepath);

// Loads an xml node and all children recursively from a string
// Returns a pointer to the same string but move to the end of the tag it read
// Returns NULL when str does not contain a tag
// Using the function again will thus read the next tag
char* xml_load(XMLNode* node, char* str);

void xml_savefile(XMLNode* root, const char* filepath);

// Creates and returns a new node
// Copies tag and content if provided
// tag and content can be NULL
XMLNode* xml_create(char* tag, char* content);

// Returns the next child from the node's parent
XMLNode* xml_get_next(XMLNode* node);

// Returns the first item in the linked list of children
XMLNode* xml_get_children(XMLNode* node);

XMLNode* xml_get_child(XMLNode* node, char* node_tag);

// Adds a child to node
// Will be sorted alphabetically
// Duplicate tag names are allowed and are after one another in the list
void xml_add_child(XMLNode* node, XMLNode* child);

// Returns the pointer to the internal tag of the node
// Changing this changes the node's tag
char* xml_get_tag(XMLNode* node);

// Copies a new string into content and deletes the old one
void xml_set_tag(XMLNode* node, char* new_tag);

char* xml_get_attribute(XMLNode* node, const char* key);
// Sets or updates a nodes attribute
void xml_set_attribute(XMLNode* node, char* key, char* val);

// Returns the pointer to the internal content of the node
// Changing this changes the node's content
char* xml_get_content(XMLNode* node);

// Copies a new string into content and deletes the old one
void xml_set_content(XMLNode* node, char* new_content);

// Destroys and frees all memory of a xml structure recursively
// Note, destroy should only be called on the root node
void xml_destroy(XMLNode* node);