#include "game.h"
#include "memory.h"

struct Projectile*
projectile_new(float x, float y)
{
	struct Projectile *prj = make(struct Projectile);
	if (!prj) {
		return NULL;
	}
	prj->x = prj->body.x = x;
	prj->y = prj->body.y = y;
	prj->ttl = (SCREEN_HEIGHT - 100) / PLAYER_PROJECTILE_INITIAL_SPEED;
	prj->body.xvel = 0;
	prj->body.yvel = -PLAYER_PROJECTILE_INITIAL_SPEED;
	prj->body.radius = 4;
	prj->body.type = BODY_TYPE_PROJECTILE;
	prj->body.collision_mask = BODY_TYPE_ENEMY;
	prj->body.userdata = prj;
	return prj;
}

void
projectile_destroy(struct Projectile *prj)
{
	destroy(prj);
}