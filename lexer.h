#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct Tok {
  unsigned char type;
  void *value;
} Tok;

typedef struct Toklist {
  Tok *toklist;
  size_t size, capacity;
} Toklist;

Toklist *init_toklist();

typedef struct StrBuff {
  char *buff;
  size_t size, capacity;
} StrBuff;

StrBuff *init_strbuff();

inline void push_strbuff(StrBuff *buff, char c);

inline void pop_strbuff(StrBuff *buff);

inline void set_strbuff(StrBuff *buff, char c);

inline void clear_strbuff(StrBuff *buff);

inline void push_toklist(Toklist *toklist, Tok t);

Toklist *lex(FILE *src);

int matches_reserved_symbol(const StrBuff *buff);

bool should_continue(const StrBuff *buff);

bool is_interrupt_char(char c);

inline bool is_alpha(char c);

inline bool is_numeric(char c);