#!/bin/bash
 gcc -Wall -g `sdl-config --cflags --libs` -o tetris -lGL -lSDL -lGLU main.c
