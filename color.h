// Mapeamento de número inteiro para cor
struct Cor { 
  // Numero de cores disponveis
  int estagios;
  // Pontos de passagem
  int verticesQuant;
  // Intervalos entre vertices
  int intervQuant;
  // Tamanho de cada intervalo
  float intervTaman;
  // Os vertices
  int** vertices;
  // Direcoes (vertices - 1)
  int** direcoes;
};

void corBuild(struct Cor* cor, int estagios);

void corMap(struct Cor* cor, int c, unsigned int* rgba);
