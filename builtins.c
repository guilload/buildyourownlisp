#include <string.h>

#include "builtins.h"
#include "lval.h"


lval* lval_eval(lval*);
lval* lval_pop(lval*, int);
lval* lval_take(lval*, int);

#define LASSERT(args, cond, err) \
  if (!(cond)) { lval_del(args); return lval_err(err); }


lval* builtin(lval* lv, char* func) {
  if (strcmp("eval", func) == 0) { return builtin_eval(lv); }
  if (strcmp("head", func) == 0) { return builtin_head(lv); }
  if (strcmp("join", func) == 0) { return builtin_join(lv); }
  if (strcmp("list", func) == 0) { return builtin_list(lv); }
  if (strcmp("tail", func) == 0) { return builtin_tail(lv); }
  if (strstr("+-/*", func)) { return builtin_op(lv, func); }

  lval_del(lv);
  return lval_err("Unknown Function!");
}

lval* builtin_eval(lval* lv) {
  LASSERT(lv, lv->length == 1,
    "Function 'eval' passed too many arguments!");
  LASSERT(lv, lv->cell[0]->type == LVAL_QEXPR,
    "Function 'eval' passed incorrect type!");

  lval* x = lval_take(lv, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(x);
}

lval* builtin_head(lval* lv) {
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

lval* builtin_join(lval* lv) {
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

lval* builtin_list(lval* lv) {
  lv->type = LVAL_QEXPR;
  return lv;
}

lval* builtin_op(lval* lv, char* op) {

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

lval* builtin_tail(lval* lv) {
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
