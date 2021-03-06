#include <stdlib.h>
#include <string.h>

#include "lenv.h"
#include "lval.h"


lenv* lenv_copy(lenv* le) {
  lenv* copy = malloc(sizeof(lenv));

  copy->length = le->length;
  copy->parent = le->parent;
  copy->symbols = malloc(sizeof(char*) * copy->length);
  copy->lvals = malloc(sizeof(lval*) * copy->length);

  for (int i = 0; i < le->length; i++) {
    copy->symbols[i] = malloc(strlen(le->symbols[i]) + 1);
    strcpy(copy->symbols[i], le->symbols[i]);
    copy->lvals[i] = lval_copy(le->lvals[i]);
  }

  return copy;
}

void lenv_def(lenv* le, lval* key, lval* value) {

  /* iterate till le has no parent */
  while (le->parent) {
    le = le->parent;
  }

  /* Put value in e */
  lenv_put(le, key, value);
}

void lenv_del(lenv* le) {
  for (int i = 0; i < le->length; i++) {
    free(le->symbols[i]);
    lval_del(le->lvals[i]);
  }

  free(le->symbols);
  free(le->lvals);
  free(le);
}

lval* lenv_get(lenv* le, lval* key) {

  do {

    /* iterate over all items in environment */
    for (int i = 0; i < le->length; i++) {
    /* check if the stored string matches the symbol string */
    /* if it does, return a copy of the value */
      if (strcmp(le->symbols[i], key->sym) == 0) {
        return lval_copy(le->lvals[i]);
      }
    }

    le = le->parent;
  }
  while (le);

  /* if no symbol found, check return error */
  return lval_err("Unbound Symbol '%s'", key->sym);
}

lenv* lenv_new(void) {
  lenv* le = malloc(sizeof(lenv));

  le->length = 0;
  le->parent = NULL;
  le->symbols = NULL;
  le->lvals = NULL;
  return le;
}

void lenv_put(lenv* le, lval* key, lval* value) {

  /* iterate over all items in environment */
  /* this is to see if variable already exists */
  for (int i = 0; i < le->length; i++) {

    /* if variable is found delete item at that position */
    /* and replace with variable supplied by user */
    if (strcmp(le->symbols[i], key->sym) == 0) {
      lval_del(le->lvals[i]);
      le->lvals[i] = lval_copy(value);
      return;
    }
  }

  /* if no existing entry found allocate space for new entry */
  le->length++;
  le->lvals = realloc(le->lvals, sizeof(lval*) * le->length);
  le->symbols = realloc(le->symbols, sizeof(char*) * le->length);

  /* copy contents of lval and symbol string into new location */
  le->lvals[le->length - 1] = lval_copy(value);
  le->symbols[le->length - 1] = malloc(strlen(key->sym) + 1);
  strcpy(le->symbols[le->length - 1], key->sym);
}
