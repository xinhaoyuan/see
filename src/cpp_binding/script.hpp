#ifndef __SCRIPT_HPP__
#define __SCRIPT_HPP__

#include <vector>
#include <map>
#include <string>

extern "C" {
#include <vm/interp.h>
}

class ScriptEngine
{
    interp_s mInterp;
    
    typedef object_t(*external_function_t)(void *priv, int argc, object_t *argv);
    std::map<std::string, std::pair<external_function_t, void *> > mExMap;
        
public:
    ScriptEngine(void);
    ScriptEngine(int external_args_max);

    object_t LoadScript(const char *name);
    int Apply(object_t object, std::vector<object_t> *args);
    int Execute(object_t value, std::vector<object_t> *excall);
        
    object_t ObjectNew(void);
    void     ObjectProtect(object_t object);
    void     ObjectUnprotect(object_t object);

    void ExternalFuncRegister(const char *name, external_function_t func, void *priv);

    ~ScriptEngine(void);
};

#endif
