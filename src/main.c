#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"

#include "values.h"

int         main()
{
        /* Create Some Parsers */
        mpc_parser_t* Number = mpc_new("number");
        mpc_parser_t* Symbol = mpc_new("symbol");
        mpc_parser_t* Sexpr  = mpc_new("sexpr");
        mpc_parser_t* Expr   = mpc_new("expr");
        mpc_parser_t* Lispy  = mpc_new("lispy");
  
        mpca_lang(MPC_LANG_DEFAULT,
        "                                          \
        number : /-?[0-9]+/ ;                    \
        symbol : '+' | '-' | '*' | '/' ;         \
        sexpr  : '(' <expr>* ')' ;               \
        expr   : <number> | <symbol> | <sexpr> ; \
        lispy  : /^/ <expr>* /$/ ;               \
        ",
        Number, Symbol, Sexpr, Expr, Lispy);

        /* Print Version and Exit Information */
        puts("Lispy Version 0.0.1");
        puts("Press Ctrl+c to Exit\n");

        /* In a never ending loop */
        while (1) {
                char* input = readline("lispy> ");
                add_history(input);

                /* Attempt to parse the user input */
                mpc_result_t r;
                if (mpc_parse("<stdin>", input, Lispy, &r)) {
                        /* On success print and delete the AST */
                        lval* result = lval_eval(lval_read(r.output));
                        lval_println(result);
                        lval_del(result);
                } else {
                        /* Otherwise print and delete the Error */
                        mpc_err_print(r.error);
                        mpc_err_delete(r.error);
                }

                /* Free retrived input */
                free(input);
        }

        mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

        return 0;
}
