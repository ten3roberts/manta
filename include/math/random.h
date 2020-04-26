#ifndef RANDOM_H
#define RANDOM_H

#include "limits.h"
#include <stdint.h>
uint32_t randi(uint32_t index)
{
	index = (index << 13) ^ index;
	return ((index * (index * index * 15731 + 789221) + 1376312589) & UINT32_MAX);
}
#endif