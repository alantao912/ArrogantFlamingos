#include "lexer.h"

#include <stdio.h>
#include <string.h>

/**
  Author(s): Alan Tao
   Email: alantao912@protonmail.ch
  Phone#: (470) 918 - 7623
*/

#define BLOCK_COMMENT_START 0
#define LINE_COMMENT_START 1

const int num_symbols = 5;
const char *reserved_symbols[] = {"/*", "//", "interface", "while", "continue"};

const int num_int_chars = 6;
const char interrupt_chars[] = {'+', '-', '*', '/', '(', ')'};

enum mode { REGULAR, IDENTIFIER, STRING_LITERAL, NUM_LITERAL, BLOCK_COMMENT, LINE_COMMENT };

Toklist *init_toklist() {
  Toklist *toklist = (Toklist *)malloc(sizeof(Toklist));
  if (!toklist) {
    printf("OOM during lexing, exiting!\n");
    exit(-1);
  }
  toklist->toklist = (Tok *)malloc(8 * sizeof(Tok));
  if (!toklist->toklist) {
    printf("OOM during lexing, exiting!\n");
    exit(-1);
  }
  toklist->capacity = 8;
  toklist->size = 0;
  return toklist;
}

StrBuff *init_strbuff() {
  StrBuff *buff = (StrBuff *)malloc(sizeof(StrBuff));
  if (!buff) {
    printf("OOM during lexing, exiting!\n");
    exit(-1);
  }
  buff->buff = (char *)malloc(8);
  if (!buff->buff) {
    printf("OOM during lexing, exiting!\n");
    exit(-1);
  }
  buff->capacity = 8;
  buff->size = 0;
  return buff;
}

static inline bool resize_strbuff(StrBuff *buff) {
  char *new_buff = (char *)malloc(2 * buff->capacity);
  if (!new_buff) {
    return false;
  }
  memcpy(new_buff, buff->buff, buff->size);
  free(buff->buff);
  buff->buff = new_buff;
  buff->capacity *= 2;
  return true;
}

static inline bool resize_toklist(Toklist *toklist) {
  Tok *new_toklist = (Tok *)malloc(2 * toklist->capacity * sizeof(Tok));
  if (!new_toklist) {
    return false;
  }
  memcpy(new_toklist, toklist->toklist, toklist->size);
  free(toklist->toklist);
  toklist->toklist = new_toklist;
  toklist->capacity *= 2;
  return true;
}

inline void push_strbuff(StrBuff *buff, char c) {
  if (buff->size >= buff->capacity && !resize_strbuff(buff)) {
    printf("OOM during lexing, exiting!\n");
    exit(-1);
  }
  buff->buff[buff->size] = c;
  ++buff->size;
}

inline void pop_strbuff(StrBuff *buff) {
  if (buff->size > 0) {
    --buff->size;
  }
}

inline void set_strbuff(StrBuff *buff, char c) {
  buff->buff[0] = c;
  buff->size = 1;
}

inline void clear_strbuff(StrBuff *buff) { buff->size = 0; }

inline void push_toklist(Toklist *toklist, Tok t) {
  if (toklist->size >= toklist->capacity && !resize_toklist(toklist)) {
    printf("OOM during lexing, exiting!\n");
    exit(-1);
  }
  toklist->toklist[toklist->size] = t;
  ++toklist->size;
}

