#include "xmlparser.h"
#include "log.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

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
	fread(buf, 1, size, file);

	free(buf);
	fclose(file);
}

char* xml_load(XMLNode* node, char* str)
{

}

// Creates a new empty node
XMLNode* xml_new()
{
}