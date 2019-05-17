#include <stdio.h>
#include <stdlib.h>
// C++17 version, with C-style IO!

static const constexpr int n = 16; // number of vertices
static const constexpr int k = 6; // degree
static const constexpr int gsize = 21; // number of bytes for g6 adjacency list
// n6 for 16 is 'O'

int dekomp(char* code) {
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

void codetonlist(char *code, char **l) {
  int i,v=1,w;

  for(i=1; i<=n; ++i)
     l[i][0]=0;

  for(i=n*k/2;i>0;--i) {
     w = *code;
     while(l[v][0]==k) ++v;
     l[v][++l[v][0]]=w;
     l[w][++l[w][0]]=v;
     ++code;
  }
}

void edgelist(char **l) {
  int ibit = 0, jbit = 0;
  unsigned char gbytes[gsize];
  for (int i = 0; i < gsize-1; ++i)
    gbytes[i] = 63;
  gbytes[gsize-1] = 0;
  for(int i=2;i<=n;++i) {
    for(int j=1; j<= k; ++j) {
      if (l[i][j] < i) {
        jbit = ibit + l[i][j] - 1;
        gbytes[jbit/6] += 32 >> (jbit % 6);
//        printf("Setting bit %d + %d - 1 = %d for %d--%d\n", ibit, l[i][j], jbit, l[i][j], i);
      }
    }
    ibit += (i - 1);
  }
  printf("O%s\n", (char*)&gbytes);
//  puts((char*)&gbytes); // adds newline
}

int main() {
  int erg;
  char *code, **nlist;

  code = (char*)calloc(n*k/2,sizeof(char));
  nlist = (char**)calloc(n+1,sizeof(char*));
  for(int i=1;i<=n;++i)
    nlist[i]=(char*)calloc(k,sizeof(char));

  while((erg=dekomp(code))>0) {
    codetonlist(code,nlist);
    edgelist(nlist);
  }

  if(erg == -1) {
    fputs("scd-file faulty\n", stderr);
    return 3;
  }

  return 0;
}

