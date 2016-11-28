#include <stdlib.h>
#include <stdio.h>
#include "game.h"
#include "matlib.h"
#include "memory.h"

/**
 * Enumeration of body type bits.
 */
enum {
	BODY_TYPE_PLAYER = 1,
	BODY_TYPE_ENEMY = 1 << 1,
	BODY_TYPE_ASTEROID = 1 << 2
};

static int
handle_player_collision(struct Body *a, struct Body *b)
{
	struct Player *p = a->type == BODY_TYPE_PLAYER ? a->userdata : b->userdata;
	p->collided = 1;
	return 1;
}

struct World*
world_new(void)
{
	struct World *w = make(struct World);
	if (!w) {
		return NULL;
	}

	// initialize simulation system and register collision callbacks
	w->sim = sim_new();
	if (!w->sim) {
		world_destroy(w);
		return NULL;
	}
	struct CollisionHandler handlers[] = {
		{ handle_player_collision, BODY_TYPE_PLAYER | BODY_TYPE_ENEMY },
		{ handle_player_collision, BODY_TYPE_PLAYER | BODY_TYPE_ASTEROID },
		{ NULL }
	};
	for (int i = 0; handlers[i].callback; i++) {
		if (sim_add_handler(w->sim, &handlers[i]) < 0) {
			world_destroy(w);
			return NULL;
		}
	}

	// initialize player
	w->player.y = SCREEN_HEIGHT / 2 - 50;
	w->player.speed = PLAYER_INITIAL_SPEED;
	struct Body player_body = {
		.x = w->player.x,
		.y = w->player.y,
		.radius = 40,
		.type = BODY_TYPE_PLAYER,
		.collision_mask = BODY_TYPE_ENEMY | BODY_TYPE_ASTEROID,
		.userdata = &w->player
	};
	w->player.body_hnd = sim_add_body(w->sim, &player_body);
	if (w->player.body_hnd < 0) {
		world_destroy(w);
		return NULL;
	}

	return w;
}

void
world_destroy(struct World *w)
{
	if (w) {
		sim_destroy(w->sim);
		destroy(w);
	}
}

int
world_add_asteroid(struct World *world, const struct Asteroid *ast)
{
	if (world->asteroid_count < MAX_ASTEROIDS) {
		int i = world->asteroid_count++;
		world->asteroids[i] = *ast;
		return i;
	}
	return -1;
}

int
world_add_enemy(struct World *world, const struct Enemy *enemy)
{
	if (world->enemy_count < MAX_ENEMIES) {
		int i = world->enemy_count++;
		world->enemies[i] = *enemy;

		struct Body body = {
			.x = enemy->x,
			.y = enemy->y,
			.radius = 40,
			.type = BODY_TYPE_ENEMY,
			.collision_mask = BODY_TYPE_PLAYER,
			.userdata = &world->enemies[i]
		};
		world->enemies[i].body_hnd = sim_add_body(
			world->sim,
			&body
		);
		if (world->enemies[i].body_hnd < 0) {
			return -1;
		}

		return i;
	}
	return -1;
}

void
world_add_projectile(struct World *w, const struct Projectile *projectile)
{
	size_t oldest = 0;
	float oldest_ttl = PLAYER_PROJECTILE_TTL;
	for (size_t i = 0; i < MAX_PROJECTILES; i++) {
		if (w->projectiles[i].ttl <= 0) {
			w->projectiles[i] = *projectile;
			return;
		} else if (w->projectiles[i].ttl < oldest_ttl) {
			oldest_ttl = w->projectiles[i].ttl;
			oldest = i;
		}
	}
	w->projectiles[oldest] = *projectile;
}

int
world_update(struct World *world, float dt)
{
	// update physics
	static float sim_acc = 0;
	sim_acc += dt;
	while (sim_acc >= SIMULATION_STEP) {
		sim_step(world->sim, SIMULATION_STEP);
		sim_acc -= SIMULATION_STEP;
	}

	// check game conditions
	if (world->player.collided) {
		printf("you're dead\n");
		return 0;
	}

	// update asteroids
	for (int i = 0; i < world->asteroid_count; i++) {
		struct Asteroid *ast = &world->asteroids[i];
		ast->x += ast->xvel * dt;
		ast->y += ast->yvel * dt;
		ast->rot += ast->rot_speed * dt;
		if (ast->rot >= M_PI * 2) {
			ast->rot -= M_PI * 2;
		}
	}

	// update projectiles
	for (int i = 0; i < MAX_PROJECTILES; i++) {
		struct Projectile *prj = &world->projectiles[i];
		if (prj->ttl > 0) {
			prj->x += prj->xvel * dt;
			prj->y += prj->yvel * dt;
			prj->ttl -= dt;
		}
	}

	// update player position
	float distance = dt * world->player.speed;
	if (world->player.actions & ACTION_MOVE_LEFT) {
		world->player.x -= distance;
	}
	if (world->player.actions & ACTION_MOVE_RIGHT) {
		world->player.x += distance;
	}
	if (world->player.actions & ACTION_MOVE_UP) {
		world->player.y -= distance;
	}
	if (world->player.actions & ACTION_MOVE_DOWN) {
		world->player.y += distance;
	}

	// update player body position
	struct Body *player_body = sim_get_body(
		world->sim,
		world->player.body_hnd
	);
	player_body->x = world->player.x;
	player_body->y = world->player.y;

	// update enemies
	for (int i = 0; i < world->enemy_count; i++) {
		struct Enemy *enemy = &world->enemies[i];

		// compute direction to target (player)
		Vec target = vec(world->player.x, world->player.y, 0, 0);
		Vec pos = vec(enemy->x, enemy->y, 0, 0);
		Vec dir;
		vec_sub(&target, &pos, &dir);
		vec_norm(&dir);

		// compute desired velocity vector
		Vec vel;
		vec_mulf(&dir, enemy->speed, &vel);

		// compute steering vector
		Vec steer, curr_vel = vec(enemy->xvel, enemy->yvel, 0, 0);
		vec_sub(&vel, &curr_vel, &steer);
		vec_clamp(&steer, 0.5);

		// compute new velocity vector
		vec_add(&curr_vel, &steer, &vel);
		vec_clamp(&vel, enemy->speed);
		enemy->xvel = vel.data[0];
		enemy->yvel = vel.data[1];

		// rotate the enemy to match direction
		enemy->rot = M_PI / 2 - atan2(vel.data[1], vel.data[0]);

		// update position
		enemy->x += vel.data[0] * dt;
		enemy->y += vel.data[1] * dt;

		struct Body *body = sim_get_body(world->sim, enemy->body_hnd);
		body->x = enemy->x;
		body->y = enemy->y;
	}

	// handle shooting
	world->player.shoot_cooldown -= dt;
	if (world->player.actions & ACTION_SHOOT &&
	    world->player.shoot_cooldown <= 0) {
		// reset cooldown timer
		world->player.shoot_cooldown = 1.0 / PLAYER_ACTION_SHOOT_RATE;
		// shoot
		struct Projectile p = {
			.x = world->player.x,
			.y = world->player.y,
			.xvel = 0,
			.yvel = -PLAYER_PROJECTILE_INITIAL_SPEED,
			.ttl = PLAYER_PROJECTILE_TTL
		};
		world_add_projectile(world, &p);
	}

	return 1;
}