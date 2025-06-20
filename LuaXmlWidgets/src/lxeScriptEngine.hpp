//
//  lxeScriptEngine.hpp
//  

#ifndef Scripting_hpp
#define Scripting_hpp

#include "lxe.hpp"

namespace lxe{
/*
class Engine;
class ScriptEngine {
private:
    Engine*engine;
    lua_State*state;
    int currentFunctionResultsCount;
public:
    ScriptEngine(class Engine*engine);
    ~ScriptEngine();
    class Engine*getEngine(){return engine;}
    void loadFile(wxString source, wxString fileName);
    void executeExpression(wxString source);
    void runFunctionFromAttribute(const TagAttribute&tagAttribute, int argsCount, int resultsCount, std::function<void(ScriptEngine*)>argProvider,std::function<void(ScriptEngine*)>resultReceiver);
    void runFunction(wxString functionName, int argsCount, int resultsCount, std::function<void(ScriptEngine*)>argProvider,std::function<void(ScriptEngine*)>resultReceiver);
    void runFunctionFromStackTop(int argsCount, int resultsCount, std::function<void(ScriptEngine*)>argProvider,std::function<void(ScriptEngine*)>resultReceiver);
    
    void putStringOnStack(wxString&string);
    void putLongOnStack(long long value);
    void putDoubleOnStack(double value);
    void putBoolOnStack(bool value);
    void putNilOnStack();
    
    wxString getStringResultFromStack(int index);
    long long getLongResultFromStack(int index);
    double getDoubleResultFromStack(int index);
    bool getBoolResultFromStack(int index);
    
    void putPointerToRegistryIndex(const char* key, void*pointer);
    void* getPointerFromRegistryIndex(const char* key);
    //remove lua variable reference to allow value to be collected by GC
    void removeReference(int reference);
    //make a reference from value on stack to prevent GC collect it and be able to store the value in C side
    int createReferenceFromStackTop();
    void putReferenceOnStackTop(int reference);
    
    void prepareSystemTables();
};
*/
}
#endif /* lxeScriptEngine_hpp */
