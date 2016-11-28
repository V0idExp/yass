#pragma once

#define MAX_ASTEROIDS 15
#define MAX_PROJECTILES 20
#define MAX_ENEMIES 1
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
	struct Sprite *sprite;
};

/**
 * Enemy.
 */
struct Enemy {
	float x, y;
	float xvel, yvel;
	float speed;
	float rot;
	struct Sprite *sprite;
};

/**
 * Asteroid.
 */
struct Asteroid {
	float x, y;
	float xvel, yvel;
	float rot;
	float rot_speed;
	struct Sprite *sprite;
};

/**
 * Projectile.
 */
struct Projectile {
	float x, y;
	float xvel, yvel;
	float ttl;
	struct Sprite *sprite;
};

/**
 * World container.
 *
 * This struct holds all the objects which make up the game.
 */
struct World {
	struct Player player;
	struct Asteroid asteroids[MAX_ASTEROIDS];
	struct Projectile projectiles[MAX_PROJECTILES];
	struct Enemy enemies[MAX_ENEMIES];
};