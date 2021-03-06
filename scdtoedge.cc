#include <stdio.h>
#include <stdlib.h>

int n,k,t=3;

int dekomp(FILE *file,char* code) {
  char readbits,samebits;

  samebits=getc(file);
  if(feof(file))
    return(0);

  readbits=n*k/2-samebits;

  fread(code+samebits,sizeof(char),readbits,file);
  if(feof(file))
    return(-1);

  return(1);
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
  printf("{");
  bool first = true;
  for(int i=1;i<=n;++i) {
    for(int j=1; j<= k; ++j) {
      if (l[i][j] < i) {
        if (!first) printf(", ");
        first = false;
        printf("%d \\[UndirectedEdge] %d",i,l[i][j]);
//        printf("%d -- %d",i,l[i][j]);
      }
    }
  }
  printf("}");
}

int main(int argc, char **argv) {
  int erg;
  char *code,**nlist,scd[20];
  FILE* scdfile;
  bool first = true;

  if(argc<3) {
    fprintf(stderr,"too few parameters\n");
    return 0;
  }

  n = atoi(*++argv);
  k = atoi(*++argv);
  argc -= 2;

  if(argc>1)
    t=atoi(*(argv+1));

  sprintf(scd,"%d%d_%d_%d.scd",n/10,n%10,k,t);
  if((scdfile=fopen(scd,"rb"))==NULL) {
    fprintf(stderr,"scd-file not found\n");
    return(0);
  }

  code = (char*)calloc(n*k/2,sizeof(char));
  nlist = (char**)calloc(n+1,sizeof(char*));
  for(int i=1;i<=n;++i)
    nlist[i]=(char*)calloc(k,sizeof(char));

  printf("{");
  while((erg=dekomp(scdfile,code))!=0) {
    if (!first) printf(",\n");
    first = false;
    codetonlist(code,nlist);
    edgelist(nlist);
  }
  printf("}\n");

  if(erg == -1) {
    fprintf(stderr,"scd-file faulty\n");
    return(-1);
  }

  return(1);
}

