#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "SDL.h"
#include "color.h"
#include <pthread.h>

#define THREADS 8

/*
printf("    Full Picture:         ./mandelbrot_omp -2.5 1.5 -2.0 2.0 11500 8\n");
printf("    Seahorse Valley:      ./mandelbrot_omp -0.8 -0.7 0.05 0.15 11500 8\n");
printf("    Elephant Valley:      ./mandelbrot_omp 0.175 0.375 -0.1 0.1 11500 8\n");
printf("    Triple Spiral Valley: ./mandelbrot_omp -0.188 -0.012 0.554 0.754 11500 8\n");
*/

// Parâmetros iniciais
#define XMIN 0.175
#define XMAX 0.375
#define YMIN -0.1
#define YMAX 0.1
#define ITERATIONS 300
#define WIDTH 800
#define HEIGHT 800
#define DEPTH 4

#define FPS 30
#define TICKS_PER_FRAME 1000 / FPS

struct View {
	size_t width, height, depth;
	size_t pixels;
	char *frame;
	// Estrutura de mapeamento posição -> cor
	struct Cor cor;
};

// Contexto de execução 
struct Context {
	// Parâmetros do conjunto mandelbrot
	long double xmin, xmax, ymin, ymax;
	int iterations;
	// Imagem
	struct View view;
	// Exibição e interação por SDL
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
	SDL_Event event;
	// Posição do cursor relativo à janela
	int curPos[2];
};

// Contexto de execução do thread
struct ThreadRange {
  struct Context *ctx;
  // Thread range
  size_t start, end;
};

void finalizar(struct Context *ctx){
	free(ctx->view.frame);
	// Encerrar SDL
	SDL_DestroyTexture(ctx->texture);
	SDL_DestroyRenderer(ctx->renderer);
	SDL_DestroyWindow(ctx->window);
	SDL_Quit();
}

long double mandelIter(long double cx, long double cy, int iterations){
	long double x = 0, y = 0;
	long double xx = 0, yy = 0, xy = 0;
	int i = 0;
	while (i < iterations && xx + yy <= 4){
		xy = x * y;
		xx = x * x;
		yy = y * y;
		x = xx - yy + cx;
		y = xy + xy + cy;
		i++;
	}
	return (long double)i;
}

void *renderPart(void *vCtx){
	struct ThreadRange *range = (struct ThreadRange *)vCtx;
	struct Context *ctx = range->ctx;
	// Coordenadas na imagem
	size_t ix, iy;
	// Coordenadas do ponto candidato
	long double x, y;
	// Cor
	unsigned int c, rgba[4];
	for (size_t i = range->start; i < range->end; i++){
		//ix = (i/ctx->view.depth) % ctx->view.width;
		ix = i % ctx->view.width;
		//iy = (i/ctx->view.depth) / ctx->view.width;
		iy = i / ctx->view.width;
		// A orientação vertical é invertida na imagem
		iy = ctx->view.height - iy - 1;
    x = ctx->xmin + (ctx->xmax - ctx->xmin) * (long double)ix / (long double)(ctx->view.width - 1);
    y = ctx->ymin + (ctx->ymax - ctx->ymin) * (long double)iy / (long double)(ctx->view.height - 1);
		//c = floor((double)ITERATIONS * log(mandelIter(x, y, ctx->iterations) )/log(ITERATIONS+1));
		c = (int)mandelIter(x, y, ctx->iterations);
		corMap(&ctx->view.cor, c, rgba);
		ctx->view.frame[ctx->view.depth * i] = (char)rgba[0];
		ctx->view.frame[ctx->view.depth * i+1] = (char)rgba[1];
		ctx->view.frame[ctx->view.depth * i+2] = (char)rgba[2];
		ctx->view.frame[ctx->view.depth * i+3] = (char)rgba[3];
	}
  pthread_exit((void*)vCtx);
}

void render(struct Context *ctx){
  // Allocate data structures for all threads
  struct ThreadRange *thread_ranges = malloc(THREADS*sizeof(struct ThreadRange));

  // The number of pixels filled by each thread
  size_t pixels_per_thread = ctx->view.pixels / THREADS;

  // Remainder pixels
  size_t remainder_pixels = ctx->view.pixels % THREADS;

  // Configure data structure for each thread
  for (size_t t = 0; t < THREADS; t++){

      // Start of the subset for the thread
      thread_ranges[t].start = t * pixels_per_thread;
      // How many pixels
      thread_ranges[t].end = thread_ranges[t].start + pixels_per_thread;

      if ( t == THREADS - 1 ){
          // The last thread gets the remainder samples
          thread_ranges[t].end += remainder_pixels;
      }

      // Context
      thread_ranges[t].ctx = ctx;
  }

  // Array to store the IDs of newly created threads
  pthread_t *thread_ids = malloc(THREADS*sizeof(pthread_t));

  // Thread attributes
  pthread_attr_t pthread_attr;

  // On success, pthread_create() returns 0;
  int pthread_return;

  // Exit status of the target thread given by pthread_exit()
  void *pthread_status;

  // Initialize thread attributes object
  pthread_attr_init(&pthread_attr);

  // Set detach state attribute in thread attributes object
  pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_JOINABLE);

  // Create threads
  for(size_t t = 0; t < THREADS; t++){
      pthread_return = pthread_create(&thread_ids[t], 
                                      &pthread_attr, 
                                      &renderPart,
                                      (void *)&thread_ranges[t]); 
      if (pthread_return) {
          printf("ERROR; return code from pthread_create() is %d\n", pthread_return);
          exit(-1);
      }
  }

  // Destroy thread attributes object
  pthread_attr_destroy(&pthread_attr);

  // Wait for threads  to terminate
  for(size_t t = 0; t < THREADS; t++){
      pthread_return = pthread_join(thread_ids[t], &pthread_status);
      if (pthread_return){
          printf("ERROR; return code from pthread_join() is %d\n", pthread_return);
          exit(-1);
      }
  }
	
	// Atualizar exibição
	SDL_UpdateTexture(ctx->texture, NULL, ctx->view.frame, DEPTH * ctx->view.width * sizeof(char));
	SDL_RenderClear(ctx->renderer);
	SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);

	SDL_RenderPresent(ctx->renderer);

  free(thread_ids);
  free(thread_ranges);
}

