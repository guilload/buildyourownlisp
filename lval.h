#ifndef LVAL_H_
#define LVAL_H_

#include "lenv.h"

/* Forward declarations */
struct lval;
typedef struct lval lval;

typedef lval*(*lbuiltin)(lenv*, lval*);

/* declare new lval struct */
struct lval {
  int type;

  long num;

  /* error and symbol types have some string data */
  char* err;
  char* sym;

  lbuiltin func;

  /* length and pointer to a list of "lval*" */
  int length;
  struct lval** cell;
};


/* create enumeration of possible lval types */
enum {
  LVAL_ERR,
  LVAL_FUNC,
  LVAL_NUM,
  LVAL_QEXPR,
  LVAL_SEXPR,
  LVAL_SYM,
};

char* ltype_name(int);

lval* lval_add(lval*, lval*);
lval* lval_copy(lval*);
lval* lval_err(char*, ...);
lval* lval_func(lbuiltin);
lval* lval_join(lval*, lval*);
lval* lval_num(long);
lval* lval_qexpr(void);
lval* lval_sexpr(void);
lval* lval_sym(char*);

void lval_del(lval*);
void lval_expr_print(lval*, char, char);
void lval_print(lval*);
void lval_println(lval*);

#endif
