#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "SDL.h"
#include "color.h"
#include <pthread.h>

// Parâmetros iniciais
#define XMIN -2.5
#define XMAX 1
#define YMIN -1
#define YMAX 1
#define ITERATIONS 1000
#define WIDTH 700
#define HEIGHT 400
#define DEPTH 4

struct View {
	int width, height, depth;
	int frameSize;
	char *frame;
	// Estrutura de mapeamento posição -> cor
	struct Cor cor;
};

// Contexto de execução 
struct Context {
	// Parâmetros do conjunto mandelbrot
	double xmin, xmax, ymin, ymax;
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

void finalizar(struct Context *ctx){
	free(ctx->view.frame);
	// Encerrar SDL
	SDL_DestroyTexture(ctx->texture);
	SDL_DestroyRenderer(ctx->renderer);
	SDL_DestroyWindow(ctx->window);
	SDL_Quit();
}

double mandelIter(double cx, double cy, int iterations){
	double x = 0, y = 0;
	double xx = 0, yy = 0, xy = 0;
	int i = 0;
	while (i < iterations && xx + yy <= 4){
		xy = x * y;
		xx = x * x;
		yy = y * y;
		x = xx - yy + cx;
		y = xy + xy + cy;
		i++;
	}
	return (double)i;
}

void *renderPart(void *vCtx){
	struct Context *ctx = (struct Context *)vCtx;
	// Coordenadas na imagem
	int ix, iy;
	// Coordenadas do ponto candidato
	double x, y;
	// Cor
	unsigned int c, rgba[4];
	for (int i = ctx->view.frameSize/2; i < ctx->view.frameSize; i += ctx->view.depth){
		ix = (i/ctx->view.depth) % ctx->view.width;
		iy = (i/ctx->view.depth) / ctx->view.width;
		// A orientação vertical é invertida na imagem
		iy = ctx->view.height - iy - 1;
      x = ctx->xmin + (ctx->xmax - ctx->xmin) * ix / (ctx->view.width - 1);
      y = ctx->ymin + (ctx->ymax - ctx->ymin) * iy / (ctx->view.height - 1);
		c = floor((double)ITERATIONS * log(mandelIter(x, y, ctx->iterations) )/log(ITERATIONS+1));
		corMap(&ctx->view.cor, c, rgba);
		ctx->view.frame[i] = (char)rgba[0];
		ctx->view.frame[i+1] = (char)rgba[1];
		ctx->view.frame[i+2] = (char)rgba[2];
		ctx->view.frame[i+3] = (char)rgba[3];
	}
	return NULL;
}

void render(struct Context *ctx){
	// Thread da segunda metade da image
	pthread_t render_thread;
	if(pthread_create(&render_thread, NULL, renderPart, ctx)) {
		fprintf(stderr, "Error creating thread\n");
		return;
	}

	// Coordenadas na imagem
	int ix, iy;
	// Coordenadas do ponto candidato
	double x, y;
	// Cor
	unsigned int c, rgba[4];
	for (int i = 0; i < ctx->view.frameSize/2; i += ctx->view.depth){
		ix = (i/ctx->view.depth) % ctx->view.width;
		iy = (i/ctx->view.depth) / ctx->view.width;
		// A orientação vertical é invertida na imagem
		iy = ctx->view.height - iy - 1;
      x = ctx->xmin + (ctx->xmax - ctx->xmin) * ix / (ctx->view.width - 1);
      y = ctx->ymin + (ctx->ymax - ctx->ymin) * iy / (ctx->view.height - 1);
		c = floor((double)ITERATIONS * log(mandelIter(x, y, ctx->iterations) )/log(ITERATIONS+1));
		corMap(&ctx->view.cor, c, rgba);
		ctx->view.frame[i] = (char)rgba[0];
		ctx->view.frame[i+1] = (char)rgba[1];
		ctx->view.frame[i+2] = (char)rgba[2];
		ctx->view.frame[i+3] = (char)rgba[3];
	}
	// wait for the second thread to finish
	if(pthread_join(render_thread, NULL)) {
		fprintf(stderr, "Error joining thread\n");
		return;
	}
	
	// Atualizar exibição
	SDL_UpdateTexture(ctx->texture, NULL, ctx->view.frame, DEPTH * ctx->view.width * sizeof(char));
	SDL_RenderClear(ctx->renderer);
	SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);

	SDL_RenderPresent(ctx->renderer);
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
	ctx.view.frameSize = ctx.view.width*ctx.view.height*ctx.view.depth;
	ctx.view.frame = (char *)malloc(sizeof(char)*ctx.view.frameSize);
	// Total de cores equivale ao números de iterações
	corBuild(&ctx.view.cor, ctx.iterations);

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

	// Recolher eventos
	while (1){
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
		SDL_PumpEvents();
		/*
		if (SDL_GetMouseState(&ctx.curPos[0], &ctx.curPos[1]) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			// Ação do mouse 
		}
		*/
	}
}
