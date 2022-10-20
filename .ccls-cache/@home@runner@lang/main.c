#include "lexer.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
  FILE *f = fopen("test.s", "r");
  Toklist *toklist = lex(f);
  for (int i = 0; i < toklist->size; ++i) {
    Tok t = toklist->toklist[i];
    printf("Match number: %d\n", t.type);
  }
  printf("'%s'\n", (char *)toklist->toklist[2].value);
  printf("Done!\n");
  return 0;
}