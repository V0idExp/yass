#pragma once

#include "physics.h"

/**
 * Enemy.
 */
struct Enemy {
	float x, y;
	float hitpoints;
	struct Body body;
};

struct Enemy*
enemy_new(float x, float y);

void
enemy_destroy(struct Enemy *enemy);