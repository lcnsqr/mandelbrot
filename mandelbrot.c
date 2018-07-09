#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "SDL.h"
#include "color.h"

// Parâmetros iniciais
#define XMIN -2.5
#define XMAX 1
#define YMIN -1.75
#define YMAX 1.75
#define ITERATIONS 1000
#define WIDTH 400
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

void render(struct Context *ctx){
	// Coordenadas na imagem
	int ix, iy;
	// Coordenadas do ponto candidato
	double x, y;
	// Cor
	unsigned int c, rgba[4];
	#pragma omp parallel for private(ix,iy,x,y,c,rgba)
	for (int i = 0; i < ctx->view.frameSize; i += ctx->view.depth){
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
	
	// Atualizar exibição
	SDL_UpdateTexture(ctx->texture, NULL, ctx->view.frame, DEPTH * ctx->view.width * sizeof(char));
	SDL_RenderClear(ctx->renderer);
	SDL_RenderCopy(ctx->renderer, ctx->texture, NULL, NULL);

	SDL_RenderPresent(ctx->renderer);
}

int main(int argc, char **argv){
	// Contextos de renderização
	// 0: Mandelbrot pelo algoritmo tradicional
	// 1: Estimado pela GRNN
	struct Context ctx[2];
	for (int c = 0; c < 2; c++){
		ctx[c].xmin = XMIN;
		ctx[c].xmax = XMAX;
		ctx[c].ymin = YMIN;
		ctx[c].ymax = YMAX;
		ctx[c].iterations = ITERATIONS;
		// Estrutura da imagem
		ctx[c].view.width = WIDTH;
		ctx[c].view.height = HEIGHT;
		ctx[c].view.depth = DEPTH;
		ctx[c].view.frameSize = ctx[c].view.width*ctx[c].view.height*ctx[c].view.depth;
		ctx[c].view.frame = (char *)malloc(sizeof(char)*ctx[c].view.frameSize);
		// Total de cores equivale ao números de iterações
		corBuild(&ctx[c].view.cor, ctx[c].iterations);
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Erro ao iniciar o SDL: %s", SDL_GetError());
		exit(-1);
	}

	// Mandelbrot
	if (SDL_CreateWindowAndRenderer(ctx[0].view.width, ctx[0].view.height, 0, &ctx[0].window, &ctx[0].renderer)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Impossível criar a janela e o renderizador: %s", SDL_GetError());
		exit(-1);
	}
	SDL_SetWindowTitle(ctx[0].window, "Mandelbrot");
	ctx[0].texture = SDL_CreateTexture(ctx[0].renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, ctx[0].view.width, ctx[0].view.height);
	render(&ctx[0]);

	// GRNN
	if (SDL_CreateWindowAndRenderer(ctx[1].view.width, ctx[1].view.height, 0, &ctx[1].window, &ctx[1].renderer)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Impossível criar a janela e o renderizador: %s", SDL_GetError());
		exit(-1);
	}
	SDL_SetWindowTitle(ctx[1].window, "GRNN");
	ctx[1].texture = SDL_CreateTexture(ctx[1].renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, ctx[1].view.width, ctx[1].view.height);
	render(&ctx[1]);

	// Recolher eventos
	while (1){
		SDL_PollEvent(&ctx[0].event);
		if (ctx[0].event.type == SDL_QUIT){
			finalizar(&ctx[0]);
			return 0;
		}
		else if( ctx[0].event.type == SDL_KEYDOWN ){
			if ( ctx[0].event.key.keysym.sym == SDLK_q ){ 
				// Tecla "q" encerra
				finalizar(&ctx[0]);
				return 0;
			}
			else if ( ctx[0].event.key.keysym.sym == SDLK_UP ){ 
				// Seta pra cima
				double dy = (ctx[0].ymax - ctx[0].ymin)*1e-1;
				ctx[0].ymin += dy;
				ctx[0].ymax += dy;
				render(&ctx[0]);
			}
			else if ( ctx[0].event.key.keysym.sym == SDLK_DOWN ){ 
				// Seta pra baixo
				double dy = (ctx[0].ymax - ctx[0].ymin)*1e-1;
				ctx[0].ymin -= dy;
				ctx[0].ymax -= dy;
				render(&ctx[0]);
			}
			else if ( ctx[0].event.key.keysym.sym == SDLK_LEFT ){ 
				// Seta pra cima
				double dx = (ctx[0].xmax - ctx[0].xmin)*1e-1;
				ctx[0].xmin -= dx;
				ctx[0].xmax -= dx;
				render(&ctx[0]);
			}
			else if ( ctx[0].event.key.keysym.sym == SDLK_RIGHT ){ 
				// Seta pra baixo
				double dx = (ctx[0].xmax - ctx[0].xmin)*1e-1;
				ctx[0].xmin += dx;
				ctx[0].xmax += dx;
				render(&ctx[0]);
			}
			else if ( ctx[0].event.key.keysym.sym == SDLK_z ){ 
				// zoom in
				double mx = (ctx[0].xmin + ctx[0].xmax)/2.0;
				double my = (ctx[0].ymin + ctx[0].ymax)/2.0;
				double dx = (ctx[0].xmax - ctx[0].xmin)/3.0;
				double dy = (ctx[0].ymax - ctx[0].ymin)/3.0;
				ctx[0].xmin = mx - dx;
				ctx[0].xmax = mx + dx;
				ctx[0].ymin = my - dy;
				ctx[0].ymax = my + dy;
				render(&ctx[0]);
			}
			else if ( ctx[0].event.key.keysym.sym == SDLK_x ){ 
				// zoom out
				double mx = (ctx[0].xmin + ctx[0].xmax)/2.0;
				double my = (ctx[0].ymin + ctx[0].ymax)/2.0;
				double dx = (ctx[0].xmax - ctx[0].xmin)*0.75;
				double dy = (ctx[0].ymax - ctx[0].ymin)*0.75;
				ctx[0].xmin = mx - dx;
				ctx[0].xmax = mx + dx;
				ctx[0].ymin = my - dy;
				ctx[0].ymax = my + dy;
				render(&ctx[0]);
			}
		}
		SDL_PumpEvents();
		/*
		if (SDL_GetMouseState(&ctx[0].curPos[0], &ctx[0].curPos[1]) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			// Ação do mouse 
		}
		*/
	}
}
