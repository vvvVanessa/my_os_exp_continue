#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>

enum {
  TK_NOTYPE = 256, TK_AND, TK_EQ, TK_NE,

  /* TODO: Add more token types */
  TK_ADD, TK_SUB, TK_MUL, TK_DIV,
  TK_REG, TK_DEC, TK_HEX, TK_LP, TK_RP,
  DEREF
};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"&&", TK_AND},
  {"==", TK_EQ},         // equal
  {"!=", TK_NE},
  {"\\+", TK_ADD},         // plus
  {"\\$[a-zA-Z]+", TK_REG},
  {"0[xX][0-9a-fA-F]+", TK_HEX},
  {"-?[0-9]+", TK_DEC},
  {"-", TK_SUB},
  {"\\*", TK_MUL},
  {"/", TK_DIV},
  {"\\(", TK_LP},
  {"\\)", TK_RP}
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

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

Token tokens[32];
int nr_token;

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

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
            case TK_AND:
                tokens[nr_token].type = TK_AND;
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                ++nr_token;
                break;
            case TK_EQ:
                tokens[nr_token].type = TK_EQ;
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                ++nr_token;
                break;
            case TK_NE:
                tokens[nr_token].type = TK_NE;
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                ++nr_token;
                break;
            case TK_ADD:
                tokens[nr_token].type = TK_ADD;
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                ++nr_token;
                break;
            case TK_SUB:
                tokens[nr_token].type = TK_SUB;
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                ++nr_token;
                break;
            case TK_DIV:
                tokens[nr_token].type = TK_DIV;
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                ++nr_token;
                break;
            case TK_MUL:
                tokens[nr_token].type = TK_MUL;
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                ++nr_token;
                break;
            case TK_REG:
                tokens[nr_token].type = TK_REG;
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                ++nr_token;
                break;
            case TK_DEC:
                tokens[nr_token].type = TK_DEC;
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                ++nr_token;
                break;
            case TK_HEX:
                tokens[nr_token].type = TK_HEX;
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                ++nr_token;
                break;
            case TK_LP:
                tokens[nr_token].type = TK_LP;
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                ++nr_token;
                break;
            case TK_RP:
                tokens[nr_token].type = TK_RP;
                strncpy(tokens[nr_token].str, substr_start, substr_len);
                ++nr_token;
                break;
            case TK_NOTYPE:
                break;
            default:
                printf("make_token: unknown token type.\n");
                return false;
        }
        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

bool BAD_EXPRESSION_SIGNAL;

bool check_parentheses(int p, int q) {
    if (BAD_EXPRESSION_SIGNAL)
        return 0;
    int i;
    int top = 1;
    bool ret = true;
    if (tokens[p].type != TK_LP || tokens[q].type != TK_RP)
        return false;

    for (i = p + 1; i <= q; i++) {
        if (top == 0) 
            ret = false;
        if (tokens[i].type == TK_LP) {
            ++top;
        } else if (tokens[i].type == TK_RP) {
            if (top == 0) {
                printf("check_parentheses: bad expression. \n");
                BAD_EXPRESSION_SIGNAL = true;
                return false;
            } else {
                --top;
            }
        }
    }
    if (top != 0) {
        printf("check_parentheses: bad expression. \n");
        BAD_EXPRESSION_SIGNAL = true;
        return false;
    }
    return ret;
}

int find_main_op(int p, int q) {
    if (BAD_EXPRESSION_SIGNAL)
        return 0;
    int i;
    int ret = -1;
    int type = 0xfff;
    int in_par = 0;
    for (i = p; i <= q; i++) {
        if (tokens[i].type == TK_LP) 
            ++in_par;
        else if (tokens[i].type == TK_RP)
            --in_par;
        if (in_par != 0) 
            continue;
        if (tokens[i].type == TK_AND ||
            tokens[i].type == TK_EQ ||
            tokens[i].type == TK_NE ||
            tokens[i].type == TK_ADD ||
            tokens[i].type == TK_SUB ||
            tokens[i].type == TK_MUL ||
            tokens[i].type == TK_DIV) {
            if (type >= tokens[i].type) {
                type = tokens[i].type;
                ret = i;
            }
        }
    }
    return ret;
}

