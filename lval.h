#ifndef LVAL_H_
#define LVAL_H_

/* create enumeration of possible lval types */
enum {
  LVAL_ERR,
  LVAL_NUM,
  LVAL_QEXPR,
  LVAL_SEXPR,
  LVAL_SYM,
};

/* declare new lval struct */
typedef struct lval {
  int type;
  long num;

  /* error and symbol types have some string data */
  char* err;
  char* sym;

  /* length and pointer to a list of "lval*" */
  int length;
  struct lval** cell;
} lval;

lval* lval_err(char*);
lval* lval_num(long);
lval* lval_qexpr(void);
lval* lval_sexpr(void);
lval* lval_sym(char*);

lval* lval_add(lval*, lval*);
void lval_del(lval*);
void lval_expr_print(lval*, char, char);
void lval_print(lval*);
void lval_println(lval*);

#endif
