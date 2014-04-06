#ifndef             __VALUES_H__
# define            __VALUES_H__

#include "mpc.h"

typedef struct      s_lval {
    int             type;
    long            num;

    /* Error and Symbol types have some string data */
    char*           err;
    char*           sym;

    /* Count and Pointer to a list of "lval*" */
    int             count;
    struct s_lval** cell;
}                   lval;

/* Create Enumeration of Possible lval Types */
enum {
    LVAL_ERR,
    LVAL_NUM,
    LVAL_SYM,
    LVAL_SEXPR
};

/* Create Enumeration of Possible Error Types */
enum {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM
};

lval*           lval_num(long);
lval*           lval_err(char*);
lval*           lval_sym(char*);
lval*           lval_sexpr(void);
lval*           lval_add(lval*, struct lval*);
void            lval_del(lval*);
lval*           lval_read_num(mpc_ast_t*);
lval*           lval_read(mpc_ast_t*);
void            lval_expr_print(lval*, char, char);
void            lval_print(lval*);
void            lval_println(lval*);

#endif
