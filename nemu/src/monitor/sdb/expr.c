/***************************************************************************************
* Copyright (c) 2014-2024 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <memory/vaddr.h>

enum {
  TK_NOTYPE = 256, TK_NUM, TK_REG,
  TK_EQ, TK_NQ,
  TK_AND, TK_OR,
  TK_NEG, TK_POS, TK_DEREF,
  TK_LEFT_PARENTHESIS, TK_RIGHT_PARENTHESIS
};

static int unary_prefix[] = {
  TK_EQ, TK_NQ,
  TK_AND, TK_OR,
  TK_NEG, TK_POS, TK_DEREF,
  TK_LEFT_PARENTHESIS
};

static int unary_operators[] = {
  TK_NEG, TK_POS, TK_DEREF,
};

bool token_in(int token, int tokens[], int len) {
  for (int i = 0; i < len; i ++) {
    if (token == tokens[i]) return true;
  }
  return false;
}

static struct rule {
  const char *regex;
  int token_type;
} rules[] = {
  {" +", TK_NOTYPE},   // spaces
  {"0x[0-9a-fA-F]+|[[:digit:]]+", TK_NUM},   // number
  {"$[0-9a-zA-Z_]+", TK_REG},   // register
  {"\\+", '+'},         // plus
  {"-", '-'},           // minus
  {"\\*", '*'},         // multiply
  {"/", '/'},           // divide
  {"\\(", TK_LEFT_PARENTHESIS},
  {"\\)", TK_RIGHT_PARENTHESIS},
  {"==", TK_EQ},        // equal
  {"!=", TK_NQ},        // not equal
  {"\\&\\&", TK_AND},       // logical and
  {"\\|\\|", TK_OR},        // logical or
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

static Token tokens[32] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        if (rules[i].token_type != TK_NOTYPE && nr_token >= ARRLEN(tokens)) {
          fprintf(stderr, "too many tokens in one expression\n");
          return false;
        }

        switch (rules[i].token_type) {
          case '+': case '-': case '*':
            // recognize unary operator
            if (nr_token == 0 || token_in(rules[i].token_type, unary_prefix, ARRLEN(unary_prefix))) {
              switch (rules[i].token_type)
              {
              case '+': tokens[nr_token].type = TK_POS; break;
              case '-': tokens[nr_token].type = TK_NEG; break;
              case '*': tokens[nr_token].type = TK_DEREF; break;
              }
              nr_token ++;
              break;
            }
            // not unary, passthrough
          case '/':
          case TK_LEFT_PARENTHESIS: case TK_RIGHT_PARENTHESIS:
          case TK_EQ: case TK_NQ: case TK_AND: case TK_OR:
            // just add to tokens
            tokens[nr_token].type = rules[i].token_type;
            nr_token ++;
            break;
          case TK_NUM: case TK_REG:
            // record the str
            if (substr_len > 31) {
              fprintf(stderr, "too long token: %.*s\n", substr_len, substr_start);
              return false;
            }
            tokens[nr_token].type = rules[i].token_type;
            memcpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token ++;
            break;
          default: 
            // do nothing
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      fprintf(stderr, "no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}


word_t eval(int start, int end, bool *success);
word_t expr(char *e, bool *success) {
  if (!make_token(e)) {
    fprintf(stderr, "make token error\n");
    *success = false;
    return 0;
  }

  *success = true;
  return eval(0, nr_token-1, success);
}

bool check_parentheses(int start, int end, bool *success) {
  if (tokens[start].type != TK_LEFT_PARENTHESIS) {
    return false;
  }

  if (tokens[end].type != TK_RIGHT_PARENTHESIS) {
    return false;
  }

  int left = 0;
  for (int i = start; i <= end; i ++) {
    if (tokens[i].type == TK_LEFT_PARENTHESIS) {
      left ++;
    }
    if (tokens[i].type == TK_RIGHT_PARENTHESIS) {
      left --;
    }
    if (left < 0) {
      *success = false;
      return false;
    }
    if (left == 0 && i != end) {
      return false;
    }
  }

  if (left != 0) {
    *success = false;
    return false;
  }

  return true;
}

int find_main_operator(int start, int end, bool *success);
word_t eval(int start, int end, bool *success) {
  // no need to eval due to previous error
  if (!*success) return 0;

  if (start > end) {
    fprintf(stderr, "start > end, start=%d, end=%d\n", start, end);
    *success = false;
    return 0;
  }

  if (start == end) {
    // single token, should be a num or register
    switch (tokens[start].type) {
      case TK_NUM:
        if (tokens[start].str[0] == '0' && tokens[start].str[1] == 'x') {
          // hex
          return strtol(tokens[start].str, NULL, 16);
        }
        // decimal
        return strtol(tokens[start].str, NULL, 10);
      case TK_REG:
        return isa_reg_str2val(tokens[start].str+1, success);
    }
    fprintf(stderr, "unsupported single token: %s\n", tokens[start].str);
    *success = false;
    return 0;
  }
  
  if (check_parentheses(start, end, success)) {
    // just drop the parentheses
    return eval(start+1, end-1, success);
  }

  // no need to eval due to previous error
  if (!*success) {
    fprintf(stderr, "check_parentheses error, start=%d, end=%d\n", start, end);
    return 0;
  }

  // eval recursively
  int op = find_main_operator(start, end, success);
  if (!*success) {
    fprintf(stderr, "find_main_operator error, start=%d, end=%d\n", start, end);
    return 0;
  }

  // unary
  if (token_in(tokens[op].type, unary_operators, ARRLEN(unary_operators))) {
    word_t val2 = eval(op+1, end, success); if (!*success) return 0;
    switch (tokens[op].type) {
      case TK_POS: return val2;
      case TK_NEG: return -val2;
      case TK_DEREF: return vaddr_read(val2, 8);
      default: fprintf(stderr, "Unsupported operator: %c\n", tokens[op].type);
    }
  }
  word_t val1 = eval(start, op-1, success); if (!*success) return 0;
  word_t val2 = eval(op+1, end, success); if (!*success) return 0;
  switch (tokens[op].type) {
    case '+':
      return val1 + val2;
    case '-':
      return val1 - val2;
    case '*':
      return val1 * val2;
    case '/':
      return (sword_t) val1 / (sword_t) val2;
    case TK_AND:
      return val1 && val2;
    case TK_OR:
      return val1 || val2;
    case TK_EQ:
      return val1 == val2;
    case TK_NQ:
      return val1 != val2;
  }
  fprintf(stderr, "Unsupported operator: %c\n", tokens[op].type);
  *success = false;
  return 0;
}

static struct {
  int op;
  int priority;
} operatorPriority[] = {
  {TK_OR, 1},
  {TK_AND, 2},
  {TK_EQ, 3},
  {TK_NQ, 3},
  {'+', 5},
  {'-', 5},
  {'*', 6},
  {'/', 6},
  {TK_POS, 7},
  {TK_NEG, 7},
  {TK_DEREF, 7},
};

int find_main_operator(int start, int end, bool *success) {
  int pos = -1;
  int posPriority = -1;
  int parentheses = 0;
  for (int i = start; i <= end; i ++) {
    if (tokens[i].type == TK_LEFT_PARENTHESIS) {
      parentheses ++;
      continue;
    }
    if (tokens[i].type == TK_RIGHT_PARENTHESIS) {
      parentheses --;
      continue;
    }
    if (parentheses != 0) continue;
    int currentPriority = -1;
    for (int j = 0; j < ARRLEN(operatorPriority); j ++) {
      if (tokens[i].type == operatorPriority[j].op) {
        currentPriority = operatorPriority[j].priority;
        break;
      }
    }
    if (currentPriority == -1) continue;
    if (posPriority == -1 || currentPriority <= posPriority) {
      pos = i;
      posPriority = currentPriority;
    }
  }
  if (pos == -1) {
    *success = false;
    return 0;
  }

  return pos;
}