#include <stdio.h>
#include <stdlib.h>

static const int n = 16; // number of vertices
static const int k = 6; // degree
//static const int gsize = 22; // number of bytes for g6 adjacency list
// n6 for 16 is 'O'
static unsigned char gbytes[22];
static char code[48];

int dekomp() {
  char readbits, samebits;
  size_t nread;

  samebits=getchar();
  if (feof(stdin))
    return 0;

  readbits=n*k/2-samebits;

  nread = fread(code+samebits,sizeof(char),readbits,stdin);
  if (nread != readbits || feof(stdin) || ferror(stdin))
    return -1;

  return 1;
}

void codetonlist() {
  int i, v=0, w, jbit=0;
  int counts[16] = {0};
  char *cp = &code;

  for (i = 1; i < 21; ++i)
    gbytes[i] = 63;

  for (i=n*k/2;i>0;--i) {
     w = *cp - 1;
     while (counts[v] == k) ++v;
     // There is an edge from vertex w to vertex v
     // w is bigger.
     ++counts[v];
     ++counts[w];
     jbit = w*(w-1)/2 + v;
     gbytes[1+jbit/6] += 32 >> (jbit % 6);
//     printf("Setting bit %d for %d--%d\n", jbit, v, w);
     ++cp;
  }
  puts((char*)&gbytes); // adds newline
}

int main() {
  int erg;

  gbytes[0] = 'O';
  gbytes[21] = 0;
  while((erg=dekomp())>0) {
    codetonlist();
  }

  if(erg == -1) {
    fputs("scd-file faulty\n", stderr);
    return 3;
  }

  return 0;
}

