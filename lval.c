#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lval.h"


lval* lval_pop(lval*, int);


/* construct a pointer to a new Error lval */
lval* lval_err(char* message) {
  lval* lv = (lval*) malloc(sizeof(lval));
  lv->type = LVAL_ERR;
  lv->err = malloc(strlen(message) + 1);
  strcpy(lv->err, message);
  return lv;
}

/* construct a pointer to a new Func lval */
lval* lval_func(lbuiltin func) {
  lval* lv = malloc(sizeof(lval));
  lv->type = LVAL_FUNC;
  lv->func = func;
  return lv;
}

lval* lval_join(lval* x, lval* y) {
  /* for each cell in 'y' add it to 'x' */
  while (y->length) {
    x = lval_add(x, lval_pop(y, 0));
  }

  /* delete the empty 'y' and return 'x' */
  lval_del(y);
  return x;
}

/* construct a pointer to a new empty Qexpr lval */
lval* lval_qexpr(void) {
  lval* lv = malloc(sizeof(lval));
  lv->type = LVAL_QEXPR;
  lv->length = 0;
  lv->cell = NULL;
  return lv;
}

/* construct a pointer to a new Number lval */
lval* lval_num(long num) {
  lval* lv = malloc(sizeof(lval));
  lv->type = LVAL_NUM;
  lv->num = num;
  return lv;
}

/* construct a pointer to a new empty Sexpr lval */
lval* lval_sexpr(void) {
  lval* lv = malloc(sizeof(lval));
  lv->type = LVAL_SEXPR;
  lv->length = 0;
  lv->cell = NULL;
  return lv;
}

/* construct a pointer to a new Symbol lval */
lval* lval_sym(char* sym) {
  lval* lv = malloc(sizeof(lval));
  lv->type = LVAL_SYM;
  lv->sym = malloc(strlen(sym) + 1);
  strcpy(lv->sym, sym);
  return lv;
}

/* append a lval to another one */
lval* lval_add(lval* this, lval* that) {
  this->length++;
  this->cell = realloc(this->cell, sizeof(lval*) * this->length);
  this->cell[this->length - 1] = that;
  return this;
}

/* deallocate a lval struct */
void lval_del(lval* lv) {

  switch (lv->type) {
    /* do nothing special for func and number type */
    case LVAL_FUNC:
    case LVAL_NUM: break;

    /* for Err or Sym free the string data */
    case LVAL_ERR: free(lv->err); break;
    case LVAL_SYM: free(lv->sym); break;

    /* if Qexpr or Sexpr then delete all elements inside */
    case LVAL_QEXPR:
    case LVAL_SEXPR:
      for (int i = 0; i < lv->length; i++) {
        lval_del(lv->cell[i]);
      }
      /* also free the memory allocated to contain the pointers */
      free(lv->cell);
    break;
  }

  /* free the memory allocated for the "lval" struct itself */
  free(lv);
}

void lval_expr_print(lval* lv, char open, char close) {
  putchar(open);

  for (int i = 0; i < lv->length; i++) {

    /* print value contained within */
    lval_print(lv->cell[i]);

    /* don't print trailing space if last element */
    if (i != (lv->length - 1)) {
      putchar(' ');
    }
  }

  putchar(close);
}

/* print an "lval" */
void lval_print(lval* lv) {
  switch (lv->type) {

    case LVAL_ERR:
      printf("Error: %s", lv->err);
      break;

    case LVAL_FUNC:
      printf("<function>");
      break;

    case LVAL_NUM:
      printf("%li", lv->num);
      break;

    case LVAL_QEXPR:
      lval_expr_print(lv, '{', '}');
      break;

    case LVAL_SEXPR:
      lval_expr_print(lv, '(', ')');
      break;

    case LVAL_SYM:
      printf("%s", lv->sym);
      break;
  }
}

/* print an "lval" followed by a newline */
void lval_println(lval* lv) {
  lval_print(lv);
  putchar('\n');
}
