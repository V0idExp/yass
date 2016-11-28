#include <stdlib.h>
#include "game.h"
#include "matlib.h"
#include "memory.h"

struct World*
world_new(void)
{
	struct World *w = make(struct World);
	if (!w) {
		return NULL;
	}

	// initialize player
	w->player.y = SCREEN_HEIGHT / 2 - 50;
	w->player.speed = PLAYER_INITIAL_SPEED;

	// add enemy
	w->enemies[0].speed = ENEMY_SPEED;
	w->enemies[0].y = -SCREEN_HEIGHT / 2 + 50;

	return w;
}

void
world_destroy(struct World *w)
{
	if (w) {
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

	// update enemies
	for (int i = 0; i < MAX_ENEMIES; i++) {
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