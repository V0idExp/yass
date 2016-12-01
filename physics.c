#include "list.h"
#include "math.h"
#include "physics.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

struct SimulationSystem*
sim_new(void)
{
	struct SimulationSystem *sys = malloc(sizeof(struct SimulationSystem));
	if (!sys) {
		return NULL;
	}
	if (!(sys->body_list = list_new())) {
		free(sys);
		return NULL;
	}
	sys->handler_count = 0;
	return sys;
}

void
sim_destroy(struct SimulationSystem *sys)
{
	if (sys) {
		list_destroy(sys->body_list);
		free(sys);
	}
}

static inline int
check_collision(struct Body *a, struct Body *b)
{
	float dist = sqrt(powf(a->x - b->x, 2) + powf(a->y - b->y, 2));
	return dist < a->radius + b->radius;
}

static void
dispatch_collision(struct SimulationSystem *sys, struct Body *a, struct Body *b)
{
	for (size_t i = 0; i < sys->handler_count; i++) {
		struct CollisionHandler h = sys->handlers[i];
		if (h.callback != NULL &&
		    h.type_mask & a->type &&
		    h.type_mask & b->type) {
			h.callback(a, b, h.userdata);
		}
	}
}

void
sim_step(struct SimulationSystem *sys, float dt)
{
	struct ListNode *node_a = sys->body_list->head;
	while (node_a) {
		// move bodies
		struct Body *a = node_a->data;
		a->x += a->xvel * dt;
		a->y += a->yvel * dt;

		// check for collisions
		struct ListNode *node_b = sys->body_list->head;
		while (node_b) {
			struct Body *b = node_b->data;
			if (a != b &&
			    a->type & b->collision_mask &&
			    b->type & a->collision_mask &&
			    check_collision(a, b)) {
				dispatch_collision(sys, a, b);
			}
			node_b = node_b->next;
		}
		node_a = node_a->next;
	}
}

int
sim_add_body(struct SimulationSystem *sys, struct Body *body)
{
	return list_add(sys->body_list, body);
}

void
sim_remove_body(struct SimulationSystem *sys, struct Body *body)
{
	while (list_remove(sys->body_list, body, ptr_cmp));
}

int
sim_add_handler(struct SimulationSystem *sys, const struct CollisionHandler *hnd)
{
	if (sys->handler_count < MAX_HANDLERS) {
		sys->handlers[sys->handler_count++] = *hnd;
		return 1;
	}
	return 0;
}