#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "lval.h"
#include "mpc.h"


/* if we are compiling on Windows compile these functions */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer) + 1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy) - 1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
#endif


lval eval(mpc_ast_t*);
lval eval_op(lval, char*, lval);

lval* lval_read(mpc_ast_t*);
lval* lval_read_num(mpc_ast_t*);


int main(int argc, char** argv) {

  /* Create some parsers */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr  = mpc_new("sexpr");
  mpc_parser_t* Expr   = mpc_new("expr");
  mpc_parser_t* Lispc  = mpc_new("lispc");

  /* Define them with the following language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                          \
      number : /-?[0-9]+/ ;                    \
      symbol : '+' | '-' | '*' | '/' ;         \
      sexpr  : '(' <expr>* ')' ;               \
      expr   : <number> | <symbol> | <sexpr> ; \
      lispc  : /^/ <expr>* /$/ ;               \
    ",
    Number, Symbol, Sexpr, Expr, Lispc
  );

  /* Print version and exit information */
  puts("Lispc Version 0.0.0.0.1");
  puts("Press Ctrl+c to Exit\n");

  /* In a never ending loop */
  while (true) {

    /* Output our prompt and get input */
    char* input = readline("lispc> ");

    /* Add input to history */
    add_history(input);

    /* Attempt to parse the user input */
    mpc_result_t r;

    if (mpc_parse("<stdin>", input, Lispc, &r)) {
      /* On success, print the AST */
      mpc_ast_print(r.output);

      lval* x = lval_read(r.output);
      lval_println(x);
      lval_del(x);

      mpc_ast_delete(r.output);
    } else {
      /* otherwise, print the error */
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    /* Free retrieved input */
    free(input);

  }

  /* Undefine and Delete our Parsers */
  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispc);

  return 0;
}

lval* lval_read_num(mpc_ast_t* tree) {
  errno = 0;
  long num = strtol(tree->contents, NULL, 10);
  return errno != ERANGE ? lval_num(num) : lval_err("invalid number");
}

lval* lval_read(mpc_ast_t* tree) {

  /* if Symbol or Number return conversion to that type */
  if (strstr(tree->tag, "number")) { return lval_read_num(tree); }
  if (strstr(tree->tag, "symbol")) { return lval_sym(tree->contents); }

  /* if root (>) or sexpr then create empty list */
  lval* acc = NULL;

  if (strcmp(tree->tag, ">") == 0) {
    acc = lval_sexpr();
  }

  if (strstr(tree->tag, "sexpr"))  {
    acc = lval_sexpr();
  }

  /* Fill this list with any valid expression contained within */
  for (int i = 0; i < tree->children_num; i++) {
    if (strcmp(tree->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(tree->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(tree->children[i]->tag,  "regex") == 0) { continue; }

    acc = lval_add(acc, lval_read(tree->children[i]));
  }

  return acc;
}
// lval eval(mpc_ast_t* tree) {

//   if (strstr(tree->tag, "number")) {
//     /* Check if there is some error in conversion */
//     errno = 0;
//     long acc = strtol(tree->contents, NULL, 10);
//     return errno != ERANGE ? lval_num(acc) : lval_err(LERR_BAD_NUM);
//   }

//   char* op = tree->children[1]->contents;
//   lval acc = eval(tree->children[2]);

//   for (int i = 3; strstr(tree->children[i]->tag, "expr"); ++i) {
//     acc = eval_op(acc, op, eval(tree->children[i]));
//   }

//   return acc;
// }

// /* Use operator string to see which operation to perform */
// lval eval_op(lval x, char* op, lval y) {

//   /* if either value is an error return it */
//   if (x.type == LVAL_ERR) { return x; }
//   if (y.type == LVAL_ERR) { return y; }

//   /* Otherwise do maths on the number values */
//   if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
//   if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
//   if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
//   if (strcmp(op, "/") == 0) {
//     /* if second operand is zero return error */
//     return y.num == 0
//       ? lval_err(LERR_DIV_ZERO)
//       : lval_num(x.num / y.num);
//   }

//   return lval_err(LERR_BAD_OP);
// }
