#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int n, k, gsize; //number of vertices, degree, number of bytes for g6 adjacency list
unsigned char n6[9]; // n encoded in g6 format---takes 1 byte for 0 <= n <= 62, 4 bytes for 63 <= n <= 258047,
            // max 8 bytes for 258048 <= n <= 68719476735

int setn6() {
  if (n < 0) {
    fprintf(stderr, "Negative number of vertices %d is impossible \n", n);
    return -1;
  }
  n6[0] = n6[1] = 126;
  int numb = 1 + 2*(n > 62) + 3*(n > 258047); // 1 or 3 or 6
  int end  = 0 + 3*(n > 62) + 4*(n > 258047); // 0 or 3 or 7
  for (int k = 0; k < numb; ++k) {
    n6[end - k] = 63 + ((n >> (6*k)) & 63);
  }
  n6[end+1] = 0;
  return 0;
}

int dekomp(FILE *file,char* code) {
  char readbits, samebits;
  size_t nread;

  samebits=getc(file);
  if (feof(file))
    return 0;

  readbits=n*k/2-samebits;

  nread = fread(code+samebits,sizeof(char),readbits,file);
  if (nread != readbits || feof(file) || ferror(file))
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
  printf("%s%s\n", n6, (char*)&gbytes);
//  puts((char*)&gbytes); // adds newline
}

int main(int argc, char **argv) {
  int erg;
  char *code, **nlist, *badchar;
  FILE* scdfile = stdin;

  if (argc < 3) {
    fputs("too few parameters\n", stderr);
    return 1;
  } else if (argc > 4) {
    fputs("too many parameters\n", stderr);
    return E2BIG;
  }

  n = strtol(argv[1], &badchar, 10);
  if (errno != 0 || *badchar != '\0') {
    fputs("first parameter must be a positive integer (# of vertices)\n", stderr);
    return 1;
  }
  k = strtol(argv[2], &badchar, 10);
  if (errno != 0 || *badchar != '\0') {
    fputs("second parameter must be a positive integer (graph degree)\n", stderr);
    return 1;
  }
  if (setn6() != 0)
    return EDOM;
  gsize = (n*n - n + 11) / 12 + 1;

  if (argc > 3) {
      if ((scdfile=fopen(argv[3], "rb"))==NULL) {
        fprintf(stderr, "scd-file %s not found\n", argv[3]);
        return ENOENT;
      }
  }


  code = (char*)calloc(n*k/2,sizeof(char));
  nlist = (char**)calloc(n+1,sizeof(char*));
  for(int i=1;i<=n;++i)
    nlist[i]=(char*)calloc(k,sizeof(char));

  while((erg=dekomp(scdfile,code))>0) {
    codetonlist(code,nlist);
    edgelist(nlist);
  }

  if(erg == -1) {
    fputs("scd-file faulty\n", stderr);
    return EINVAL;
  }

  return 0;
}

