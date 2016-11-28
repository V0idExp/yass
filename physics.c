#include "physics.h"
#include "memory.h"
#include "math.h"
#include <stdio.h>

struct SimulationSystem*
sim_new(void)
{
	struct SimulationSystem *sys = make(struct SimulationSystem);
	if (!sys) {
		return NULL;
	}
	return sys;
}

void
sim_destroy(struct SimulationSystem *sys)
{
	destroy(sys);
}

static int
check_collision(struct Body *a, struct Body *b)
{
	float dist = sqrt(powf(a->x - b->x, 2) + powf(a->y - b->y, 2));
	return dist < a->radius + b->radius;
}

static void
dispatch_collision(struct CollisionHandler handlers[], struct Body *a, struct Body *b)
{
	for (size_t i = 0; i < MAX_HANDLERS; i++) {
		if (handlers[i].callback &&
		    handlers[i].type_mask & a->type &&
		    handlers[i].type_mask & b->type) {
			    handlers[i].callback(a, b);
		    }
	}
}

void
sim_step(struct SimulationSystem *sys, float dt)
{
	for (size_t i = 0; i < MAX_BODIES; i++) {
		if (!sys->used_bodies[i]) {
			continue;
		}
		struct Body *a = &sys->bodies[i];

		for (size_t j = 0; j < MAX_BODIES; j++) {
			if (!sys->used_bodies[j] || j == i) {
				continue;
			}
			struct Body *b = &sys->bodies[j];

			if (a->type & b->collision_mask &&
			    b->type & a->collision_mask &&
			    check_collision(a, b)) {
				dispatch_collision(sys->handlers, a, b);
			}
		}
	}
}

int
sim_add_body(struct SimulationSystem *sys, const struct Body *body)
{
	// find unused body slot
	size_t i;
	for (i = 0; i < MAX_BODIES; i++) {
		if (!sys->used_bodies[i]) {
			break;
		}
	}
	if (i == MAX_BODIES) {
		return -1;
	}

	sys->bodies[i] = *body;
	sys->used_bodies[i] = 1;
	return i;
}

struct Body*
sim_get_body(struct SimulationSystem *sys, int hnd)
{
	if (sys->used_bodies[hnd]) {
		return &sys->bodies[hnd];
	}
	return NULL;
}

int
sim_add_handler(struct SimulationSystem *sys, const struct CollisionHandler *hnd)
{
	for (size_t i = 0; i < MAX_HANDLERS; i++) {
		if (sys->handlers[i].callback == NULL) {
			sys->handlers[i] = *hnd;
			return i;
		}
	}
	return -1;
}