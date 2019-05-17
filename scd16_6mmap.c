#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

static const int n = 16; // number of vertices
static const int k = 6; // degree
//static const int gsize = 22; // number of bytes for g6 adjacency list
// n6 for 16 is 'O'
static unsigned char gbytes[22];
static char code[48];
const char* scd;

void dekomp() {
  int toread, samebits;

  samebits = *scd;
  toread = n*k/2 - samebits;
  memcpy(code + samebits, ++scd, toread);
  scd += toread;
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

int main(int argc, char **argv) {
  int fd;
  struct stat sb;
  const char* end;

  if (argc < 2) {
    fputs("too few parameters\n", stderr);
    return 1;
  } else if (argc > 2) {
    fputs("too many parameters\n", stderr);
    return 1;
  }

  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fputs("Could not open file\n", stderr);
    return 1;
  }
  if (fstat(fd, &sb)) {
    fputs("Could not stat file\n", stderr);
    return 1;
  }
  
  scd = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (scd == MAP_FAILED) {
    fputs("Could not mmap\n", stderr);
    return 1;
  }
  if (madvise(scd, sb.st_size, MADV_SEQUENTIAL | MADV_WILLNEED)) {
    fputs("Could not advise\n", stderr);
    return 1;
  }
  end = scd + sb.st_size;
  gbytes[0] = 'O';
  gbytes[21] = 0;
  while(scd < end) {
    dekomp();
    codetonlist();
  }

  return 0;
}

