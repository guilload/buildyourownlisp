#ifndef LENV_H_
#define LENV_H_

struct lval;

typedef struct lenv {
  int length;

  char** symbols;
  struct lval** lvals;
} lenv;


lenv* lenv_new(void);

struct lval* lenv_get(lenv* e, struct lval* k);

void lenv_del(lenv*);
void lenv_put(lenv*, struct lval* key, struct lval* value);

#endif
