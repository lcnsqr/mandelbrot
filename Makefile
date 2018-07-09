CC=gcc
OPTIONS=-O3

all: mandelbrot

mandelbrot: mandelbrot.c
	$(CC) mandelbrot.c -lm -fopenmp `sdl2-config --cflags --libs` $(OPTIONS) -o $@

clean:
	rm mandelbrot
