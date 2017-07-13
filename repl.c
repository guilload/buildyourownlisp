#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "lval.h"
#include "mpc.h"


/* if we are compiling on Windows compile these functions */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer) + 1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy) - 1] = '\0';
  return cpy;
}

/* fake add_history function */
void add_history(char* unused) {}

/* otherwise include the editline headers */
#else
#include <editline/readline.h>
#endif


lval* builtin_op(lval*, char*);
lval* lval_eval(lval*);
lval* lval_pop(lval*, int);
lval* lval_read(mpc_ast_t*);
lval* lval_read_num(mpc_ast_t*);
lval* lval_take(lval*, int);


int main(int argc, char** argv) {

  /* create some parsers */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr  = mpc_new("sexpr");
  mpc_parser_t* Qexpr  = mpc_new("qexpr");
  mpc_parser_t* Expr   = mpc_new("expr");
  mpc_parser_t* Lispc  = mpc_new("lispc");

  /* define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                    \
      number : /-?[0-9]+/ ;                              \
      symbol : '+' | '-' | '*' | '/' ;                   \
      sexpr  : '(' <expr>* ')' ;                         \
      qexpr  : '{' <expr>* '}' ;                         \
      expr   : <number> | <symbol> | <sexpr> | <qexpr> ; \
      lispc  : /^/ <expr>* /$/ ;                         \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Lispc);

  /* print version and exit information */
  puts("Lispc Version 0.0.0.0.1");
  puts("Press Ctrl+c to Exit\n");

  /* in a never ending loop */
  while (true) {

    /* output our prompt and get input */
    char* input = readline("lispc> ");

    /* add input to history */
    add_history(input);

    /* attempt to parse the user input */
    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Lispc, &r)) {

      lval* x = lval_eval(lval_read(r.output));
      lval_println(x);
      lval_del(x);

      mpc_ast_delete(r.output);
    } else {
      /* otherwise, print the error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    /* free retrieved input */
    free(input);

  }

  /* undefine and delete our parsers */
  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Lispc);

  return 0;
}

lval* lval_read_num(mpc_ast_t* tree) {
  errno = 0;
  long num = strtol(tree->contents, NULL, 10);
  return errno != ERANGE ? lval_num(num) : lval_err("Invalid number");
}

lval* lval_read(mpc_ast_t* tree) {

  /* if Symbol or Number return conversion to that type */
  if (strstr(tree->tag, "number")) {
    return lval_read_num(tree);
  }

  if (strstr(tree->tag, "symbol")) {
    return lval_sym(tree->contents);
  }

  /* if root (>) or sexpr then create empty list */
  lval* acc = NULL;

  if (strcmp(tree->tag, ">") == 0) {
    acc = lval_sexpr();
  }

  if (strstr(tree->tag, "qexpr"))  {
    acc = lval_qexpr();
  }

  if (strstr(tree->tag, "sexpr"))  {
    acc = lval_sexpr();
  }

  /* fill this list with any valid expression contained within */
  for (int i = 0; i < tree->children_num; i++) {
    if (strcmp(tree->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(tree->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(tree->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(tree->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(tree->children[i]->tag,  "regex") == 0) { continue; }

    acc = lval_add(acc, lval_read(tree->children[i]));
  }

  return acc;
}

lval* lval_eval_sexpr(lval* lv) {
  /* evaluate children */
  for (int i = 0; i < lv->length; i++) {
    lv->cell[i] = lval_eval(lv->cell[i]);
  }

  /* error checking */
  for (int i = 0; i < lv->length; i++) {
    if (lv->cell[i]->type == LVAL_ERR) {
      return lval_take(lv, i);
    }
  }

  /* empty expression */
  if (lv->length == 0) {
    return lv;
  }

  /* single expression */
  if (lv->length == 1) {
    return lval_take(lv, 0);
  }

  /* ensure first element is Symbol */
  lval* first = lval_pop(lv, 0);

  if (first->type != LVAL_SYM) {
    lval_del(first);
    lval_del(lv);
    return lval_err("S-expression does not start with symbol!");
  }

  /* call builtin with operator */
  lval* acc = builtin_op(lv, first->sym);
  lval_del(first);

  return acc;
}

lval* lval_eval(lval* lv) {
  /* evaluate S-expressions */
  if (lv->type == LVAL_SEXPR) {
    return lval_eval_sexpr(lv);
  }

  /* all other lval types remain the same */
  return lv;
}

lval* lval_pop(lval* lv, int i) {
  /* find the item at "i" */
  lval* x = lv->cell[i];

  /* shift memory after the item at "i" over the top */
  memmove(&lv->cell[i], &lv->cell[i + 1], sizeof(lval*) * (lv->length - i - 1));

  /* decrease the length of items in the list */
  lv->length--;

  /* reallocate the memory used */
  lv->cell = realloc(lv->cell, sizeof(lval*) * lv->length);
  return x;
}

lval* lval_take(lval* lv, int i) {
  lval* x = lval_pop(lv, i);
  lval_del(lv);
  return x;
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
