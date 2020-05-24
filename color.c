#include "color.h"
#include <stdlib.h>

void corBuild(struct Cor* cor, int estagios){
  cor->estagios = estagios;
  cor->verticesQuant = 4;
  cor->intervQuant = cor->verticesQuant;
  cor->intervTaman = cor->estagios / cor->intervQuant;

  cor->vertices = (int**) malloc(sizeof(int*) * cor->verticesQuant);
  for ( int i = 0; i < cor->verticesQuant; i++ ){
    cor->vertices[i] = (int*) malloc(sizeof(int) * 4);
  }

  cor->direcoes = (int**) malloc(sizeof(int*) * cor->intervQuant);
  for ( int i = 0; i < cor->intervQuant; i++ ){
    cor->direcoes[i] = (int*) malloc(sizeof(int) * 4);
  }

  // Pontos de passagem de cor
  cor->vertices[0][0] = 0;
  cor->vertices[0][1] = 0;
  cor->vertices[0][2] = 120;
  cor->vertices[0][3] = 255;

  cor->vertices[1][0] = 20;
  cor->vertices[1][1] = 0;
  cor->vertices[1][2] = 0;
  cor->vertices[1][3] = 255;

  cor->vertices[2][0] = 100;
  cor->vertices[2][1] = 0;
  cor->vertices[2][2] = 200;
  cor->vertices[2][3] = 255;

  cor->vertices[3][0] = 100;
  cor->vertices[3][1] = 50;
  cor->vertices[3][2] = 200;
  cor->vertices[3][3] = 255;

  // Definir direcoes entre vertices
  for ( int i = 0; i < cor->intervQuant - 1; i++ ){
    for ( int k = 0; k < 4; k++ ){
      cor->direcoes[i][k] = cor->vertices[i + 1][k] - cor->vertices[i][k];
    }
  }
  // Última direção de volta ao primeiro vértice
  for ( int k = 0; k < 4; k++ ){
    cor->direcoes[cor->intervQuant - 1][k] = cor->vertices[0][k] - cor->vertices[cor->intervQuant - 1][cor->intervQuant - 1];
  }
}

void corMap(struct Cor* cor, int c, unsigned int* rgba){
  // k: Cor
  // p: Estágio relativo à cor
  int k, p;
  // Obter a posição no período
  c = c % cor->estagios;
  k = cor->intervQuant * c / cor->estagios;
  p = c - cor->intervTaman * k;
  for ( int j = 0; j < 4; j++ ){
    rgba[j] = cor->vertices[k][j] + p * cor->direcoes[k][j] / cor->intervTaman;
  }
}
