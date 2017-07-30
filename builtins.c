#include <string.h>

#include "builtins.h"
#include "lenv.h"
#include "lval.h"


// FIXME
lval* lval_eval(lenv*, lval*);
lval* lval_pop(lval*, int);
lval* lval_take(lval*, int);


#define LASSERT(args, cond, fmt, ...) \
  if (!(cond)) { \
    lval* err = lval_err(fmt, ##__VA_ARGS__); \
    lval_del(args); return err; \
  }

#define LASSERT_NOT_EMPTY(func, args, index) \
  LASSERT(args, args->cell[index]->length != 0, \
    "Function '%s' passed {} for argument %i.", func, index);

#define LASSERT_NUM(func, args, num) \
  LASSERT(args, args->length == num, \
    "Function '%s' passed incorrect number of arguments. Got %i, expected %i.", \
    func, args->length, num)

#define LASSERT_TYPE(func, args, index, expect) \
  LASSERT(args, args->cell[index]->type == expect, \
    "Function '%s' passed incorrect type for argument %i. Got %s, expected %s.", \
    func, index, ltype_name(args->cell[index]->type), ltype_name(expect))


lval* builtin_add(lenv* le, lval* lv) {
  return builtin_op(le, lv, "+");
}

lval* builtin_def(lenv* le, lval* la) {
  return builtin_var(le, la, "def");
}

lval* builtin_div(lenv* le, lval* lv) {
  return builtin_op(le, lv, "/");
}

lval* builtin_lambda(lenv* le, lval* la) {
  /* check two arguments, each of which are Q-Expressions */
  LASSERT_NUM("\\", la, 2);
  LASSERT_TYPE("\\", la, 0, LVAL_QEXPR);
  LASSERT_TYPE("\\", la, 1, LVAL_QEXPR);

  /* check first Q-Expression contains only symbols */
  for (int i = 0; i < la->cell[0]->length; i++) {
    LASSERT(la, (la->cell[0]->cell[i]->type == LVAL_SYM),
      "Cannot define non-symbol. Got %s, Expected %s.",
      ltype_name(la->cell[0]->cell[i]->type), ltype_name(LVAL_SYM));
  }

  /* pop first two arguments and pass them to lval_lambda */
  lval* args = lval_pop(la, 0);
  lval* body = lval_pop(la, 0);
  lval_del(la);

  return lval_lambda(args, body);
}

lval* builtin_eval(lenv* le, lval* lv) {
  LASSERT_NUM("eval", lv, 1);
  LASSERT_TYPE("eval", lv, 0, LVAL_QEXPR);

  lval* x = lval_take(lv, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(le, x);
}

lval* builtin_head(lenv* le, lval* lv) {
  LASSERT_NUM("head", lv, 1);
  LASSERT_TYPE("head", lv, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("head", lv, 0);

  lval* head = lval_take(lv, 0);

  while (head->length > 1) {
    lval_del(lval_pop(head, 1));
  }

  return head;
}

lval* builtin_join(lenv* le, lval* lv) {
  for (int i = 0; i < lv->length; i++) {
    LASSERT_TYPE("join", lv, i, LVAL_QEXPR);
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
    LASSERT_TYPE(op, lv, i, LVAL_NUM);
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

lval* builtin_put(lenv* le, lval* la) {
  return builtin_var(le, la, "=");
}

lval* builtin_sub(lenv* le, lval* lv) {
  return builtin_op(le, lv, "-");
}

lval* builtin_tail(lenv* le, lval* lv) {
  LASSERT_NUM("tail", lv, 1);
  LASSERT_TYPE("tail", lv, 0, LVAL_QEXPR);
  LASSERT_NOT_EMPTY("tail", lv, 0);

  lval* acc = lval_take(lv, 0);
  lval_del(lval_pop(acc, 0));
  return acc;
}

lval* builtin_var(lenv* le, lval* la, char* func) {
  LASSERT_TYPE(func, la, 0, LVAL_QEXPR);

  lval* symbols = la->cell[0];

  for (int i = 0; i < symbols->length; i++) {
    LASSERT(la, (symbols->cell[i]->type == LVAL_SYM),
      "Function '%s' cannot define non-symbol. "
      "Got %s, Expected %s.", func,
      ltype_name(symbols->cell[i]->type),
      ltype_name(LVAL_SYM));
  }

  LASSERT(la, (symbols->length == la->length-1),
    "Function '%s' passed too many arguments for symbols. "
    "Got %i, Expected %i.", func, symbols->length, la->length - 1);

  for (int i = 0; i < symbols->length; i++) {
    /* If 'def' define in globally. If 'put' define in locally */
    if (strcmp(func, "def") == 0) {
      lenv_def(le, symbols->cell[i], la->cell[i + 1]);
    }

    if (strcmp(func, "=")   == 0) {
      lenv_put(le, symbols->cell[i], la->cell[i + 1]);
    }
  }

  lval_del(la);
  return lval_sexpr();
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
  lenv_add_builtin(le, "=",   builtin_put);
  lenv_add_builtin(le, "\\", builtin_lambda);
  lenv_add_builtin(le, "def",  builtin_def);
}
