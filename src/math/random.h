#pragma once

#include "limits.h"
#include "types.h"
uint randi(uint32 index)
{
	index = (index << 13) ^ index;
	return ((index * (index * index * 15731 + 789221) + 1376312589) & UINT32_MAX);
}