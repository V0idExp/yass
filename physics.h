#pragma once

#include <stddef.h>

#define MAX_BODIES 20
#define MAX_HANDLERS 10

struct Body {
	float x, y;
	float radius;
	int type;
	int collision_mask;
	void *userdata;
};

typedef int (*CollisionCallback)(struct Body *a, struct Body *b, void *userdata);

struct CollisionHandler {
	CollisionCallback callback;
	int type_mask;
	void *userdata;
};

struct SimulationSystem {
	struct List *body_list;
	struct CollisionHandler handlers[MAX_HANDLERS];
	size_t handler_count;
};

struct SimulationSystem*
sim_new(void);

void
sim_destroy(struct SimulationSystem *sys);

void
sim_step(struct SimulationSystem *sys, float dt);

int
sim_add_body(struct SimulationSystem *sys, struct Body *body);

void
sim_remove_body(struct SimulationSystem *sys, struct Body *body);

int
sim_add_handler(struct SimulationSystem *sys, const struct CollisionHandler *c);
