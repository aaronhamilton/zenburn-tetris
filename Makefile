CC=gcc
EXE=zenburn_tetris
FLAGS=`pkg-config --cflags --libs sdl` -lGL -lGLU
SRC=main.c

all: $(SRC)
	$(CC) $(FLAGS) main.c -o $(EXE)
