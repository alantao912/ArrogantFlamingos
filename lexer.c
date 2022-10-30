#include "lexer.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

/**
  Author(s): Alan Tao
   Email: alantao912@protonmail.ch
  Phone#: (470) 918 - 7623
*/

#define NO_MATCH -1
#define BLOCK_COMMENT_START 0
#define LINE_COMMENT_START 1
#define END_OF_FILE 255

const int num_symbols = 61;
const char *reserved_symbols[] = {"/*", "//", "+", "-", "*", "/", "%", "++", "+=", "--", "-=", "*=", "/=", 
                                  "%=", "(", ")", "[", "]", "{", "}", "!", "&&", "||", "<", ">", "<=", ">=",                                       "==", "!=", "&", "|", "~", "^", "&=", "|=", "~=", "^=", "=", ".", ",", ";",
                                  ":", "?", "for", "while", "do", "break", "continue", "if", "else", "switch",                                     "case", "default", "const", "struct", "def", "self", "return", "let", "true",                                    "false"};

// |, &, =, /, *, non interrupt if in interrupt mode
const int num_allowable_dups = 7;
const char interrupt_chars[] = { '|', '&', '=', '*', '/', '+', '-', '%', '"', '.', ',', 
                                '^', '!', '~', ':', ';', '<', '>', '?', '[', 
                                ']', '{', '}', '(', ')', 0};

enum mode { REGULAR, STRING_LITERAL, NUM_LITERAL, BLOCK_COMMENT, LINE_COMMENT };

toklist *init_toklist() {
  toklist *toklist = (struct toklist *) malloc(sizeof(toklist));
  if (!toklist) {
    printf("OOM during lexing, exiting!\n");
    exit(-1);
  }
  toklist->toklist = (tok *) malloc(8 * sizeof(tok));
  if (!toklist->toklist) {
    printf("OOM during lexing, exiting!\n");
    exit(-1);
  }
  toklist->capacity = 8;
  toklist->size = 0;
  return toklist;
}

StrBuff *init_strbuff() {
  StrBuff *buff = (StrBuff *) malloc(sizeof(StrBuff));
  if (!buff) {
    printf("OOM during lexing, exiting!\n");
    exit(-1);
  }
  buff->buff = (char *) malloc(8);
  if (!buff->buff) {
    printf("OOM during lexing, exiting!\n");
    exit(-1);
  }
  buff->capacity = 8;
  buff->size = 0;
  return buff;
}

static inline bool resize_strbuff(StrBuff *buff) {
  char *new_buff = (char *) malloc(2 * buff->capacity);
  if (!new_buff) {
    return false;
  }
  memcpy(new_buff, buff->buff, buff->size);
  free(buff->buff);
  buff->buff = new_buff;
  buff->capacity *= 2;
  return true;
}

