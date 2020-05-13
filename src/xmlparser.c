#include "xmlparser.h"
#include "log.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "magpie.h"

#include "mempool.h"

static mempool_t* node_pool = NULL;

struct attribute_t
{
	// key\0value
	char *key, *value;
};

struct XMLNode
{
	XMLNode* parent;
	XMLNode* next;
	char* tag;
	char* content;
	// The attributes for the tag
	uint16_t attribute_count;
	struct attribute_t* attributes;
	// Linked list to children
	XMLNode* children;
};

// Pool allocated a new node
static XMLNode* xml_node_new()
{
	// Allocate pool for the first time
	if (node_pool == NULL)
	{
		node_pool = mempool_create(sizeof(XMLNode), 16);
	}
	return mempool_alloc(node_pool);
}

XMLNode* xml_loadfile(const char* filepath)
{
	FILE* file = fopen(filepath, "r");
	if (file == NULL)
	{
		LOG_E("Failed to open file %s", filepath);
		return NULL;
	}
	char* buf = NULL;
	size_t size;
	fseek(file, 0L, SEEK_END);
	size = ftell(file);
	buf = malloc(size);
	fseek(file, 0L, SEEK_SET);
	// Loads the xml file into memory, taking escaping into account
	fread(buf, 1, size, file);

	// Loads the root node
	XMLNode* root = xml_node_new();
	root->parent = NULL;
	char* body = buf;
	body = xml_load(root, buf);
	if (body == NULL)
	{
		LOG_E("Failed to read xml file %s", filepath);
	}
	free(buf);
	fclose(file);
	return root;
}

char* xml_load(XMLNode* node, char* str)
{
	node->attributes = NULL;
	node->children = NULL;
	node->next = NULL;
	node->content = NULL;
	node->parent = NULL;
	node->tag = NULL;
	// Step pointer so that the tag opening is at pos 0
	do
	{
		str = strchr(str, '<');

		if (str == NULL)
			return NULL;
		if (str[1] == '?')
		{
			str++;
		}
	} while (*str == '?');
	if (str[1] == '/')
		return NULL;
	// Close tag is the distance to the > after the <
	// Indicates the end of the opening tag
	size_t tag_end = strchr(str, '>') - str;

	// Copy the tag into node
	node->tag = malloc(tag_end);
	char* tmp_attr[2];
	tmp_attr[0] = malloc(tag_end);
	tmp_attr[1] = malloc(tag_end);

	// Only get first space separated part outside comma
	int in_quotes = 0;
	int in_attributes = 0;
	int after_equal = 0;
	node->attributes = NULL;
	node->attribute_count = 0;

	// i : iterator for string
	// j : iterator for dst
	for (uint32_t i = 1, j = 0; i < tag_end; i++)
	{
		// Next attribute
		// Either on space otuside quotes or end of loop
		if ((str[i] == ' ' && !in_quotes) || (i == tag_end - 1 && in_attributes))
		{
			if (in_attributes)
			{
				// Copy val into map
				// Null terminate val
				tmp_attr[1][j] = '\0';
				xml_set_attribute(node, tmp_attr[0], tmp_attr[1]);

				after_equal = 0;
			}
			else
			{
				node->tag[j] = '\0';
			}

			j = 0;
			in_attributes = 1;
			continue;
		}
		// Toggle quotes
		// Skip over writing
		if (str[i] == '"')
		{
			in_quotes = !in_quotes;
			continue;
		}

		// Equal sign on pair
		if (str[i] == '=' && !in_quotes)
		{
			// Null terminate key
			tmp_attr[after_equal][j] = '\0';
			after_equal = 1;
			j = 0;
			continue;
		}

		// Write
		if (!in_attributes)
		{
			node->tag[j] = str[i];
			// Null terminate after last character
			if (i == tag_end - 1)
				node->tag[j + 1] = '\0';
		}
		// Write to attributes
		else
		{
			// Don't include quotes
			tmp_attr[after_equal][j] = str[i];
		}
		// If loop body reaches bottom, increment j
		j++;
	}
	free(tmp_attr[0]);
	free(tmp_attr[1]);
	// Empty node
	if (str[tag_end - 1] == '/')
	{
		node->content = NULL;
		node->children = NULL;
		// Remove slash
		node->tag[tag_end - 2] = '\0';
		return str + tag_end + 1;
	}
	// Move str to the start of the body
	str += tag_end + 1;
	char* tmp_tag = malloc(tag_end + 3);
	tmp_tag[0] = '<';
	tmp_tag[1] = '/';
	memcpy(tmp_tag + 2, node->tag, tag_end);
	tmp_tag[tag_end + 1] = '>';
	tmp_tag[tag_end + 2] = '\0';

	// Find the closing match of the tag
	char* tmp = strstr(str, tmp_tag);
	free(tmp_tag);
	// No closing part was found
	if (tmp == NULL)
	{
		free(node->tag);
		return NULL;
	}
	size_t tag_close = tmp - str;

	// Node is valid

	// Load children
	while (1)
	{

		XMLNode* new_node = xml_node_new();
		char* buf = xml_load(new_node, str);
		new_node->parent = node;
		if (buf == NULL)
		{
			mempool_free(node_pool, new_node);
			break;
		}
		tag_close -= buf - str;
		str = buf;

		// Insert into linked list
		XMLNode* it = node->children;
		XMLNode* prev = NULL;
		// No children yet
		if (it == NULL)
		{
			node->children = new_node;
		}
		else
		{ // Follow linked list until name sorting lower is found
			while (1)
			{
				// Insert before
				if (strcmp(new_node->tag, it->tag) < 0)
				{
					if (prev == NULL)
					{
						new_node->next = it;
						node->children = new_node;
					}
					else
					{
						prev->next = new_node;
						new_node->next = it;
					}
					break;
				}
				// Insert after end
				if (it->next == NULL)
				{
					it->next = new_node;
					new_node->next = NULL;
					break;
				}
				prev = it;
				it = it->next;
			}
		}
	}

	node->content = malloc(tag_close + 1);
	memcpy(node->content, str, tag_close + 1);
	node->content[tag_close] = '\0';

	str += tag_close + strlen(node->tag) + 3;
	return str;
}

