#include "game.h"
#include "matlib.h"
#include "memory.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

struct UpdateContext {
	struct World *world;
	float dt;
};

struct Entity {
	float x, y;
	struct Body body;
};

static void
add_event(struct World *world, const struct Event *evt)
{
	// extend the event queue
	if (world->event_count == world->event_queue_size) {
		size_t new_size = world->event_queue_size + EVENT_QUEUE_BASE_SIZE;
		void *new_queue = realloc(
			world->event_queue,
			sizeof(struct Event) * new_size
		);
		if (!new_queue) {
			fprintf(stderr, "event queue too long: out of memory\n");
			exit(EXIT_FAILURE);
		}
		world->event_queue = new_queue;
		world->event_queue_size = new_size;
	}

	// append the event to the end of the queue
	world->event_queue[world->event_count++] = *evt;
}

static int
handle_player_collision(struct Body *a, struct Body *b, void *userdata)
{
	if (a->type == BODY_TYPE_PLAYER) {
		struct Event evt = {
			.type = EVENT_PLAYER_COLLISION,
			.collision = {
				.first = a,
				.second = b
			}
		};
		struct World *world = userdata;
		add_event(world, &evt);
	}
	return 1;
}

static int
handle_enemy_hit(struct Body *a, struct Body *b, void *userdata)
{
	if (a->type == BODY_TYPE_ENEMY) {
		struct Event evt = {
			.type = EVENT_ENEMY_HIT,
			.hit = {
				.target = a->userdata,
				.projectile = b->userdata
			}

		};
		struct World *world = userdata;
		add_event(world, &evt);
	}
	return 1;
}

struct World*
world_new(void)
{
	struct World *w = make(struct World);
	if (!w) {
		return NULL;
	}

	// initialize entity lists
	w->asteroid_list = list_new();
	w->projectile_list = list_new();
	w->enemy_list = list_new();
	if (!w->asteroid_list ||
	    !w->projectile_list ||
	    !w->enemy_list) {
		world_destroy(NULL);
		return NULL;
	}

	// initialize simulation system and register collision callbacks
	w->sim = sim_new();
	if (!w->sim) {
		world_destroy(w);
		return NULL;
	}
	struct CollisionHandler handlers[] = {
		{
			handle_player_collision,
			BODY_TYPE_PLAYER | BODY_TYPE_ENEMY,
			w
		},
		{
			handle_player_collision,
			BODY_TYPE_PLAYER | BODY_TYPE_ASTEROID,
			w
		},
		{
			handle_enemy_hit,
			BODY_TYPE_ENEMY | BODY_TYPE_PROJECTILE,
			w
		},
		{ NULL }
	};
	for (int i = 0; handlers[i].callback; i++) {
		if (!sim_add_handler(w->sim, &handlers[i])) {
			world_destroy(w);
			return NULL;
		}
	}

	// initialize event queue
	w->event_queue = malloc(sizeof(struct Event) * EVENT_QUEUE_BASE_SIZE);
	if (!w->event_queue) {
		world_destroy(w);
		return NULL;
	}
	w->event_queue_size = EVENT_QUEUE_BASE_SIZE;

	// initialize player
	w->player.hitpoints = PLAYER_INITIAL_HITPOINTS;
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
	w->player.body = player_body;

	if (!sim_add_body(w->sim, &w->player.body)) {
		world_destroy(w);
		return NULL;
	}

	return w;
}

static void
destroy_entity(void *entity_ptr, void *type)
{
	switch ((long)type) {
	case 0:  // enemy
		enemy_destroy(entity_ptr);
		break;
	case 1:  // projectile
		projectile_destroy(entity_ptr);
		break;
	case 2:  // asteroid
		asteroid_destroy(entity_ptr);
		break;
	}
}

void
world_destroy(struct World *w)
{
	if (w) {
		free(w->event_queue);
		sim_destroy(w->sim);

		// destroy entities
		struct List *lists[] = {
			w->enemy_list,
			w->projectile_list,
			w->asteroid_list,
			NULL,
		};
		for (long i = 0; lists[i] != NULL; i++) {
			list_foreach(lists[i], destroy_entity, (void*)i);
			list_destroy(lists[i]);
		}

		destroy(w);
	}
}

int
world_add_asteroid(struct World *world, struct Asteroid *ast)
{
	if (!list_add(world->asteroid_list, ast)) {
		return 0;
	} else if (!sim_add_body(world->sim, &ast->body)) {
		list_remove(world->asteroid_list, ast, ptr_cmp);
		return 0;
	}
	return 1;
}

int
world_add_enemy(struct World *world, struct Enemy *enemy)
{
	if (!list_add(world->enemy_list, enemy)) {
		return 0;
	} else if (!sim_add_body(world->sim, &enemy->body)) {
		list_remove(world->enemy_list, enemy, ptr_cmp);
		return 0;
	}
	return 1;
}

int
world_add_projectile(struct World *world, struct Projectile *projectile)
{
	if (!list_add(world->projectile_list, projectile)) {
		return 0;
	} else if (!sim_add_body(world->sim, &projectile->body)) {
		list_remove(world->projectile_list, projectile, ptr_cmp);
		return 0;
	}
	return 1;
}

