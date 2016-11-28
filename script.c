#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "memory.h"
#include "script.h"

struct Arg {
	int type;
	const char *name;
};

/**
 * Check called function arguments.
 */
static void
check_args(lua_State *state, const struct Arg args[])
{
	int i;
	for (i = 0; args[i].type != LUA_TNIL; i++) {
		int type = lua_type(state, i + 1);
		if (type != args[i].type) {
			char *fmt = "`%s` must be a %s, got %s";
			const char *xname = lua_typename(state, args[i].type);
			const char *tname = lua_typename(state, type);
			size_t len = snprintf(
				NULL,
				0,
				fmt,
				args[i].name,
				xname,
				tname
			);
			char err[len + 1];
			snprintf(err, len + 1, fmt, args[i].name, xname, tname);
			luaL_argerror(state, i + 1, err);
		}
	}
	if (lua_gettop(state) > i) {
		luaL_argerror(state, i + 1, "unexpected argument");
	}
}

/**
 * Check and unpack called function arguments.
 */
static void
get_args(lua_State *state, const struct Arg args[], ...)
{
	check_args(state, args);

	va_list ap;
	va_start(ap, args);
	for (int i = 0; args[i].type != LUA_TNIL; i++) {
		int index = i + 1;
		switch (args[i].type) {
		case LUA_TNUMBER:
			{
				lua_Number *dst = va_arg(ap, lua_Number*);
				*dst = lua_tonumber(state, index);
			}
			break;
		default:
			luaL_argerror(state, index, "unsupported argument type");
		}
	}
}

static struct World*
get_world_upvalue(lua_State *state)
{
	return lua_touserdata(state, lua_upvalueindex(1));
}

/**
 * Add an asteroid.
 *
 * Arguments:
 *     x:        Position x coordinate.
 *     y:        Position y coordinate.
 *     xvel:     Velocity x component.
 *     yvel:     Velocity y component.
 *     rot_spd:  Rotation speed (rad/s).
 */
static int
luafunc_add_asteroid(lua_State *state)
{
	static struct Arg args[] = {
		{ LUA_TNUMBER, "x" },
		{ LUA_TNUMBER, "y" },
		{ LUA_TNUMBER, "xvel" },
		{ LUA_TNUMBER, "yvel" },
		{ LUA_TNUMBER, "rot_spd" },
		{ LUA_TNIL }
	};
	lua_Number x, y, xvel, yvel, rot_spd;
	get_args(state, args, &x, &y, &xvel, &yvel, &rot_spd);

	struct World *world = get_world_upvalue(state);
	struct Asteroid ast = {
		.x = x,
		.y = y,
		.xvel = xvel,
		.yvel = yvel,
		.rot_speed = rot_spd
	};
	int idx = world_add_asteroid(world, &ast);

	lua_pushinteger(state, idx);

	return 1;
}

static const luaL_Reg reg[] = {
	{ "add_asteroid", luafunc_add_asteroid },
	{ NULL, NULL }
};

struct ScriptEnv*
script_env_new(void)
{
	struct ScriptEnv *env = make(struct ScriptEnv);
	if (!env) {
		return NULL;
	}

	env->state = luaL_newstate();
	if (!env->state) {
		script_env_destroy(env);
		return NULL;
	}

	luaL_openlibs(env->state);

	// retrieve version and print it
	lua_getglobal(env->state, "_VERSION");
	printf("Initialized %s environment\n", lua_tostring(env->state, -1));
	lua_pop(env->state, 1);

	return env;
}

int
script_env_init(struct ScriptEnv *env, struct World *world)
{
	assert(env);
	assert(world);

	// create library with game functions
	luaL_newlibtable(env->state, reg);
	lua_pushlightuserdata(env->state, world);
	luaL_setfuncs(env->state, reg, 1);
	lua_setglobal(env->state, "game");

	return 1;
}

int
script_load(struct ScriptEnv *env, const char *filename)
{
	if (luaL_dofile(env->state, filename)) {
		fprintf(
			stderr,
			"failed to load Lua script file `%s`:\n%s\n",
			filename,
			lua_tostring(env->state, -1)
		);
		return 0;
	}
#ifdef DEBUG
	printf("loaded script `%s`\n", filename);
#endif
	return 1;
}

void
script_env_destroy(struct ScriptEnv *env)
{
	if (env) {
		if (env->state) {
			lua_close(env->state);
		}
		destroy(env);
	}
}