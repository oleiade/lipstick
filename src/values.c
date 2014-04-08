#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "values.h"

/* Create a new number type lval */
lval*       lval_num(long x) {
        lval*   v;

        v = malloc(sizeof(lval));
        v->type = LVAL_NUM;
        v->num = x;

        return v;
}

/* Create a new error type lval */
lval*       lval_err(char* m) {
        lval*   v;

        v = malloc(sizeof(lval));
        v->type = LVAL_ERR;
        v->err = malloc(strlen(m) + 1);
        strcpy(v->err, m);

        return v;
}

lval*       lval_sym(char* s)
{
        lval*   v;

        v = malloc(sizeof(lval));
        v->type = LVAL_SYM;
        v->sym = malloc(strlen(s) + 1);
        strcpy(v->sym, s);

        return v;
}

lval*       lval_sexpr(void)
{
        lval*   v;

        v = malloc(sizeof(lval));
        v->type = LVAL_SEXPR;
        v->count = 0;
        v->cell = NULL;

        return v;
}

lval*       lval_add(lval* v, lval* x) {
        v->count++;
        v->cell = realloc(v->cell, sizeof(lval*) * v->count);
        v->cell[v->count - 1] = x;

        return v;
}

void        lval_del(lval* v) {
        switch (v->type) {
                case LVAL_NUM:
                        break;
                case LVAL_ERR:
                        free(v->err);
                        break;
                case LVAL_SYM:
                        free(v->sym);
                        break;
                case LVAL_SEXPR:
                        for (int i = 0; i < v->count; i++)
                        lval_del(v->cell[i]);
                        free(v->cell);
                break;
        }

        free(v);
}

lval*       lval_read_num(mpc_ast_t* t) {
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err("invalid number");
}

lval*       lval_read(mpc_ast_t* t) {
        /* If symbol or number return conversion to that type */ 
        if (strstr(t->tag, "number"))
                return lval_read_num(t);
        if (strstr(t->tag, "symbol"))
                return lval_sym(t->contents);

        /* If root (>) or sexpr then create empty list */
        lval* x = NULL;
        if (strcmp(t->tag, ">") == 0)
                x = lval_sexpr();
        if (strstr(t->tag, "sexpr"))
                x = lval_sexpr();

        /* Fill this list with any valid expression contained within */
        for (int i = 0; i < t->children_num; i++) {
                if (strcmp(t->children[i]->contents, "(") == 0)
                        continue;
                if (strcmp(t->children[i]->contents, ")") == 0)
                        continue;
                if (strcmp(t->children[i]->contents, "}") == 0)
                        continue;
                if (strcmp(t->children[i]->contents, "{") == 0)
                        continue;
                if (strcmp(t->children[i]->contents, "regex") == 0)
                        continue;
                x = lval_add(x, lval_read(t->children[i]));
        }

        return x;
}

lval*           lval_eval_sexpr(lval* v) {
        /* Evaluate Children */
        for (int i = 0; i < v->count; i++) {
                if (v->cell[i] != NULL)
                        v->cell[i] = lval_eval(v->cell[i]);
        }

        /* Error Checking */
        for (int i = 0; i < v->count; i++) {
                if (v->cell[i] != NULL) {
                        if (v->cell[i]->type == LVAL_ERR)
                                return lval_take(v, i);
                }
        }

        /* Empty Expression */
        if (v->count == 0)
                return v;

        /* Single Expression */
        if (v->count == 1)
                return lval_take(v, 0);

        /* Ensure First Element is Symbol */
        lval* f = lval_pop(v, 0);
        if (f->type != LVAL_SYM) {
                lval_del(f);
                lval_del(v);

                return lval_err("S-expression Does not start with symbol!");
        }        

        /* Call builtin with operator */
        lval* result = builtin_op(v, f->sym);
        lval_del(f);

        return result;
}

lval*           lval_pop(lval* v, int i) {
        /* Find the item at "i" */
        lval* x = v->cell[i];

        /* Shift the memory following the item at "i" over the top of it */
        memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

        /* Decrease the count of items in the list */
        v->count--;

        /* Reallocate the memory used */
        v->cell = realloc(v->cell, sizeof(lval*) * v->count);

        return x;
}

lval*           lval_take(lval* v, int i) {
        lval* x = lval_pop(v, i);
        lval_del(v);

        return x;
}

lval*       builtin_op(lval* a, char* op) {
        /* Ensure all arguments are numbers */
        for (int i = 0; i < a->count; i++) {
                if (a->cell[i]->type != LVAL_NUM) {
                        lval_del(a);
                        return lval_err("Cannot operator on non number!");
                }
        }

        /* Pop the first element */
        lval* x = lval_pop(a, 0);

        /* If no arguments and sub then perform unary negation */
        if ((strcmp(op, "-") == 0) && a->count == 0)
                x->num = -(x->num);

        /* While there are still elements remaining */
        while (a->count > 0) {
                /* Pop the next element */
                lval* y = lval_pop(a, 0);

                /* Perform operation */
                if (strcmp(op, "+") == 0)
                        x->num += y->num;
                if (strcmp(op, "-") == 0)
                        x->num -= y->num;
                if (strcmp(op, "*") == 0)
                        x->num *= y->num;
                if (strcmp(op, "/") == 0) {
                        if (y->num == 0) {
                                lval_del(x); lval_del(y); lval_del(a);
                                x = lval_err("Division By Zero!"); break;
                        } else {
                                x->num /= y->num;
                        }
                }

                /* Delete element now finished with */
                lval_del(y);
        }

        /* Delete input expression and return result */
        lval_del(a);
        return x;
}

lval* lval_eval(lval* v) {
        if (v != NULL) {
                /* Evaluate Sexpressions */
                if (v->type == LVAL_SEXPR)
                        return lval_eval_sexpr(v);
        }

        /* All other lval types remain the same */
        return v;
}

/* Print an "lval" */
void        lval_print(lval* v) {
        switch (v->type) {
                case LVAL_NUM:
                        printf("%li", v->num);
                        break;
                case LVAL_ERR:
                        printf("%s", v->err);
                case LVAL_SYM:
                        printf("%s", v->sym);
                        break;
                case LVAL_SEXPR:
                        lval_expr_print(v, '(', ')');
                        break;
                break;
        }
}

void lval_expr_print(lval* v, char open, char close) {
        putchar(open);

        for (int i = 0; i < v->count; i++) {
                lval_print(v->cell[i]);
                if (i != (v->count - 1))
                        putchar(' ');
        }

        putchar(close);
}

void        lval_println(lval* v) {
        lval_print(v);
        putchar('\n');
}
