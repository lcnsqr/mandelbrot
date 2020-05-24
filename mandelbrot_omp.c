#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
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

#define FPS 60
#define TICKS_PER_FRAME 1000 / FPS

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

	// Mandelbrot
	if (SDL_CreateWindowAndRenderer(ctx.view.width, ctx.view.height, 0, &ctx.window, &ctx.renderer)) {
		SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Impossível criar a janela e o renderizador: %s", SDL_GetError());
		exit(-1);
	}
	SDL_SetWindowTitle(ctx.window, "Mandelbrot");
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
		if (SDL_GetMouseState(&ctx.curPos, &ctx.curPos[1]) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
			// Ação do mouse 
		}
		*/
		elapsedTime = SDL_GetTicks() - startTime;
		if ( elapsedTime < TICKS_PER_FRAME) {
			SDL_Delay(TICKS_PER_FRAME - elapsedTime);
		}

	}
}