static int
update_enemy(void *enemy_ptr, void *ctx_ptr)
{
	struct Enemy *enemy = enemy_ptr;
	struct UpdateContext *ctx = ctx_ptr;

	// check enemy conditions and TTL and if not satisfied, destroy it and
	// remove from the list
	if (enemy->hitpoints <= 0 ||
	    (enemy->ttl -= ctx->dt) <= 0) {
		sim_remove_body(ctx->world->sim, &enemy->body);
		enemy_destroy(enemy);
		return 0;
	}
	return 1;
}

static int
update_asteroid(void *ast_ptr, void *ctx_ptr)
{
	struct Asteroid *ast = ast_ptr;
	struct UpdateContext *ctx = ctx_ptr;

	// destroy and filter out the asteroid from list if its TTL expired
	if ((ast->ttl -= ctx->dt) <= 0) {
		sim_remove_body(ctx->world->sim, &ast->body);
		asteroid_destroy(ast);
		return 0;
	}

	// update position
	ast->x = ast->body.x;
	ast->y = ast->body.y;

	// update rotation
	ast->rot += ast->rot_speed * ctx->dt;
	if (ast->rot >= M_PI * 2) {
		ast->rot -= M_PI * 2;
	}

	return 1;
}

static int
update_projectile(void *prj_ptr, void *ctx_ptr)
{
	struct Projectile *prj = prj_ptr;
	struct UpdateContext *ctx = ctx_ptr;
	if ((prj->ttl -= ctx->dt) <= 0) {
		sim_remove_body(ctx->world->sim, &prj->body);
		projectile_destroy(prj);
		return 0;
	}

	// update position
	prj->x = prj->body.x;
	prj->y = prj->body.y;

	return 1;
}

static void
scroll_entity(void *entity_ptr, void *ctx_ptr)
{
	struct Entity *entity = entity_ptr;
	struct UpdateContext *ctx = ctx_ptr;
	entity->y = entity->body.y += SCROLL_SPEED * ctx->dt;
}

int
world_update(struct World *world, float dt)
{
	struct Player *plr = &world->player;

	// update physics
	static float sim_acc = 0;
	sim_acc += dt;
	while (sim_acc >= SIMULATION_STEP) {
		sim_step(world->sim, SIMULATION_STEP);
		sim_acc -= SIMULATION_STEP;
	}

	// process events
	for (size_t i = 0; i < world->event_count; i++) {
		struct Event *evt = &world->event_queue[i];
		struct Enemy *enemy;
		struct Projectile *prj = NULL;
		struct Asteroid *ast = NULL;

		switch (evt->type) {
		case EVENT_ENEMY_HIT:
			printf("enemy hit by player!\n");
			enemy = evt->hit.target;
			enemy->hitpoints -= PLAYER_INITIAL_DAMAGE;
			prj = evt->hit.projectile;
			prj->ttl = 0;
			break;
		case EVENT_PLAYER_COLLISION:
			switch (evt->collision.second->type) {
			case BODY_TYPE_ENEMY:
				printf("player collided with an enemy!\n");
				plr->hitpoints -= ENEMY_COLLISION_DAMAGE;
				enemy = evt->collision.second->userdata;
				enemy->hitpoints = 0;
				break;
			case BODY_TYPE_ASTEROID:
				printf("player collided with an asteroid!\n");
				plr->hitpoints -= ASTEROID_COLLISION_DAMAGE;
				ast = evt->collision.second->userdata;
				ast->ttl = 0;
				break;
			}
			break;
		}
	}
	// flush the event queue
	world->event_count = 0;

	// check game conditions
	if (plr->hitpoints <= 0) {
		return 0;
	}

	// update player position
	float distance = dt * plr->speed;
	if (plr->actions & ACTION_MOVE_LEFT) {
		plr->x = plr->body.x -= distance;
	} else if (plr->actions & ACTION_MOVE_RIGHT) {
		plr->x = plr->body.x += distance;
	}

	// handle shooting
	plr->shoot_cooldown -= dt;
	if (plr->actions & ACTION_SHOOT &&
	    plr->shoot_cooldown <= 0) {
		// reset cooldown timer
		plr->shoot_cooldown = 1.0 / PLAYER_ACTION_SHOOT_RATE;

		// shoot a projectile
		struct Projectile *prj = projectile_new(plr->x, plr->y);
		if (!prj || !world_add_projectile(world, prj)) {
			return 0;
		}
	}

	struct UpdateContext ctx = {
		world,
		dt
	};

	// update enemies
	list_filter(world->enemy_list, update_enemy, &ctx);

	// update asteroids
	list_filter(world->asteroid_list, update_asteroid, &ctx);

	// update projectiles
	list_filter(world->projectile_list, update_projectile, &ctx);

	// scroll entities down
	list_foreach(world->enemy_list, scroll_entity, &ctx);
	list_foreach(world->asteroid_list, scroll_entity, &ctx);

	return 1;
}