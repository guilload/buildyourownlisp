#include <string.h>

#include "builtins.h"
#include "lenv.h"
#include "lval.h"


lval* lval_eval(lenv*, lval*);
lval* lval_pop(lval*, int);
lval* lval_take(lval*, int);

#define LASSERT(args, cond, err) \
  if (!(cond)) { lval_del(args); return lval_err(err); }


lval* builtin_add(lenv* le, lval* lv) {
  return builtin_op(le, lv, "+");
}

lval* builtin_def(lenv* le, lval* lv) {
  LASSERT(lv, lv->cell[0]->type == LVAL_QEXPR,
    "Function 'def' passed incorrect type!");

  /* first argument is symbol list */
  lval* symbols = lv->cell[0];

  /* ensure all elements of first list are symbols */
  for (int i = 0; i < symbols->length; i++) {
    LASSERT(lv, symbols->cell[i]->type == LVAL_SYM,
      "Function 'def' cannot define non-symbol");
  }

  /* check correct number of symbols and values */
  LASSERT(lv, symbols->length == lv->length-1,
    "Function 'def' cannot define incorrect "
    "number of values to symbols");

  /* assign copies of values to symbols */
  for (int i = 0; i < symbols->length; i++) {
    lenv_put(le, symbols->cell[i], lv->cell[i + 1]);
  }

  lval_del(lv);
  return lval_sexpr();
}


lval* builtin_div(lenv* le, lval* lv) {
  return builtin_op(le, lv, "/");
}

lval* builtin_eval(lenv* le, lval* lv) {
  LASSERT(lv, lv->length == 1,
    "Function 'eval' passed too many arguments!");
  LASSERT(lv, lv->cell[0]->type == LVAL_QEXPR,
    "Function 'eval' passed incorrect type!");

  lval* x = lval_take(lv, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(le, x);
}

lval* builtin_head(lenv* le, lval* lv) {
  LASSERT(lv, lv->length == 1,
    "Function 'head' passed too many arguments!");
  LASSERT(lv, lv->cell[0]->type == LVAL_QEXPR,
    "Function 'head' passed incorrect type!");
  LASSERT(lv, lv->cell[0]->length != 0,
    "Function 'head' passed empty {}!");

  lval* head = lval_take(lv, 0);

  while (head->length > 1) {
    lval_del(lval_pop(head, 1));
  }

  return head;
}

lval* builtin_join(lenv* le, lval* lv) {
  for (int i = 0; i < lv->length; i++) {
    LASSERT(lv, lv->cell[i]->type == LVAL_QEXPR,
      "Function 'join' passed incorrect type.");
  }

  lval* acc = lval_pop(lv, 0);

  while (lv->length) {
    acc = lval_join(acc, lval_pop(lv, 0));
  }

  lval_del(lv);
  return acc;
}

lval* builtin_list(lenv* le, lval* lv) {
  lv->type = LVAL_QEXPR;
  return lv;
}

lval* builtin_mul(lenv* le, lval* lv) {
  return builtin_op(le, lv, "*");
}

lval* builtin_op(lenv* le, lval* lv, char* op) {

  /* ensure all arguments are numbers */
  for (int i = 0; i < lv->length; i++) {
    if (lv->cell[i]->type != LVAL_NUM) {
      lval_del(lv);
      return lval_err("Cannot operate on non-number!");
    }
  }

  /* pop the first element */
  lval* acc = lval_pop(lv, 0);

  /* if no arguments and sub then perform unary negation */
  if ((strcmp(op, "-") == 0) && lv->length == 0) {
    acc->num = -acc->num;
  }

  /* while there are still elements remaining */
  while (lv->length > 0) {

    /* pop the next element */
    lval* current = lval_pop(lv, 0);

    if (strcmp(op, "+") == 0) { acc->num += current->num; }
    if (strcmp(op, "-") == 0) { acc->num -= current->num; }
    if (strcmp(op, "*") == 0) { acc->num *= current->num; }
    if (strcmp(op, "/") == 0) {

      if (current->num == 0) {
        lval_del(acc);
        lval_del(current);
        acc = lval_err("Division by zero!");
        break;
      }

      acc->num /= current->num;
    }

    lval_del(current);
  }

  lval_del(lv);
  return acc;
}

lval* builtin_sub(lenv* le, lval* lv) {
  return builtin_op(le, lv, "-");
}

lval* builtin_tail(lenv* le, lval* lv) {
  LASSERT(lv, lv->length == 1,
    "Function 'tail' passed too many arguments!");
  LASSERT(lv, lv->cell[0]->type == LVAL_QEXPR,
    "Function 'tail' passed incorrect type!");
  LASSERT(lv, lv->cell[0]->length != 0,
    "Function 'tail' passed {}!");

  lval* acc = lval_take(lv, 0);
  lval_del(lval_pop(acc, 0));
  return acc;
}

void lenv_add_builtin(lenv* le, char* name, lbuiltin func) {
  lval* key = lval_sym(name);
  lval* value = lval_func(func);

  lenv_put(le, key, value);

  lval_del(key);
  lval_del(value);
}

void lenv_add_builtins(lenv* le) {
  /* List functions */
  lenv_add_builtin(le, "eval", builtin_eval);
  lenv_add_builtin(le, "head", builtin_head);
  lenv_add_builtin(le, "join", builtin_join);
  lenv_add_builtin(le, "list", builtin_list);
  lenv_add_builtin(le, "tail", builtin_tail);

  /* Mathematical functions */
  lenv_add_builtin(le, "+", builtin_add);
  lenv_add_builtin(le, "/", builtin_div);
  lenv_add_builtin(le, "*", builtin_mul);
  lenv_add_builtin(le, "-", builtin_sub);

  /* variable functions */
  lenv_add_builtin(le, "def",  builtin_def);
}
