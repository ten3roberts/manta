#pragma once
#include <stdint.h>

typedef enum
{
	RT_SHADER, RT_MODEL, RT_UNIFORMBUFFER
} ResourceType;

typedef struct
{
	ResourceType type;
	char name[256];
	uint32_t id;
} Head;