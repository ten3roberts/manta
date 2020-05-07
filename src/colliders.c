#include "colliders.h"

// Distance functions
// Returns the distance to the surface in a gives direction of a sphere
// Note, argument must be a valid sphere collider
float spherecollider_distance(struct BaseCollider* sphere, vec3 direction)
{
	// Like a virtual class
	vec3 pos = vec3_add(sphere->transform->position, sphere->origin);
	return vec3_dot(pos, direction) + ((SphereCollider*)sphere)->radius * vec3_largest(sphere->transform->scale);
}

SphereCollider spherecollider_create(float radius, vec3 origin, Transform* transform)
{
	SphereCollider sphere;
	sphere.base			  = (BaseCollider){.get_distance = spherecollider_distance};
	sphere.base.origin	  = origin;
	sphere.base.transform = transform;
	sphere.radius		  = radius;
	return sphere;
}

// Returns true if two spheres collider_intersect
bool spherecollider_intersect(const SphereCollider* a, const SphereCollider* b)
{
	vec3 mid_a = a->base.origin;
	if (a->base.transform)
		mid_a = vec3_add(mid_a, a->base.transform->position);

	vec3 mid_b = b->base.origin;
	if (b->base.transform)
		mid_b = vec3_add(mid_b, b->base.transform->position);

	float radii = vec3_largest(a->base.transform->scale) * a->radius + b->radius * vec3_largest(b->base.transform->scale);

	return (vec3_sqrdistance(mid_a, mid_b) < radii * radii);
}

// Returns true if two colliders intersect
bool collider_intersect(const BaseCollider* a, const BaseCollider* b);
