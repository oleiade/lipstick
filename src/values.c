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

lval*       lval_add(lval* v, struct lval* x) {
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
                                if (v->cell[i])
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

lval*           lval_read(mpc_ast_t* t) {

        /* If Symbol or Number return conversion to that type */
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
                if (strcmp(t->children[i]->tag,  "regex") == 0)
                        continue;
                x = lval_add(x, lval_read(t->children[i]));
        }

        return x;
}

/* Print an "lval" */
void        lval_print(lval* v) {
        if (v) {
                switch (v->type) {
                        case LVAL_NUM:
                                printf("%li", v->num);
                                break;
                        case LVAL_ERR:
                                printf("Error: %s", v->err);
                                break;
                        case LVAL_SYM:
                                printf("%s", v->sym);
                                break;
                        case LVAL_SEXPR:
                                lval_expr_print(v, '(', ')');
                                break;
                }
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