uint32_t eval(int p, int q) {
    if (BAD_EXPRESSION_SIGNAL)
        return 0;

    int i;
    if (p > q) {
        printf("eval: bad expression. \n");
        BAD_EXPRESSION_SIGNAL = true;
        return 0;
    } else if (p == q) {
        uint32_t ret = 0;
        int amp = 1;
        if (tokens[p].type == TK_DEC) {
            int low = 0;
            if (tokens[p].str[0] == '-') {
                amp *= -1;
                low = 1;
            }
            for (i = strlen(tokens[p].str) - 1; i >= low; i--) {
                ret += amp * (tokens[p].str[i] - '0');
                amp *= 10;
            }
            return ret;
        } else if (tokens[p].type == TK_HEX) {
            /* heximal must be positive */
            for (i = strlen(tokens[p].str) - 1; i >= 0; i--) {
                if (tokens[p].str[i] >= 'a' && tokens[p].str[i] <= 'f') {
                    ret += amp * (tokens[p].str[i] - 'a');
                } else if (tokens[p].str[i] >= 'A' && tokens[p].str[i] <= 'F') {
                    ret += amp * (tokens[p].str[i] - 'A');
                } else if (tokens[p].str[i] >= '0' && tokens[p].str[i] <= '9') {
                    ret += amp * (tokens[p].str[i] - '0');
                }
                amp *= 16;
            }
            return ret;
        } else if (tokens[p].type == TK_REG) {
            char* reg_name = tokens[p].str + 1;
            int i;
            for (i = 0; i < 8; i++) {
                if (!strcmp(reg_name, regsl[i])) {
                    return reg_l(i);
                } else if (!strcmp(reg_name, regsw[i])) {
                    return reg_w(i);
                } else if (!strcmp(reg_name, regsb[i])) {
                    return reg_b(i);
                }
            }
            /* wrong reg name */
            printf("eval: wrong register name. \n");
            BAD_EXPRESSION_SIGNAL = true;
            return 0;
        } else {
            printf("eval: wrong base type. \n");
            BAD_EXPRESSION_SIGNAL = true;
            return 0;
        }
    } else if (check_parentheses(p, q)) {
        return eval(p + 1, q - 1);
    } else if (tokens[p].type == DEREF && find_main_op(p + 1, q) == -1) {
        return vaddr_read(eval(p + 1, q), 1);
    } else {
        int op = find_main_op(p, q);
        if (op == -1) {
            /* legal main operator not found */
            printf("eval: legal main operator not found. \n");
            BAD_EXPRESSION_SIGNAL = true;
            return 0;
        }
        Log("find op at %d", op);
        uint32_t val1 = eval(p, op - 1);
        uint32_t val2 = eval(op + 1, q);
        Log("val1: %d val2: %d", val1, val2);
        switch (tokens[op].type) {
            case TK_AND: return val1 && val2;
            case TK_EQ: return val1 == val2;
            case TK_NE: return val1 != val2; 
            case TK_ADD: return val1 + val2;
            case TK_SUB: return val1 - val2;
            case TK_MUL: return val1 * val2;
            case TK_DIV: 
                 if (val2 == 0) {
                     printf("Divisor cannot be zero. \n");
                     BAD_EXPRESSION_SIGNAL = true;
                     return 0;
                 } else {
                     return val1 / val2;
                 }
            default:
                 printf("eval: wrong operator. \n");
                 BAD_EXPRESSION_SIGNAL = true;
                 return 0;
        }
    }
    return 0;
}

uint32_t expr(char *e, bool *success) {
  memset(tokens, 0, sizeof(tokens));
  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  int i;
  for (i = 0; i < nr_token; i++) {
      if (tokens[i].type == TK_MUL && (i == 0 || (tokens[i - 1].type == TK_EQ ||
                                                  tokens[i - 1].type == TK_LP ||
                                                  tokens[i - 1].type == TK_NE ||
                                                  tokens[i - 1].type == TK_AND ||
                                                  tokens[i - 1].type == TK_ADD ||
                                                  tokens[i - 1].type == TK_SUB ||
                                                  tokens[i - 1].type == TK_DIV ||
                                                  tokens[i - 1].type == TK_MUL))) {
          tokens[i].type = DEREF;
          Log("pos %d type deref", i);
      }
  }
  /* TODO: Insert codes to evaluate the expression. */
  BAD_EXPRESSION_SIGNAL = false;
  uint32_t val = eval(0, nr_token - 1);
  if (BAD_EXPRESSION_SIGNAL) {
      *success = false;
      return 0;
  }
  return val;
}
