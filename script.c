#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "memory.h"
#include "script.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

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
 *     x:          Position x coordinate.
 *     y:          Position y coordinate.
 *     xvel:       Velocity x component.
 *     yvel:       Velocity y component.
 *     rot_speed:  Rotation speed (rad/s).
 */
static int
luafunc_add_asteroid(lua_State *state)
{
	static struct Arg args[] = {
		{ LUA_TNUMBER, "x" },
		{ LUA_TNUMBER, "y" },
		{ LUA_TNUMBER, "xvel" },
		{ LUA_TNUMBER, "yvel" },
		{ LUA_TNUMBER, "rot_speed" },
		{ LUA_TNIL }
	};
	lua_Number x, y, xvel, yvel, rot_speed;
	get_args(state, args, &x, &y, &xvel, &yvel, &rot_speed);

	struct World *world = get_world_upvalue(state);
	struct Asteroid *ast = asteroid_new(x, y, xvel, yvel, rot_speed);
	if (!ast || !world_add_asteroid(world, ast)) {
		asteroid_destroy(ast);
		return luaL_error(state, "add_asteroid() call failed");
	}

	return 0;
}

/**
 * Add an enemy.
 *
 * Arguments:
 *     x:     Position x coordinate.
 *     y:     Position y coordinate.
 */
static int
luafunc_add_enemy(lua_State *state)
{
	static struct Arg args[] = {
		{ LUA_TNUMBER, "x" },
		{ LUA_TNUMBER, "y" },
		{ LUA_TNIL }
	};
	lua_Number x, y;
	get_args(state, args, &x, &y);

	struct Enemy *enemy = enemy_new(x, y);
	struct World *world = get_world_upvalue(state);
	if (!enemy || !world_add_enemy(world, enemy)) {
		enemy_destroy(enemy);
		return luaL_error(state, "add_enemy() call failed");
	}

	return 0;
}

static const luaL_Reg reg[] = {
	{ "add_asteroid", luafunc_add_asteroid },
	{ "add_enemy", luafunc_add_enemy },
	{ NULL, NULL }
};

static const struct {
	const char *name;
	lua_Number value;
} game_constants[] = {
	{ "SCROLL_SPEED", SCROLL_SPEED },
	{ "SCREEN_WIDTH", SCREEN_WIDTH },
	{ "SCREEN_HEIGHT", SCREEN_HEIGHT },
	{ NULL }
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

	env->tick_func = LUA_NOREF;

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

	// create game library
	luaL_newlibtable(env->state, reg);
	int game_table = lua_gettop(env->state);

	// register functions
	lua_pushlightuserdata(env->state, world);
	luaL_setfuncs(env->state, reg, 1);

	// register game constants
	for (unsigned i = 0; game_constants[i].name != NULL; i++) {
		lua_pushnumber(env->state, game_constants[i].value);
		lua_setfield(env->state, game_table, game_constants[i].name);
	}

	// register the library as `game` global
	lua_setglobal(env->state, "game");

	return 1;
}

int
script_env_load_file(struct ScriptEnv *env, const char *filename)
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

	// check whether there's an update function and make a reference for it
	lua_getglobal(env->state, "tick");
	if (lua_type(env->state, -1) == LUA_TFUNCTION) {
		env->tick_func = luaL_ref(env->state, LUA_REGISTRYINDEX);
	} else {
		lua_pop(env->state, 1);
	}

	return 1;
}

int
script_env_tick(struct ScriptEnv *env)
{
	if (env->tick_func != LUA_NOREF) {
		lua_rawgeti(env->state, LUA_REGISTRYINDEX, env->tick_func);
		if (lua_pcall(env->state, 0, 0, 0) != LUA_OK) {
			fprintf(
				stderr,
				"failed to call `update()` script function:\n%s\n",
				lua_tostring(env->state, -1)
			);
			return 0;
		}
	}
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