#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "builtins.h"
#include "lval.h"


lval* lval_pop(lval*, int);


char* ltype_name(int type) {
  switch(type) {
    case LVAL_FUNC:
      return "Function";

    case LVAL_NUM:
      return "Number";

    case LVAL_ERR:
      return "Error";

    case LVAL_SYM:
      return "Symbol";

    case LVAL_SEXPR:
      return "S-Expression";

    case LVAL_QEXPR:
      return "Q-Expression";

    default:
      return "Unknown";
  }
}

/* call a function */
lval* lval_call(lenv* le, lval* func, lval* la) {

  /* if builtin then simply apply that */
  if (func->builtin) {
    return func->builtin(le, la);
  }

  /* save argument length */
  int provided = la->length;
  int expected = func->args->length;

  /* while arguments still remain to be processed */
  while (la->length) {

    /* if we've ran out of formal arguments to bind */
    if (func->args->length == 0) {
      lval_del(la);

      return lval_err(
        "Function passed too many arguments. "
        "Got %i, Expected %i.", provided, expected
      );
    }

    /* pop the first symbol from the args */
    lval* symbol = lval_pop(func->args, 0);

    /* special case to deal with '&' */
    if (strcmp(symbol->sym, "&") == 0) {

      /* ensure '&' is followed by another symbol */
      if (func->args->length != 1) {
        lval_del(la);

        return lval_err(
          "Function format invalid. "
          "Symbol '&' not followed by single symbol."
        );
      }

      /* next arg should be bound to remaining arguments */
      lval* nsymbol = lval_pop(func->args, 0);
      lenv_put(func->env, nsymbol, builtin_list(le, la));

      lval_del(symbol);
      lval_del(nsymbol);
      break;
    }

    /* pop the next argument from the list */
    lval* value = lval_pop(la, 0);

    /* bind a copy into the function's environment */
    lenv_put(func->env, symbol, value);

    /* delete symbol and value */
    lval_del(symbol);
    lval_del(value);
  }

  /* argument list is now bound so can be cleaned up */
  lval_del(la);

  /* if '&' remains in arg list bind to empty list */
  if (func->args->length > 0 &&
      strcmp(func->args->cell[0]->sym, "&") == 0) {

    /* Check to ensure that & is not passed invalidly. */
    if (func->args->length != 2) {
      return lval_err(
        "Function format invalid. "
        "Symbol '&' not followed by single symbol."
      );
    }

    /* pop and delete '&' symbol */
    lval_del(lval_pop(func->args, 0));

    /* pop next symbol and create empty list */
    lval* symbol = lval_pop(func->args, 0);
    lval* value = lval_qexpr();

    /* bind to environment and delete */
    lenv_put(func->env, symbol, value);

    lval_del(symbol);
    lval_del(value);
  }

  /* If all args have been bound evaluate */
  if (func->args->length == 0) {

    /* Set environment parent to evaluation environment */
    func->env->parent = le;

    /* Evaluate and return */
    return builtin_eval(
      func->env,
      lval_add(lval_sexpr(), lval_copy(func->body))
    );
  } else {
    /* Otherwise return partially evaluated function */
    return lval_copy(func);
  }

}

/* copy a lval */
lval* lval_copy(lval* lv) {

  lval* copy = malloc(sizeof(lval));
  copy->type = lv->type;

  switch (lv->type) {

    /* Copy functions and numbers directly */
    case LVAL_FUNC:
      if (lv->builtin) {
        copy->builtin = lv->builtin;
      } else {
        copy->builtin = NULL;
        copy->env = lenv_copy(lv->env);
        copy->args = lval_copy(lv->args);
        copy->body = lval_copy(lv->body);
      }
      break;

    case LVAL_NUM:
      copy->num = lv->num;
      break;

    /* Copy strings using malloc and strcpy */
    case LVAL_ERR:
      copy->err = malloc(strlen(lv->err) + 1);
      strcpy(copy->err, lv->err);
      break;

    case LVAL_SYM:
      copy->sym = malloc(strlen(lv->sym) + 1);
      strcpy(copy->sym, lv->sym);
      break;

    /* Copy lists by copying each sub-expression recursively */
    case LVAL_SEXPR:
    case LVAL_QEXPR:
      copy->length = lv->length;
      copy->cell = malloc(sizeof(lval*) * copy->length);

      for (int i = 0; i < copy->length; i++) {
        copy->cell[i] = lval_copy(lv->cell[i]);
      }

      break;
  }

  return copy;
}

/* construct a pointer to a new Error lval */
lval* lval_err(char* fmt, ...) {
  lval* lv = malloc(sizeof(lval));
  lv->type = LVAL_ERR;

  /* Create a va list and initialize it */
  va_list va;
  va_start(va, fmt);

  /* Allocate 512 bytes of space */
  lv->err = malloc(512);

  /* printf the error string with a maximum of 511 characters */
  vsnprintf(lv->err, 511, fmt, va);

  /* Reallocate to number of bytes actually used */
  lv->err = realloc(lv->err, strlen(lv->err)+1);

  /* Cleanup our va list */
  va_end(va);

  return lv;
}

/* construct a pointer to a new Func lval */
lval* lval_func(lbuiltin func) {
  lval* lv = malloc(sizeof(lval));
  lv->type = LVAL_FUNC;
  lv->builtin = func;
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

lval* lval_lambda(lval* args, lval* body) {
  lval* lv = malloc(sizeof(lval));
  lv->type = LVAL_FUNC;

  /* set func to null */
  lv->builtin = NULL;

  /* build new environment */
  lv->env = lenv_new();

  /* set args and body */
  lv->args = args;
  lv->body = body;

  return lv;
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
      if (!lv->builtin) {
        lenv_del(lv->env);
        lval_del(lv->args);
        lval_del(lv->body);
      }
      break;

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
      if (lv->builtin) {
        printf("<func>");
      } else {
        printf("(\\ ");
        lval_print(lv->args);
        putchar(' ');
        lval_print(lv->body);
        putchar(')');
      }
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
