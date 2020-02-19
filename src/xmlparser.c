#include "xmlparser.h"
#include "log.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "strmap.h"

struct XMLNode
{
	struct XMLNode* parent;
	char* tag;
	char* content;
	// The attributes for the tag
	strmap* attributes;
	strmap* children;
};

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
	size_t i = 0;
	// Loads the xml file into memory, taking escaping into account
	fread(buf, 1, size, file);

	// Loads the root node
	XMLNode* root = malloc(sizeof(XMLNode));
	root->parent = NULL;
	char* body = buf;
	body = xml_load(root, buf);
	free(buf);
	fclose(file);
	return root;
}

char* xml_load(XMLNode* node, char* str)
{
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
	node->attributes = strmap_create(node->attributes);
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
				strmap_insert(node->attributes, tmp_attr[0], tmp_attr[1], strlen(tmp_attr[1]) + 1);
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
	if (node->tag[tag_end - 2] == '/')
	{
		node->content = NULL;
		node->children = NULL;
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

	node->content = malloc(tag_close + 1);
	memcpy(node->content, str, tag_close + 1);
	node->content[tag_close] = '\0';

	// Load children
	node->children = strmap_create();
	while (1)
	{
		XMLNode* new_node = malloc(sizeof(XMLNode));
		new_node->parent = node;
		char* buf = xml_load(new_node, str);
		if (buf == NULL)
		{
			free(new_node);
			break;
		}
		str = buf;
		strmap_insert(node->children, new_node->tag, &new_node, sizeof(XMLNode*));
	}

	str += tag_close + tag_end;
	return str;
}

void xml_save_internal(XMLNode* node, FILE* file)
{
	// Tag and attributes
	fputc('<', file);
	fputs(node->tag, file);
	uint32_t attr_count = strmap_count(node->attributes);
	for (uint32_t i = 0; i < attr_count; i++)
	{
		fputs(strmap_index(node->attributes, i)->key, file);
		fputc('=', file);
		fputc('"', file);
		fputs(strmap_index(node->attributes, i)->data, file);
		fputc('"', file);
		if (i < attr_count - 1)
			fputc(' ', file);
	}
	fputc('>', file);
	fputs(node->content, file);
	// Children
	uint32_t child_count = strmap_count(node->children);
	for (uint32_t i = 0; i < child_count; i++)
	{
		xml_save_internal(*(XMLNode**)strmap_index(node->children, i)->data, file);
	}

	// Closing tag
	fputc('/', file);
	fputc('<', file);
	fputs(node->tag, file);
	fputc('>', file);
}

void xml_savefile(XMLNode* root, const char* filepath)
{
	FILE* file = NULL;
	file = fopen(filepath, "w");
	if (file == NULL)
		return;
	xml_save_internal(root, stdout);
	fclose(file);
}


XMLNode* xml_create(char* tag, char* content)
{
	XMLNode* node = malloc(sizeof(XMLNode));
	if (node == NULL) return NULL;
	if (tag)
	{
		node->tag = malloc(strlen(tag) + 1);
		if (node->tag == NULL) return NULL;
		strcpy(node->tag, tag);
	}
	else
	{
		node->tag = NULL;
	}
	if (content)
	{
		node->content = malloc(strlen(content) + 1);
		if (node->content == NULL) return NULL;
		strcpy(node->content, content);
	}
	else
	{
		node->content = NULL;
	}
	node->children = strmap_create();
	node->attributes = strmap_create();
	node->parent = NULL;
	return node;
}

XMLNode* xml_get_child(XMLNode* node, char* node_name)
{
	return *(XMLNode**)strmap_find(node->children, node_name);
}

void xml_add_child(XMLNode* node, XMLNode* child)
{
	strmap_insert(node->children, child->tag, &node, sizeof(XMLNode*));
}

char* xml_get_attribute(XMLNode* node, char* key)
{
	return (char*)strmap_find(node->attributes, key);
}

void xml_set_attribute(XMLNode* node, char* key, char* val)
{
	strmap_insert(node->attributes, key, val, strlen(val) + 1);
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
	if (node->children)
	{
		uint32_t size = strmap_count(node->children);
		// Destroy the children
		for (uint32_t i = 0; i < size; i++)
		{
			xml_destroy(*(XMLNode**)strmap_index(node->children, i)->data);
		}

		// Free the children
		strmap_destroy(node->children);
	}
	strmap_destroy(node->attributes);
	free(node->tag);
	free(node->content);
	free(node);
}