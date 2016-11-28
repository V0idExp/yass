#pragma once

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 800
#define MAX_ASTEROIDS 100
#define MAX_PROJECTILES 20
#define MAX_ENEMIES 20
#define ENEMY_SPEED 50.0  // units/second
#define PLAYER_INITIAL_SPEED 200.0  // units/second
#define PLAYER_ACTION_SHOOT_RATE 1.0  // projectiles/second
#define PLAYER_PROJECTILE_INITIAL_SPEED 400  // units/second
#define PLAYER_PROJECTILE_TTL 5.0  // seconds

/**
 * Player action bits.
 */
enum {
	ACTION_MOVE_LEFT = 1,
	ACTION_MOVE_RIGHT = 1 << 1,
	ACTION_MOVE_UP = 1 << 2,
	ACTION_MOVE_DOWN = 1 << 3,
	ACTION_SHOOT = 1 << 4
};

/**
 * Player.
 */
struct Player {
	float x, y;     // position
	int actions;    // actions bitmask
	float speed;    // speed in units/second
	float shoot_cooldown;
};

/**
 * Enemy.
 */
struct Enemy {
	float x, y;
	float xvel, yvel;
	float speed;
	float rot;
};

/**
 * Asteroid.
 */
struct Asteroid {
	float x, y;
	float xvel, yvel;
	float rot;
	float rot_speed;
};

/**
 * Projectile.
 */
struct Projectile {
	float x, y;
	float xvel, yvel;
	float ttl;
};

/**
 * World container.
 *
 * This struct holds all the objects which make up the game.
 */
struct World {
	struct Player player;
	struct Asteroid asteroids[MAX_ASTEROIDS];
	size_t asteroid_count;
	struct Projectile projectiles[MAX_PROJECTILES];
	struct Enemy enemies[MAX_ENEMIES];
	size_t enemy_count;
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
world_add_enemy(struct World *world, const struct Enemy *enemy);

/**
 * Add an asteroid to the world.
 */
int
world_add_asteroid(struct World *world, const struct Asteroid *ast);

/**
 * Add a projectile tp the world.
 */
void
world_add_projectile(struct World *w, const struct Projectile *projectile);

/**
 * Update the world by given delta time.
 */
int
world_update(struct World *world, float dt);