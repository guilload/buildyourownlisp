#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpc.h"


/* If we are compiling on Windows compile these functions */
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


long eval(mpc_ast_t*);
long eval_op(long, char*, long);


int main(int argc, char** argv) {

  /* Create some parsers */
  mpc_parser_t* Number   = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr     = mpc_new("expr");
  mpc_parser_t* Lispc    = mpc_new("lispc");

  /* Define them with the following language */
  mpca_lang(
    MPCA_LANG_DEFAULT,
    "                                                   \
    number   : /-?[0-9]+/ ;                             \
    operator : '+' | '-' | '*' | '/' ;                  \
    expr     : <number> | '(' <operator> <expr>+ ')' ;  \
    lispc    : /^/ <operator> <expr>+ /$/ ;             \
    ",
    Number, Operator, Expr, Lispc
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

      long ret = eval(r.output);
      printf("eval: %li\n", ret);

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
  mpc_cleanup(4, Number, Operator, Expr, Lispc);

  return 0;
}

long eval(mpc_ast_t* tree) {

  /* If tagged as number return it directly. */
  if (strstr(tree->tag, "number")) {
    return atoi(tree->contents);
  }

  /* The operator is always second child. */
  char* op = tree->children[1]->contents;

  /* We store the third child in `x` */
  long acc = eval(tree->children[2]);

  /* Iterate the remaining children and combining. */
  for (int i = 3; strstr(tree->children[i]->tag, "expr"); ++i) {
    acc = eval_op(acc, op, eval(tree->children[i]));
  }

  return acc;
}

/* Use operator string to see which operation to perform */
long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  return 0;
}
