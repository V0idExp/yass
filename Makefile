CFLAGS := -std=c99 -Wall -Werror -g -DDEBUG -I./lua/install/include `sdl2-config --cflags` `pkg-config --cflags glew SDL2_image`
LDFLAGS := `sdl2-config --libs` `pkg-config --libs glew SDL2_image` -framework OpenGL -framework Accelerate
OBJS = main.o sprite.o memory.o matlib.o shader.o ioutils.o strutils.o script.o
LUA_LIB = lua/install/lib/liblua.a
all: game

test: game
	./game

game: $(OBJS)
	$(CC) $(LUA_LIB) $^ $(LDFLAGS) -o $@

clean:
	rm -fv $(OBJS) game