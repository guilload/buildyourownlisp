#ifndef BUILTINS_H_
#define BUILTINS_H_

#include "lenv.h"
#include "lval.h"


lval* builtin_add(lenv*, lval*);
lval* builtin_def(lenv*, lval*);
lval* builtin_div(lenv*, lval*);
lval* builtin_eval(lenv*, lval*);
lval* builtin_head(lenv*, lval*);
lval* builtin_join(lenv*, lval*);
lval* builtin_list(lenv*, lval*);
lval* builtin_mul(lenv*, lval*);
lval* builtin_op(lenv*, lval*, char*);
lval* builtin_sub(lenv*, lval*);
lval* builtin_tail(lenv*, lval*);

void lenv_add_builtin(lenv*, char*, lbuiltin func);
void lenv_add_builtins(lenv*);

#endif
