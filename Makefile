CFLAGS := -std=c99 -Wall -Werror `sdl2-config --cflags` `pkg-config --cflags glew`
LDFLAGS := `sdl2-config --libs` `pkg-config --libs glew` -framework OpenGL

all: game

test: game
	./game

game: main.o
	$(CC) $< $(LDFLAGS) -o $@

clean:
	rm -fv main.o game