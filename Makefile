CC = gcc
FLAGS = -Wall -O3

all: mandelbrot_omp mandelbrot_pth

mandelbrot_pth: mandelbrot_pth.o color.o
	$(CC) -o $@ $^ -lm -pthread `sdl2-config --cflags --libs`

mandelbrot_omp: mandelbrot_omp.o color.o
	$(CC) -o $@ $^ -lm -fopenmp `sdl2-config --cflags --libs`

mandelbrot_omp.o: mandelbrot_omp.c
	$(CC) $(FLAGS) -lm -fopenmp `sdl2-config --cflags --libs` -c -o $@ $^
          
mandelbrot_pth.o: mandelbrot_pth.c
	$(CC) $(FLAGS) -lm -pthread `sdl2-config --cflags --libs` -c -o $@ $^

color.o: color.c
	$(CC) $(FLAGS) -c -o $@ $^

.PHONY: clean
clean:
	rm mandelbrot_omp mandelbrot_pth *.o
