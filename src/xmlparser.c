#include "xmlparser.h"
#include "log.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "strmap.h"

struct XMLNode
{
	struct XMLNode* parent;
	char* tag;
	char* content;
	uint32_t child_count;
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
	while (1)
	{
		char c = fgetc(file);
		if (feof(file))
		{
			buf[i] = '\0';
			break;
		}

		buf[i] = c;
		// Control character escape codes
		if (strncmp(buf + i - 2, "&lt", 2) == 0)
		{
			i -= 2;
			buf[i] = '<';
		}
		else if (strncmp(buf + i - 2, "&gt", 2) == 0)
		{
			i -= 2;
			buf[i] = '>';
		}
		else if (strncmp(buf + i - 3, "&amp", 3) == 0)
		{
			i -= 3;
			buf[i] = '&';
		}
		else if (strncmp(buf + i - 4, "&apos", 4) == 0)
		{
			i -= 4;
			buf[i] = '\'';
		}
		else if (strncmp(buf + i - 4, "&quot", 4) == 0)
		{
			i -= 4;
			buf[i] = '\"';
		}
		i++;
	}

	// Loads the root node
	XMLNode* root = malloc(sizeof(XMLNode));
	char* body = buf;
	body = xml_load(root, buf);
	free(buf);
	fclose(file);
	return root;
}

char* xml_load(XMLNode* node, char* str)
{
	// Step pointer so that the tag opening is at pos 0
	do {
		str = strchr(str, '<');

		if (str == NULL)
			return;
		if (str[1] == '?')
		{
			str++;
		}
	} while (*str == '?');

	// Close tag is the distance to the > after the <
	// Indicates the end of the opening tag
	size_t tag_end = strchr(str, '>') - str;


	// Copy the tag into node
	node->tag = malloc(tag_end);
	memcpy(node->tag, str + 1, tag_end - 1);
	node->tag[tag_end - 1] = '\0';
	LOG("Tag : %s", node->tag);

	// Move str to the start of the body
	str += tag_end;
	char* tmp_tag = malloc(tag_end + 3);
	tmp_tag[0] = '<';
	tmp_tag[1] = '/';
	memcpy(tmp_tag + 2, node->tag, tag_end);
	tmp_tag[tag_end + 1] = '>';
	tmp_tag[tag_end + 2] = '\0';


	size_t tag_close = strstr(str, tmp_tag) - str;
	free(tmp_tag);

	node->content = malloc(tag_close);
	memcpy(node->content, str, tag_close - 1);
	node->content[tag_close - 1] = '\0';

	node->children = strmap_create();
	return str;
}

void xml_destroy(XMLNode* node)
{
	uint32_t size = strmap_count(node->children);

	// Destroy the children
	for (uint32_t i = 0; i < size; i++)
	{
		xml_destroy((XMLNode*)strmap_index(node->children, i)->data);
	}

	// Free the children
	strmap_destroy(node->children);
	free(node->tag);
	free(node->content);
	free(node);
}