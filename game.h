#pragma once

#include "enemy.h"
#include "list.h"
#include "physics.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define ENEMY_SPEED 50.0  // units/second
#define ENEMY_INITIAL_HITPOINTS 30.0
#define ASTEROID_COLLISION_DAMAGE 20
#define ENEMY_COLLISION_DAMAGE 50
#define PLAYER_INITIAL_HITPOINTS 100.0
#define PLAYER_INITIAL_SPEED 200.0  // units/second
#define PLAYER_ACTION_SHOOT_RATE 1.0  // projectiles/second
#define PLAYER_PROJECTILE_INITIAL_SPEED 400  // units/second
#define PLAYER_PROJECTILE_TTL 5.0  // seconds
#define SIMULATION_STEP 1.0 / 15
#define TICK 1.0 // seconds
#define EVENT_QUEUE_BASE_SIZE 20

/**
 * Player action bits.
 */
enum {
	ACTION_MOVE_LEFT = 1,
	ACTION_MOVE_RIGHT = 1 << 1,
	ACTION_SHOOT = 1 << 2,
};

/**
 * Enumeration of body type bits.
 */
enum {
	BODY_TYPE_PLAYER = 1,
	BODY_TYPE_ENEMY = 1 << 1,
	BODY_TYPE_ASTEROID = 1 << 2
};

/**
 * Player.
 */
struct Player {
	float hitpoints;
	float x, y;     // position
	int actions;    // actions bitmask
	float speed;    // speed in units/second
	float shoot_cooldown;
	struct Body body;
};

/**
 * Asteroid.
 */
struct Asteroid {
	int id;
	float x, y;
	float xvel, yvel;
	float rot;
	float rot_speed;
};

/**
 * Projectile.
 */
struct Projectile {
	int id;
	float x, y;
	float xvel, yvel;
	float ttl;
};

/**
 * Game event types.
 */
enum {
	EVENT_PLAYER_HIT = 1,
	EVENT_ENEMY_HIT,
	EVENT_ASTEROID_HIT
};

/**
 * Game event.
 */
struct Event {
	int type;
	void *payload;
};

/**
 * World container.
 *
 * This struct holds all the objects which make up the game.
 */
struct World {
	struct Player player;
	struct List *asteroid_list;
	struct List *projectile_list;
	struct List *enemy_list;

	struct SimulationSystem *sim;

	struct Event *event_queue;
	size_t event_queue_size;
	size_t event_count;
};

/**
 * Create and initialize a game world.
 */
struct World*
world_new(void);

/**
 * Destroy game world.
 */
void
world_destroy(struct World *w);

/**
 * Add an anemy to the world.
 */
int
world_add_enemy(struct World *world, struct Enemy *enemy);

/**
 * Add an asteroid to the world.
 */
int
world_add_asteroid(struct World *world, struct Asteroid *ast);

/**
 * Add a projectile to the world.
 */
int
world_add_projectile(struct World *w, struct Projectile *projectile);

/**
 * Update the world by given delta time.
 */
int
world_update(struct World *world, float dt);