#pragma once

#include "list.h"
#include "physics.h"

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define SIMULATION_STEP 1.0 / 30
#define TICK 1.0 // seconds
#define EVENT_QUEUE_BASE_SIZE 20
#define SCROLL_SPEED 30.0 // units / second

#define ENEMY_INITIAL_HITPOINTS 30.0
#define ENEMY_TTL 5.0 // seconds
#define ENEMY_COLLISION_DAMAGE 50

#define ASTEROID_TTL 20.0 // seconds
#define ASTEROID_COLLISION_DAMAGE 20

#define PLAYER_INITIAL_HITPOINTS 100.0
#define PLAYER_INITIAL_DAMAGE 10.0
#define PLAYER_INITIAL_SPEED 200.0  // units/second
#define PLAYER_ACTION_SHOOT_RATE 2.0  // projectiles/second
#define PLAYER_PROJECTILE_INITIAL_SPEED 400  // units/second
#define PLAYER_PROJECTILE_TTL 5.0  // seconds

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
	BODY_TYPE_ASTEROID = 1 << 2,
	BODY_TYPE_PROJECTILE = 1 << 3,
};

/**
 * Game event types.
 */
enum {
	EVENT_PLAYER_HIT = 1,
	EVENT_ENEMY_HIT,
	EVENT_PLAYER_COLLISION,
};

/**
 * Player.
 */
struct Player {
	float x, y;
	struct Body body;
	float hitpoints;
	int actions;
	float speed;
	float shoot_cooldown;
};

/**
 * Enemy.
 */
struct Enemy {
	float x, y;
	struct Body body;
	float hitpoints;
	float ttl;
};

/**
 * Asteroid.
 */
struct Asteroid {
	float x, y;
	struct Body body;
	float rot;
	float rot_speed;
	float ttl;
};

/**
 * Projectile.
 */
struct Projectile {
	float x, y;
	struct Body body;
	float ttl;
};

/**
 * Game event.
 */
struct Event {
	int type;
	union {
		struct CollisionEvent {
			struct Body *first;
			struct Body *second;
		} collision;
		struct HitEvent {
			void *target;
			struct Projectile *projectile;
		} hit;
	};
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
 *
 * NOTE: This function takes the ownership of the object.
 */
int
world_add_enemy(struct World *world, struct Enemy *enemy);

/**
 * Add an asteroid to the world.
 *
 * NOTE: This function takes the ownership of the object.
 */
int
world_add_asteroid(struct World *world, struct Asteroid *ast);

/**
 * Add a projectile to the world.
 *
 * NOTE: This function takes the ownership of the object.
 */
int
world_add_projectile(struct World *world, struct Projectile *projectile);

/**
 * Update the world by given delta time.
 */
int
world_update(struct World *world, float dt);

/**
 * Create an enemy.
 */
struct Enemy*
enemy_new(float x, float y);

/**
 * Destroy an enemy.
 *
 * NOTE: Do not attempt to destroy an object owned by the world.
 */
void
enemy_destroy(struct Enemy *enemy);

/**
 * Create an asteroid.
 *
 * NOTE: Do not attempt to destroy an object owned by the world.
 */
struct Asteroid*
asteroid_new(float x, float y, float xvel, float yvel, float rot_speed);

/**
 * Destroy an asteroid.
 *
 * NOTE: Do not attempt to destroy an object owned by the world.
 */
void
asteroid_destroy(struct Asteroid *ast);