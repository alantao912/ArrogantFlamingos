#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct tok {
  unsigned char type;
  void *value;
} tok;

typedef struct toklist {
  tok *toklist;
  size_t size, capacity;
} toklist;

toklist *init_toklist();

typedef struct StrBuff {
  char *buff;
  size_t size, capacity;
} StrBuff;

StrBuff *init_strbuff();

static inline void push_strbuff(StrBuff *buff, char c);

static inline void pop_strbuff(StrBuff *buff);

static inline void set_strbuff(StrBuff *buff, char c);

static inline void clear_strbuff(StrBuff *buff);

static inline void push_toklist(toklist *toklist, tok t);

char *strndup(const char *src, size_t size);

toklist *lex(FILE *src);

int matches_reserved_symbol(const StrBuff *buff);

bool should_continue(const StrBuff *buff);

bool is_interrupt_char(char c);

bool is_in(char c, const char set[]);

static inline bool is_alpha(char c);

static inline bool is_numeric(char c);

void show_toklist(const toklist *toklist);