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
	ast->xvel = xvel;
	ast->yvel = yvel;
	ast->rot_speed = rot_speed;
	return ast;
}

void
asteroid_destroy(struct Asteroid *ast)
{
	if (ast) {
		destroy(ast);
	}
}