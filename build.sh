#!/bin/bash
 gcc -Wall -g `sdl-config --cflags --libs` `pkg-config --cflags --libs ftgl` -o tetris -lGL  -lGLU main.c
