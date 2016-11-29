CFLAGS := -std=c99 -Wall -Werror -g -DDEBUG -I./lua/install/include `sdl2-config --cflags` `pkg-config --cflags glew SDL2_image`
LDFLAGS := `sdl2-config --libs` `pkg-config --libs glew SDL2_image` -framework OpenGL -framework Accelerate
OBJS = main.o sprite.o memory.o matlib.o shader.o ioutils.o strutils.o script.o physics.o game.o
LUA_LIB = lua/install/lib/liblua.a

all: $(LUA_LIB) game

test: game
	./game

game: $(LUA_LIB) $(OBJS)
	$(CC) $^ $(LDFLAGS) -o $@

$(LUA_LIB):
	make -C lua macosx local

clean:
	rm -fv $(OBJS) game