void xml_save_internal(XMLNode* node, FILE* file)
{
	// Tag and attributes
	fputc('<', file);
	fputs(node->tag, file);
	for (uint32_t i = 0; i < node->attribute_count; i++)
	{
		fputs(node->attributes[i].key, file);
		fputc('=', file);
		fputc('"', file);
		fputs(node->attributes[i].value, file);
		fputc('"', file);
		if (i < node->attribute_count - 1)
			fputc(' ', file);
	}
	fputc('>', file);
	if (node->content)
		fputs(node->content, file);

	// Children
	XMLNode* it = node->children;
	while (it != NULL)
	{
		xml_save_internal(it, file);

		it = it->next;
	}

	// Closing tag
	fputc('<', file);
	fputc('/', file);
	fputs(node->tag, file);
	fputc('>', file);
}

void xml_savefile(XMLNode* root, const char* filepath)
{
	FILE* file = NULL;
	file = fopen(filepath, "w");
	if (file == NULL)
		return;
	xml_save_internal(root, file);
	fclose(file);
}

XMLNode* xml_create(char* tag, char* content)
{
	XMLNode* node = xml_node_new();
	if (node == NULL)
		return NULL;
	if (tag)
	{
		node->tag = malloc(strlen(tag) + 1);
		if (node->tag == NULL)
			return NULL;
		strcpy(node->tag, tag);
	}
	else
	{
		node->tag = NULL;
	}
	if (content)
	{
		node->content = malloc(strlen(content) + 1);
		if (node->content == NULL)
			return NULL;
		strcpy(node->content, content);
	}
	else
	{
		node->content = NULL;
	}
	// NO children
	node->children = NULL;
	node->attributes = NULL;
	node->attribute_count = 0;
	node->parent = NULL;
	return node;
}

XMLNode* xml_get_next(XMLNode* node)
{
	return node->next;
}

XMLNode* xml_get_children(XMLNode* node)
{
	return node->children;
}

XMLNode* xml_get_child(XMLNode* node, char* node_tag)
{
	XMLNode* it = node->children;
	while (it != NULL)
	{
		if (strcmp(it->tag, node_tag) == 0)
			return it;
		it = it->next;
	}
	return NULL;
}

void xml_add_child(XMLNode* node, XMLNode* child)
{
	XMLNode* it = node->children;

	// Follow linked list until name sorting lower is found
	while (1)
	{
		if (strcmp(node->tag, it->tag) > 0)
		{
			XMLNode* tmp = it->next;
			it->next = child;
			child->next = tmp;
			break;
		}
	}
}

char* xml_get_tag(XMLNode* node)
{
	return node->tag;
}

void xml_set_tag(XMLNode* node, char* new_tag)
{
	if (node->tag)
		free(node->tag);
	node->tag = malloc(strlen(new_tag + 1));
	strcpy(node->tag, new_tag);
}

char* xml_get_attribute(XMLNode* node, char* key)
{
	for (uint32_t i = 0; i < node->attribute_count; i++)
	{
		if (strcmp(node->attributes[i].key, key) == 0)
		{
			return node->attributes[i].value;
		}
	}
	return NULL;
}

void xml_set_attribute(XMLNode* node, char* key, char* val)
{
	// Look if it already exists
	for (uint32_t i = 0; i < node->attribute_count; i++)
	{
		if (strcmp(node->attributes[i].key, key) == 0)
		{
			free(node->attributes[i].key);
			struct attribute_t* attribute = &node->attributes[i];
			size_t l1 = strlen(key);
			size_t l2 = strlen(val);
			char* pair = malloc(l1 + l2 + 2);
			attribute->key = pair;
			attribute->value = pair + l1 + 1;
			pair[l1] = '\0';
			pair[l2 + 1] = '\0';
			memcpy(pair, key, l1);
			memcpy(pair + l1 + 1, val, l2);
			return;
		}
	}

	node->attributes = realloc(node->attributes, (++node->attribute_count) * sizeof(struct attribute_t));
	struct attribute_t* attribute = &node->attributes[node->attribute_count - 1];
	size_t l1 = strlen(key);
	size_t l2 = strlen(val);
	char* pair = malloc(l1 + l2 + 2);
	attribute->key = pair;
	attribute->value = pair + l1 + 1;
	pair[l1] = '\0';
	pair[l1 + l2 + 1] = '\0';
	memcpy(pair, key, l1);
	memcpy(pair + l1 + 1, val, l2);
}

char* xml_get_content(XMLNode* node)
{
	return node->content;
}
void xml_set_content(XMLNode* node, char* new_content)
{
	if (node->content)
		free(node->content);
	node->content = malloc(strlen(new_content + 1));
	strcpy(node->content, new_content);
}

void xml_destroy(XMLNode* node)
{
	XMLNode* it = node->children;
	while (it != NULL)
	{
		XMLNode* next = it->next;
		xml_destroy(it);
		it = next;
	}

	for (uint32_t i = 0; i < node->attribute_count; i++)
	{
		free(node->attributes[i].key);
	}
	free(node->attributes);
	free(node->tag);
	if (node->content)
		free(node->content);

	mempool_free(node_pool, node);

	// Destroy pool if it is empty
	if (mempool_get_count(node_pool) == 0)
	{
		mempool_destroy(node_pool);
		node_pool = NULL;
	}
}