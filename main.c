#include <stdio.h>
#include "lexer.h"

int main(int argc, char *argv[]) {
  FILE *f = fopen("test.s", "r");
  Toklist *toklist = lex(f);
  
  for (int i = 0; i < toklist->size; ++i) {
    Tok t = toklist->toklist[i];
    printf("Match number: %d. ", t.type);
    if (t.value) {
      printf("Associated value: %s, %d\n", (char *) t.value, strlen((char *) t.value));
    } else {
      printf("\n");
    }
  }
  return 0;
}