int main(){
	// Contexto de renderização
	struct Context ctx;

	ctx.xmin = XMIN;
	ctx.xmax = XMAX;
	ctx.ymin = YMIN;
	ctx.ymax = YMAX;
	ctx.iterations = ITERATIONS;
	// Estrutura da imagem
	ctx.view.width = WIDTH;
	ctx.view.height = HEIGHT;
	ctx.view.depth = DEPTH;
	ctx.view.pixels = ctx.view.width*ctx.view.height;
	ctx.view.frame = (char *)malloc(sizeof(char)*ctx.view.pixels*ctx.view.depth);

	// Total de cores
	corBuild(&ctx.view.cor, 12);

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Erro ao iniciar o SDL: %s", SDL_GetError());
		exit(-1);
	}

	if (SDL_CreateWindowAndRenderer(ctx.view.width, ctx.view.height, SDL_WINDOW_RESIZABLE, &ctx.window, &ctx.renderer)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Impossível criar a janela e o renderizador: %s", SDL_GetError());
		exit(-1);
	}
	SDL_SetWindowTitle(ctx.window, "Conjunto Mandelbrot");

	ctx.texture = SDL_CreateTexture(ctx.renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, ctx.view.width, ctx.view.height);

	render(&ctx);

	size_t startTime, elapsedTime;

	// Recolher eventos
	while (1){
		startTime = SDL_GetTicks();

		SDL_PollEvent(&ctx.event);
		if (ctx.event.type == SDL_QUIT){
			finalizar(&ctx);
			return 0;
		}
		else if( ctx.event.type == SDL_KEYDOWN ){
			if ( ctx.event.key.keysym.sym == SDLK_q ){ 
				// Tecla "q" encerra
				finalizar(&ctx);
				return 0;
			}
			else if ( ctx.event.key.keysym.sym == SDLK_UP ){ 
				// Seta pra cima
				double dy = (ctx.ymax - ctx.ymin)*1e-1;
				ctx.ymin += dy;
				ctx.ymax += dy;
				render(&ctx);
			}
			else if ( ctx.event.key.keysym.sym == SDLK_DOWN ){ 
				// Seta pra baixo
				double dy = (ctx.ymax - ctx.ymin)*1e-1;
				ctx.ymin -= dy;
				ctx.ymax -= dy;
				render(&ctx);
			}
			else if ( ctx.event.key.keysym.sym == SDLK_LEFT ){ 
				// Seta pra cima
				double dx = (ctx.xmax - ctx.xmin)*1e-1;
				ctx.xmin -= dx;
				ctx.xmax -= dx;
				render(&ctx);
			}
			else if ( ctx.event.key.keysym.sym == SDLK_RIGHT ){ 
				// Seta pra baixo
				double dx = (ctx.xmax - ctx.xmin)*1e-1;
				ctx.xmin += dx;
				ctx.xmax += dx;
				render(&ctx);
			}
			else if ( ctx.event.key.keysym.sym == SDLK_z ){ 
				// zoom in
				double mx = (ctx.xmin + ctx.xmax)/2.0;
				double my = (ctx.ymin + ctx.ymax)/2.0;
				double dx = (ctx.xmax - ctx.xmin)/3.0;
				double dy = (ctx.ymax - ctx.ymin)/3.0;
				ctx.xmin = mx - dx;
				ctx.xmax = mx + dx;
				ctx.ymin = my - dy;
				ctx.ymax = my + dy;
				render(&ctx);
			}
			else if ( ctx.event.key.keysym.sym == SDLK_x ){ 
				// zoom out
				double mx = (ctx.xmin + ctx.xmax)/2.0;
				double my = (ctx.ymin + ctx.ymax)/2.0;
				double dx = (ctx.xmax - ctx.xmin)*0.75;
				double dy = (ctx.ymax - ctx.ymin)*0.75;
				ctx.xmin = mx - dx;
				ctx.xmax = mx + dx;
				ctx.ymin = my - dy;
				ctx.ymax = my + dy;
				render(&ctx);
			}
		}
		/*
		if (SDL_GetMouseState(&ctx.curPos[0], &ctx.curPos[1]) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			// Ação do mouse 
		}
		*/
		elapsedTime = SDL_GetTicks() - startTime;
		if ( elapsedTime < TICKS_PER_FRAME) {
			SDL_Delay(TICKS_PER_FRAME - elapsedTime);
		}

	}
}
