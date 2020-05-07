#ifndef COLLIDERS_H
#define COLLIDERS_H
#include "math/vec.h"
#include "transform.h"
#include <stdbool.h>

// A 'virtual' struct working as a base for colliders
// Contains function to assign for the different colliders to get points on the surface
// A collider is bound to a transform
// The colliders size will change according to the scale of the transform
typedef struct BaseCollider
{
	float (*get_distance)(struct BaseCollider*, vec3 direction);
	vec3 origin;
	// The attached transform
	const Transform* transform;
} BaseCollider;

typedef struct SphereCollider
{
	struct BaseCollider base;
	float radius;

} SphereCollider;

SphereCollider spherecollider_create(float radius, vec3 origin, Transform* transform);

// Returns true if two spheres collider_intersect
// Faster than generic collision detection and useful for pruning
bool spherecollider_intersect(const SphereCollider* a, const SphereCollider* b);

// Returns true if two colliders intersect
bool collider_intersect(const BaseCollider* a, const BaseCollider* b);
#endif