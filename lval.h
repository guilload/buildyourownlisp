#ifndef LVAL_H_
#define LVAL_H_

/* Create Enumeration of Possible Error Types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* Create Enumeration of Possible lval Types */
enum { LVAL_NUM, LVAL_ERR };

/* Declare New lval Struct */
typedef struct {
  int type;
  long num;
  int err;
} lval;

lval lval_err(int);
lval lval_num(long);
void lval_print(lval);
void lval_println(lval);

#endif
