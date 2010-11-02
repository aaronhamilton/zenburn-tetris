prefix=/usr/local
bindir=$(prefix)/bin
datadir=$(prefix)/share

CC=gcc
EXE=zenburn_tetris
FLAGS=`pkg-config --cflags --libs sdl` -lGL -lGLU
SRC=main.c

all: $(SRC)
	echo "#define DATA_DIR \"$(datadir)/zenburn_tetris\"" > config.h
	$(CC) $(FLAGS) main.c -o $(EXE)	

clean:
	rm config.h
#	rm $(EXE)

install:
	strip zenburn_tetris
	install -D zenburn_tetris $(bindir)/zb_tetris
	mkdir -p $(datadir)/zenburn_tetris
	install -D data/font.bmp $(datadir)/zenburn_tetris/font.bmp

uninstall:
	rm $(bindir)/zb_tetris
	rm $(datadir)/zenburn_tetris/font.bmp
	rmdir $(datadir)/zenburn_tetris
