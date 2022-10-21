#include <stdio.h>
#include "lexer.h"

extern const char *reserved_symbols[];

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; ++i) {
    FILE *f = fopen(argv[i], "r");
    if (!f) {
      printf("Failed to open file '%s'\n", argv[i]);
      exit(-1);
    }
    const toklist *toklist = lex(f);
    fclose(f);
    show_toklist(toklist);
    free((struct toklist *) toklist);
  }
  printf("Done\n");
  return 0;
}