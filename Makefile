CFLAGS := $(CFLAGS) -std=c99 -Wall -Werror -g -DDEBUG -I./lua/install/include `sdl2-config --cflags` `pkg-config --cflags freetype2 glew SDL2_image`
LDFLAGS := $(LDFLAGS) -L./lua/install/lib -llua `sdl2-config --libs` `pkg-config --libs freetype2 glew SDL2_image`
OS := $(shell uname -s)
LUA_LIB = lua/install/lib/liblua.a
LUA_TARGET :=
OBJS = text.o font.o error.o projectile.o asteroid.o utils.o enemy.o list.o main.o sprite.o memory.o matlib.o shader.o ioutils.o strutils.o script.o physics.o game.o

ifeq ($(OS), Linux)
	LUA_TARGET += linux
	LDFLAGS += -lm -lblas -ldl -Wl,-Bstatic -Wl,-Bdynamic
else ifeq ($(OS), Darwin)
	LUA_TARGET += macosx
	LDFLAGS += -framework OpenGL -framework Accelerate
endif

all: $(LUA_LIB) game

test: game
	./game

game: $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(LUA_LIB):
	make -C lua $(LUA_TARGET) local

clean:
	rm -fv $(OBJS) game

distclean: clean
	make -C lua clean
	rm -rf lua/install
