#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vm/interp.h"

/* This source file shows the usage of SEE interfaces */

struct simple_stream
{
    FILE *file;
    int   buf;
};

#define BUF_EMPTY (-2)

int simple_stream_in(struct simple_stream *stream, int advance)
{
    int r;
    if (advance)
    {
        if (stream->buf == BUF_EMPTY)
            r = fgetc(stream->file);
        else
        {
            r = stream->buf;
            stream->buf = BUF_EMPTY;
        }
    }
    else
    {
        if (stream->buf == BUF_EMPTY)
        {
            stream->buf = fgetc(stream->file);
            if (stream->buf < 0) stream->buf = -1;
        }
        
        r = stream->buf;
    }
    return r;
}

int main(int argc, const char *args[])
{
    if (argc != 3)
    {
        printf("USAGE: %s [i|s|r] file\n", args[0]);
        return -1;
    }

    char mode = args[1][0];
    struct simple_stream s;
    ast_node_t n;
    interp_s __interp;
    interp_t interp = &__interp;
    object_t prog;
    
    switch (mode)
    {
    case 'i':
    case 's':

        s.file = fopen(args[2], "r");
        s.buf  = BUF_EMPTY;

        n = ast_simple_parse_char_stream((stream_in_f)simple_stream_in, &s);

        fclose(s.file);

        if (mode == 'i')
        {
            ast_dump(n, stdout);
            ast_free(n);
            
            break;
        }

        ast_syntax_parse(n, 0);
        sematic_symref_analyse(n);

        ast_dump(n, stdout);
        ast_free(n);

        break;

    case 'r':

        interp_initialize(interp, 16);
        
        s.file = fopen(args[2], "r");
        s.buf  = BUF_EMPTY;

        prog = interp_eval(interp, (stream_in_f)simple_stream_in, &s, NULL);

        fclose(s.file);

        interp_apply(interp, prog, 0, NULL);
        
        int       ex_argc = 0;
        object_t *ex_args;
        object_t  ex_ret = NULL;
        int i;

        object_t __me = interp_object_new(interp);
        __me->string = xstring_from_cstr("SEE interpreter", -1);
        OBJECT_TYPE_INIT(__me, OBJECT_TYPE_STRING);
        interp_protect(interp, __me);
        
        while (1)
        {
            int r = interp_run(interp, ex_ret, &ex_argc, &ex_args);
            if (r <= 0) break;
            
            switch (r)
            {
            case APPLY_EXTERNAL_CALL:
                ex_ret = OBJECT_NULL;
                /* An example for handling external calls: display */
                if (OBJECT_TYPE(ex_args[0]) == OBJECT_TYPE_STRING)
                {
                    if (xstring_equal_cstr(ex_args[0]->string, "display", -1))
                    {
                        for (i = 1; i != ex_argc; ++ i)
                        {
                            object_dump(ex_args[i], stdout);
                        }
                        printf("\n");
                    }
                }

                /* The caller should unprotect the ex arguments by themself */
                for (i = 0; i != ex_argc; ++ i)
                    interp_unprotect(interp, ex_args[i]);

                break;

            case APPLY_EXTERNAL_CONSTANT:
                /* An example for handling external constant. */
                /* remember that we should never create objects from
                 * heap for the external constant */
                if (xstring_equal_cstr(interp->ex->exp->constant.name, "#answer-to-the-universe", -1))
                    ex_ret = INT_BOX(42);
                else ex_ret = OBJECT_NULL;
                
                break;

            case APPLY_EXTERNAL_LOAD:
                /* Handling external load */
                ex_ret = OBJECT_NULL;
                if (xstring_equal_cstr(interp->ex->exp->load.name, "me", -1))
                {
                    ex_ret = __me;
                }
                break;

            case APPLY_EXTERNAL_STORE:
                /* Handler external store */
                ex_ret = OBJECT_NULL;
                if (xstring_equal_cstr(interp->ex->exp->store.name, "me", -1))
                {
                    fprintf(stderr, "Setting ``me'' is impossible\n");
                    fprintf(stderr, "VALUE: \n");
                    object_dump(interp->ex->value, stderr);
                    fprintf(stderr, "\n");
                }
                break;
                
            default:
                ex_ret = OBJECT_NULL;
                break;
            }
        }
        interp_uninitialize(interp);

        break;
        
    default:

        printf("ERROR OPTION\n");
        return -1;
    }

    return 0;
}
