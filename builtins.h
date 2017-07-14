#ifndef BUILTINS_H_
#define BUILTINS_H_

#include "lval.h"

lval* builtin(lval*, char*);
lval* builtin_eval(lval*);
lval* builtin_head(lval*);
lval* builtin_join(lval*);
lval* builtin_list(lval*);
lval* builtin_op(lval*, char*);
lval* builtin_tail(lval*);

#endif
