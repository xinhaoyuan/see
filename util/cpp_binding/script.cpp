#include "script.hpp"

extern "C"
{
#include <stdio.h>
#include <stdlib.h>
}

ScriptEngine::ScriptEngine(void)
{
    interp_initialize(&mInterp, 16);
}

ScriptEngine::ScriptEngine(int external_args_max)
{
    interp_initialize(&mInterp, external_args_max);
}

ScriptEngine::~ScriptEngine(void)
{
    interp_uninitialize(&mInterp);
}

struct simple_stream
{
    FILE *file;
    int   buf;
};

#define BUF_EMPTY (-2)

static int
simple_stream_in(struct simple_stream *stream, int advance)
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
    
object_t
ScriptEngine::LoadScript(const char *name)
{
    struct simple_stream s;
    if ((s.file = fopen(name, "r")) == NULL) return NULL;
    s.buf  = BUF_EMPTY;

    object_t result = interp_eval(&mInterp, (stream_in_f)simple_stream_in, &s, NULL);

    fclose(s.file);

    return result;
}

int
ScriptEngine::Apply(object_t object, std::vector<object_t> *args)
{
    return interp_apply(&mInterp, object, args->size(), args->empty() ? NULL : &((*args)[0]));
}
    
                        
int
ScriptEngine::Execute(void)
{
    int       ex_argc;
    object_t *ex_args;
    object_t  value = OBJECT_NULL;
        
    int r;
    while (1)
    {
        r = interp_run(&mInterp, value, &ex_argc, &ex_args);

        if (r <= 0)
            break;

        value = OBJECT_NULL;
        switch (r)
        {
        case VM_EXTERNAL_CALL:
        {
            if (OBJECT_TYPE(ex_args[0]) != OBJECT_TYPE_STRING) break;
            std::map<std::string, std::pair<external_function_t, void *> >::iterator
                it = mExCallMap.find(xstring_cstr(ex_args[0]->string));
            if (it != mExCallMap.end())
                value = it->second.first(it->second.second, ex_argc, ex_args);

            int i;
            for (i = 0; i != ex_argc; ++ i)
                interp_unprotect(&mInterp, ex_args[i]);
            
            break;
        }
        case VM_EXTERNAL_LOAD:
        {
            std::map<std::string, std::pair<external_var_op_t, void *> >::iterator
                it = mExVarMap.find(xstring_cstr(mInterp.ex->exp->load.name));
            if (it != mExVarMap.end())
                value = it->second.first(it->second.second, mInterp.ex->exp->load.name, NULL);
            break;
        }
        case VM_EXTERNAL_STORE:
        {
            std::map<std::string, std::pair<external_var_op_t, void *> >::iterator
                it = mExVarMap.find(xstring_cstr(mInterp.ex->exp->store.name));
            if (it != mExVarMap.end())
                value = it->second.first(it->second.second, mInterp.ex->exp->store.name, mInterp.ex->value);
            break;
        }
        case VM_EXTERNAL_CONSTANT:
        {
            std::map<std::string, object_t >::iterator
                it = mExConstMap.find(xstring_cstr(mInterp.ex->exp->constant.name));
            if (it != mExConstMap.end())
                value = it->second;
            break;
        }
        }
    }
        
    return r;
}

void
ScriptEngine::ExternalFuncRegister(const char *name, external_function_t func, void *priv)
{
    mExCallMap[name].first  = func;
    mExCallMap[name].second = priv;
}

object_t
ScriptEngine::ObjectNew(void)
{
    return interp_object_new(&mInterp);
}
    
void
ScriptEngine::ObjectProtect(object_t object)
{
    return interp_protect(&mInterp, object);
}
    
void
ScriptEngine::ObjectUnprotect(object_t object)
{
    return interp_unprotect(&mInterp, object);
}
