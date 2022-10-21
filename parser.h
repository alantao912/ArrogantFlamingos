#pragma once

#include "lexer.h"

typedef struct production {
  unsigned char id;
  unsigned char num_children;
  struct production *children;
} prod;

prod *parse(toklist *toklist);