static inline bool resize_toklist(toklist *toklist) {
  tok *new_toklist = (tok *) malloc(2 * toklist->capacity * sizeof(tok));
  if (!new_toklist) {
    return false;
  }
  memcpy(new_toklist, toklist->toklist, toklist->size * sizeof(tok));
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

inline void push_toklist(toklist *toklist, tok t) {
  if (toklist->size >= toklist->capacity && !resize_toklist(toklist)) {
    printf("OOM during lexing, exiting!\n");
    exit(-1);
  }
  toklist->toklist[toklist->size] = t;
  ++toklist->size;
}

char *strndup(const char *src, size_t size) {
  size_t len = strnlen (src, size);
  char *new = malloc (len + 1);
  if (new == NULL) {
    return NULL;
  }
  new[len] = '\0';
  return memcpy (new, src, len);
}

toklist *lex(FILE *src) {
  toklist *toklist = init_toklist();
  const int chunk_size = 1024;
  char *buff = (char *) malloc(chunk_size + 2);
  if (!buff) {
    printf("OOM during lexing, exiting!\n");
    exit(-1);
  }
  buff[chunk_size + 1] = 0;
  StrBuff *char_stack = init_strbuff();

  enum mode state = REGULAR;
  uint8_t flags = 1;

  size_t n;
  while ((n = fread(buff, 1, chunk_size, src)) != 0) {
    /* Appends a space to the ned of the buffer */
    buff[n++] = ' ';
    int i = 0;
    while (i < n) {
      char c = buff[i];
      switch (state) {
      case REGULAR:
        if (is_interrupt_char(c) && char_stack->size) {
          int match = matches_reserved_symbol(char_stack);
          if (match == NO_MATCH) {
            /* Identifier has been found */
            char *identifier = strndup(char_stack->buff, char_stack->size);
            if (!identifier) {
              printf("OOM during lexing, exiting!");
              exit(-1);
            }
            tok t = {0, identifier};
            push_toklist(toklist, t);
            clear_strbuff(char_stack);
            if (c != ' ') push_strbuff(char_stack, c);
          } else if (match == BLOCK_COMMENT_START) {
            /* Encountered the start of a block comment */
            clear_strbuff(char_stack);
            state = BLOCK_COMMENT;
          } else if (match == LINE_COMMENT_START) {
            /* Encountered double slash identifying line comment */
            clear_strbuff(char_stack);
            state = LINE_COMMENT;
          } else {
            /* Encountered a reserved word or symbol */
            tok t = {.type = match, .value = NULL};
            push_toklist(toklist, t);
            clear_strbuff(char_stack);
            if (c != ' ') push_strbuff(char_stack, c);
          }

          if (is_numeric(c)) {
            set_strbuff(char_stack, c);
            state = NUM_LITERAL;
          }
        } else if (char_stack->size == 0 && is_numeric(c)) {
          set_strbuff(char_stack, c);
          state = NUM_LITERAL;
        } else if (c != ' ') {
          /* Pushing non space character to the buffer */
          push_strbuff(char_stack, c);
        }
        
        if (c == '"') {
          /* Encountered an open quote. Start of string literal. */
          clear_strbuff(char_stack);
          state = STRING_LITERAL;
        }
        break;
      case STRING_LITERAL:
        if (flags == 2) {
          switch(c) {
            case 'a':
            push_strbuff(char_stack, (char) (0x07));
            break;
            case 'b':
            push_strbuff(char_stack, (char) (0x08));
            break;
            case 'e':
            push_strbuff(char_stack, (char) (0x1B));
            break; 
            case 'f':
            push_strbuff(char_stack, (char) (0x0C));
            break;
            case 'n':
            push_strbuff(char_stack, (char) (0x0A));
            break;
            case 'r':
            push_strbuff(char_stack, (char) (0x0D));
            break;
            case 't':
            push_strbuff(char_stack, (char) (0x09));
            break;
            case 'v':
            push_strbuff(char_stack, (char) (0x0B));
            break;
            case '\\':
            push_strbuff(char_stack, (char) (0x5C));
            break; 
            case '\'':
            push_strbuff(char_stack, (char) (0x27));
            break;
            case '"':
            push_strbuff(char_stack, (char) (0x22));
            break;
            case '?':
            push_strbuff(char_stack, (char) (0x3F));
            break;
            default:
            push_strbuff(char_stack, c);
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
          tok t = {1, literal};
          push_toklist(toklist, t);
          clear_strbuff(char_stack);
          flags = 1;
          state = REGULAR;
        } else {
          push_strbuff(char_stack, c);
        }
        break;
      case NUM_LITERAL:
        if (is_numeric(c)) {
          push_strbuff(char_stack, c);
        } else if (c == '.' && flags == 1) {
          /* First time a period character is encountered */
          push_strbuff(char_stack, c);
          ++flags;
        } else {
          char *value = strndup(char_stack->buff, char_stack->size);
          if (!value) {
            printf("OOM during lexing, exiting!\n");
            exit(-1);
          }
          tok t = {2, value};
          push_toklist(toklist, t);
          clear_strbuff(char_stack);
          if (c != ' ') push_strbuff(char_stack, c);
          flags = 1;
          state = REGULAR;
        }
        break;
      case BLOCK_COMMENT:
        if (flags == 1) {
          if (c == '*') {
            flags <<= 1;
          }
        } else if (flags == 2) {
          if (c == '/') {
            flags = 1;
            state = REGULAR;
          } else {
            flags >>= 1;
          }
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
  tok t = {END_OF_FILE, NULL};
  push_toklist(toklist, t);
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
  return NO_MATCH;
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
  static bool isInterruptMode = false;
  if (isInterruptMode) {
    return !(isInterruptMode = !(is_alpha(c) || is_numeric(c) || c == ' ' || is_in(c, &interrupt_chars[num_allowable_dups])));
  } else {
    return (isInterruptMode = is_in(c, interrupt_chars)) || c == ' ';
  }
}

bool is_in(char c, const char set[]) {
  int i = 0;
  while (set[i]) {
    if (set[i] == c) return true;
    ++i;
  }
  return false;
}

inline bool is_alpha(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool is_numeric(char c) { return c >= '0' && c <= '9'; }

void show_toklist(const toklist *toklist) {
  printf("Showing toklist %d\n", toklist->size);
  for (int i = 0; i < toklist->size - 1; ++i) {
    tok t = toklist->toklist[i];
    if (!t.value) {
      printf("Reserved symbol: '%s' %d\n", reserved_symbols[t.type], t.type);
    } else if (t.type == 0) {
      printf("Identifier: '%s'\n", (char *) t.value);
    } else if (t.type == 1) {
      printf("String literal: '%s'\n", (char *) t.value);
    } else if (t.type == 2) {
      printf("Numerical literal: '%s'\n", (char *) t.value);
    }
  }
}