Toklist *lex(FILE *src) {
  Toklist *toklist = init_toklist();
  const int chunk_size = 1024;
  char *buff = (char *)malloc(chunk_size + 2);
  if (!buff) {
    printf("OOM during lexing, exiting!\n");
    exit(-1);
  }
  buff[chunk_size + 1] = 0;
  StrBuff *char_stack = init_strbuff();

  mode_t state = REGULAR;
  u_int8_t flags = 1;

  size_t n;
  while ((n = fread(buff, 1, chunk_size, src)) != 0) {
    /* Appends a space to the ned of the buffer */
    buff[n++] = ' ';
    int i = 0;
    while (i < n) {
      char c = buff[i];
      switch (state) {
      case REGULAR:
        push_strbuff(char_stack, c);
        if (!should_continue(char_stack)) {
          pop_strbuff(char_stack);
          int match = matches_reserved_symbol(char_stack);
          if (match == BLOCK_COMMENT_START) {
            state = BLOCK_COMMENT;
            clear_strbuff(char_stack);
          } else if (match == LINE_COMMENT_START) {
            state = LINE_COMMENT;
            clear_strbuff(char_stack);
          } else if (match != -1) {
            /* Matches some charsequence*/
            Tok t = {(unsigned char)match, NULL};
            push_toklist(toklist, t);
            clear_strbuff(char_stack);
            if (c != ' ') {
              push_strbuff(char_stack, c);
            }
          } else if (c == ' ') {
            if (char_stack->size) {
              state = IDENTIFIER;
              continue;
            }
          } else if (!is_interrupt_char(c)) {
            /* char_stack holds an identifier */
            push_strbuff(char_stack, c);
            state = IDENTIFIER;
          } else {
            set_strbuff(char_stack, c);
          }
          if (c == '"') {
            state = STRING_LITERAL;
            clear_strbuff(char_stack);
          } else if (is_numeric(c)) {
            state = NUM_LITERAL;
            set_strbuff(char_stack, c);
          }
        }
        break;
      case IDENTIFIER:
        if (c == ' ' || is_interrupt_char(c)) {
          char *identifier = strndup(char_stack->buff, char_stack->size);
          if (!identifier) {
            printf("OOM during lexing, exiting!\n");
            exit(-1);
          }
          Tok t = {0, identifier};
          push_toklist(toklist, t);
          clear_strbuff(char_stack);
          push_strbuff(char_stack, c);
          state = REGULAR;
        } else {
          push_strbuff(char_stack, c);
        }
        break;
      case STRING_LITERAL:
        if (flags == 2) {
          if (c == 't') {
            push_strbuff(char_stack, (char) 0x09);
          } else if (c == 'n') {
            push_strbuff(char_stack, (char) 0x0A);
          } else if (c == 'r') {
            push_strbuff(char_stack, (char) 0x0D);
          } else if (c == '"') {
            push_strbuff(char_stack, '"');
          } else if (c == '\\') {
            push_strbuff(char_stack, '\\');
          }
          flags >>= 1;
        } else if (c == '\\') {
          flags <<= 1;
        } else if (c == '"') {
          char *literal = strndup(char_stack->buff, char_stack->size);
          if (!literal) {
            printf("OOM during lexing, exiting!\n");
            exit(-1);
          }
          Tok t = {1, literal};
          push_toklist(toklist, t);
          clear_strbuff(char_stack);
          state = REGULAR;
        } else {
          push_strbuff(char_stack, c);
        }
        break;
      case NUM_LITERAL:

        break;
      case BLOCK_COMMENT:
        if (flags == 1) {
          if (c == '*') {
            flags <<= 1;
          }
        } else if (flags == 2) {
          if (c == '/') {
            state = REGULAR;
          }
          flags = 1;
        }
        break;
      case LINE_COMMENT:
        if (c == '\n') {
          state = REGULAR;
        }
        break;
      }
      ++i;
    }
  }
  return toklist;
}

int matches_reserved_symbol(const StrBuff *buff) {
  int i = 0;
  while (i < num_symbols) {
    const char *symb = reserved_symbols[i];
    const int l = strlen(symb);

    if (l != buff->size) {
      /* If the length of the string buffer and the reserved symbol do not
       * match, clearly, they cannot be equal. */
      goto END;
    }

    for (int j = 0; j < buff->size; ++j) {
      if (symb[j] != buff->buff[j]) {
        goto END;
      }
    }
    return i;
  END:
    ++i;
  }
  return -1;
}

bool should_continue(const StrBuff *buff) {
  int i = 0;
  while (i < num_symbols) {
    const char *symb = reserved_symbols[i];
    const int l = strlen(symb);

    int j;
    for (j = 0; j < buff->size && j < l; ++j) {
      if (buff->buff[j] != symb[j]) {
        goto END;
      }
    }

    if (j == buff->size) {
      /* If every character in the buffer is a prefix of symb */
      return true;
    }
  END:
    ++i;
  }
  return false;
}

bool is_interrupt_char(char c) {
  for (int i = 0; i < num_int_chars; ++i) {
    if (interrupt_chars[i] == c) {
      return true;
    }
  }
  return false;
}

inline bool is_numeric(char c) { return c >= '0' && c <= '9'; }