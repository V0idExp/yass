#include "game.h"
#include "memory.h"

struct Asteroid*
asteroid_new(float x, float y, float xvel, float yvel, float rot_speed)
{
	struct Asteroid *ast = make(struct Asteroid);
	if (!ast) {
		return NULL;
	}
	ast->x = x;
	ast->y = y;
	ast->rot_speed = rot_speed;
	ast->ttl = ENTITY_TTL;
	ast->body.xvel = xvel;
	ast->body.yvel = yvel;
	ast->body.x = x;
	ast->body.y = y;
	ast->body.radius = 13;
	ast->body.type = BODY_TYPE_ASTEROID;
	ast->body.collision_mask = BODY_TYPE_PLAYER;
	ast->body.userdata = ast;
	return ast;
}

void
asteroid_destroy(struct Asteroid *ast)
{
	if (ast) {
		destroy(ast);
	}
}