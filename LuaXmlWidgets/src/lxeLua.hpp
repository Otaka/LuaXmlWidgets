//  LuaWrapper.hpp
//  LuaXmlWidgets
#ifndef LuaWrapper_h
#define LuaWrapper_h

#include "lxe.hpp"

namespace lxe {

class Lua;

wxString dumpStack(lua_State * state);
class NativeError {
public:
    NativeError(wxString error) {
        this->errorMessage=error;
    }
    wxString errorMessage;
};

typedef struct{
    int ref;
} FunctionRef;
typedef struct{
    int ref;
} TableRef;


enum ValueType {LTYPE_INT, LTYPE_DOUBLE, LTYPE_BOOL, LTYPE_TABLE, LTYPE_STRING, LTYPE_FUNCTION, LTYPE_USERDATA, LTYPE_NIL, LTYPE_OTHER};
class ExecBuilder;
class TableReader;
class TableWriter;
class TableReaderWriter;
class ValuesListReader;
class ValuesListWriter;

typedef std::function<void(ValuesListReader*args, ValuesListWriter*retValues)> NativeFunction;

ValueType getLuaTypeOnTop(lua_State*state);
ValueType getTypeFromLuaType(int type);

class TableReader {
    lua_State*state;
    Lua*lua;
public:
    TableReader(Lua*lua,lua_State*state) {this->lua=lua; this->state=state;}
    
    bool exists(wxString key) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_gettable(state, -2);
        bool isPresent=!lua_isnil(state, -1);
        lua_pop(state, 1);
        return isPresent;
    }
    /**
        Make a reference to function that is currently on top of stack. Reference will prevent gc to collect it.
     */
    FunctionRef getFunctionRef(int key) {
        lua_pushnumber(state, key);
        lua_gettable(state, -2);
        int ref=luaL_ref(state, LUA_REGISTRYINDEX);
        return {ref};
    }
    /**
        Make a reference to function that is currently on top of stack. Reference will prevent gc to collect it.
     */
    FunctionRef getFunctionRef(wxString key) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_gettable(state, -2);
        int ref=luaL_ref(state, LUA_REGISTRYINDEX);
        return {ref};
    }
    
    /**
        Make a reference to table that is currently on top of stack. Reference will prevent gc to collect it.
     */
    TableRef getTableRef(int key) {
        lua_pushnumber(state, key);
        lua_gettable(state, -2);
        int ref=luaL_ref(state, LUA_REGISTRYINDEX);
        return {ref};
    }
    /**
        Make a reference to table that is currently on top of stack. Reference will prevent gc to collect it.
     */
    TableRef getTableRef(wxString key) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_gettable(state, -2);
        int ref=luaL_ref(state, LUA_REGISTRYINDEX);
        return {ref};
    }

    ValueType getType(int key){
        lua_pushnumber(state, key);
        lua_gettable(state, -2);
        ValueType type= getLuaTypeOnTop(state);
        lua_pop(state, 1);
        return type;
    }
    ValueType getType(wxString key){
        lua_pushstring(state, key.ToUTF8().data());
        lua_gettable(state, -2);
        ValueType type= getLuaTypeOnTop(state);
        lua_pop(state, 1);
        return type;
    }

    int getInt(int key) {
        lua_pushnumber(state, key);
        lua_gettable(state, -2);
        int value=lua_tonumber(state, -1);
        lua_pop(state,1);
        return value;
    }
    
    int getInt(wxString key) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_gettable(state, -2);
        int value=lua_tonumber(state, -1);
        lua_pop(state,1);
        return value;
    }
    
    double getDouble(int key) {
        lua_pushnumber(state, key);
        lua_gettable(state, -2);
        double value=lua_tonumber(state, -1);
        lua_pop(state,1);
        return value;
    }
    
    double getDouble(wxString key) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_gettable(state, -2);
        double value=lua_tonumber(state, -1);
        lua_pop(state,1);
        return value;
    }
    
    wxString getString(int key) {
        lua_pushnumber(state, key);
        lua_gettable(state, -2);
        wxString value=lua_tostring(state, -1);
        lua_pop(state,1);
        return value;
    }
    
    wxString getString(wxString key) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_gettable(state, -2);
        wxString value=lua_tostring(state, -1);
        lua_pop(state,1);
        return value;
    }
    
    bool getBool(int key) {
        lua_pushnumber(state, key);
        lua_gettable(state, -2);
        bool value=lua_toboolean(state, -1);
        lua_pop(state,1);
        return value;
    }
    
    bool getBool(wxString key) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_gettable(state, -2);
        bool value=lua_toboolean(state, -1);
        lua_pop(state,1);
        return value;
    }
    
    void* getUserData(int key) {
        lua_pushnumber(state, key);
        lua_gettable(state, -2);
        void* value=lua_touserdata(state, -1);
        lua_pop(state,1);
        return value;
    }
    
    void* getUserData(wxString key) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_gettable(state, -2);
        void* value=lua_touserdata(state, -1);
        lua_pop(state,1);
        return value;
    }
    
    void getTable(int key, std::function<void(TableReaderWriter*)>lambda);
    void getTable(wxString key, std::function<void(TableReaderWriter*)>lambda);
    
    ExecBuilder*execBuilder(wxString key);
};


class TableWriter {
    lua_State*state;
    Lua*lua;
public:
    TableWriter(Lua*lua, lua_State*state) { this->lua=lua; this->state=state; }
    
    TableWriter&put(int key, int value) {
        lua_pushinteger(state, value);
        lua_rawseti(state, -2, key);
        return *this;
    }
    
    TableWriter&put(wxString key, int value) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_pushinteger(state, value);
        lua_settable(state, -3);
        return *this;
    }
    
    TableWriter&put(int key, double value) {
        lua_pushnumber(state, value);
        lua_rawseti(state, -2, key);
        return *this;
    }
    
    TableWriter&put(wxString key, double value) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_pushnumber(state, value);
        lua_settable(state, -3);
        return *this;
    }
    
    TableWriter&put(int key, wxString value) {
        lua_pushstring(state, value.ToUTF8().data());
        lua_rawseti(state, -2, key);
        return *this;
    }
    
    TableWriter&put(wxString key, wxString value) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_pushstring(state, value.ToUTF8().data());
        lua_settable(state, -3);
        return *this;
    }
    
    TableWriter&put(int key, bool value) {
        lua_pushboolean(state, value);
        lua_rawseti(state, -2, key);
        return *this;
    }
    
    TableWriter&put(wxString key, bool value) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_pushboolean(state, value);
        lua_settable(state, -3);
        return *this;
    }
    
    TableWriter&put(int key, void* value) {
        lua_pushlightuserdata(state, value);
        lua_rawseti(state, -2, key);
        return *this;
    }
    
    TableWriter&put(wxString key, void* value) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_pushlightuserdata(state, value);
        lua_settable(state, -3);
        return *this;
    }
    
    TableWriter&put(int key, FunctionRef functionRef, bool freeRef) {
        lua_rawgeti(state, LUA_REGISTRYINDEX, functionRef.ref);
        lua_rawseti(state, -2, key);
        if (freeRef) {
            luaL_unref(state, LUA_REGISTRYINDEX, freeRef);
        }
        return *this;
    }
    
    TableWriter&put(wxString key, FunctionRef functionRef, bool freeRef) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_rawgeti(state, LUA_REGISTRYINDEX, functionRef.ref);
        lua_settable(state, -3);
        if (freeRef) {
            luaL_unref(state, LUA_REGISTRYINDEX, freeRef);
        }
        return *this;
    }
    
    TableWriter&put(int key, TableRef tableRef, bool freeRef) {
        lua_rawgeti(state, LUA_REGISTRYINDEX, tableRef.ref);
        lua_rawseti(state, -2, key);
        if (freeRef) {
            luaL_unref(state, LUA_REGISTRYINDEX, freeRef);
        }
        return *this;
    }
    
    TableWriter&put(wxString key, TableRef tableRef, bool freeRef) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_rawgeti(state, LUA_REGISTRYINDEX, tableRef.ref);
        lua_settable(state, -3);
        if (freeRef) {
            luaL_unref(state, LUA_REGISTRYINDEX, freeRef);
        }
        return *this;
    }
    
    TableWriter&putTable(int key, std::function<void(TableWriter*)>tableWriterLambda) {
        lua_pushinteger(state, key);
        lua_newtable(state);
        TableWriter tableWriter(lua, state);
        tableWriterLambda(&tableWriter);
        lua_settable(state, -3);
        return *this;
    }
    
    TableWriter&putTable(wxString key, std::function<void(TableWriter*)>tableWriterLambda) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_newtable(state);
        TableWriter tableWriter(lua, state);
        tableWriterLambda(&tableWriter);
        lua_settable(state, -3);
        return *this;
    }
    
    TableWriter&putNil(wxString key) {
        lua_pushstring(state, key.ToUTF8().data());
        lua_pushnil(state);
        lua_settable(state, -3);
        return *this;
    }
    
    TableWriter&putNil(int key) {
        lua_pushnil(state);
        lua_rawseti(state, -2, key);
        return *this;
    }
    
    TableWriter& putNativeFunction(wxString key, NativeFunction nativeFunction);
};

class TableReaderWriter:public TableReader,public TableWriter{
public:
    TableReaderWriter(Lua*lua, lua_State*state):TableReader(lua, state),TableWriter(lua, state) { }
};

class ValuesListWriter {
    Lua*lua;
    lua_State*state;
    int count;
public:
    ValuesListWriter(Lua*lua, lua_State*state) {
        this->lua=lua;
        this->state=state;
        count=0;
    }
    ValuesListWriter*pushInt(int value) {
        lua_pushnumber(state, value);
        count++;
        return this;
    }
    ValuesListWriter*pushDouble(double value) {
        lua_pushnumber(state, value);
        count++;
        return this;
    }
    ValuesListWriter*pushBool(bool value) {
        lua_pushboolean(state, value);
        count++;
        return this;
    }
    ValuesListWriter*pushString(wxString value) {
        lua_pushstring(state, value.ToUTF8().data());
        count++;
        return this;
    }
    ValuesListWriter*pushUserData(void*value) {
        lua_pushlightuserdata(state, value);
        count++;
        return this;
    }
    ValuesListWriter*pushFunction(FunctionRef functionRef, bool freeRef) {
        lua_rawgeti(state, LUA_REGISTRYINDEX, functionRef.ref);
        count++;
        if (freeRef)
            luaL_unref(state, LUA_REGISTRYINDEX, freeRef);
        return this;
    }
    
    ValuesListWriter*pushTableRef(TableRef tableRef, bool freeRef) {
        lua_rawgeti(state, LUA_REGISTRYINDEX, tableRef.ref);
        count++;
        if (freeRef)
            luaL_unref(state, LUA_REGISTRYINDEX, freeRef);
        return this;
    }
    ValuesListWriter*pushNil() {
        lua_pushnil(state);
        count++;
        return this;
    }
    ValuesListWriter*pushTable(std::function<void(TableWriter*)>tableWriter) {
        lua_newtable(state);
        TableWriter writer(lua, state);
        tableWriter(&writer);
        count++;
        return this;
    }
    int getValuesCount() { return count; }
};

class ValuesListReader {
    Lua*lua;
    lua_State*state;
    int offset;
    int maxCount;
public:
    ValuesListReader(Lua*lua, lua_State*state, int offset, int maxCount) { this->lua=lua; this->state=state;this->offset=offset; this->maxCount=maxCount; }
    ValueType getType(int index){return getTypeFromLuaType(lua_type(state, offset + index));}
    int getInt(int index) { return (int)lua_tointeger(state, offset + index);}
    double getDouble(int index) { return lua_tonumber(state, offset + index);}
    wxString getString(int index) { return wxString(lua_tostring(state, offset + index));}
    bool getBool(int index) { return lua_toboolean(state, offset + index);}
    void*getUserData(int index){ return lua_touserdata(state, offset+index);}
    void getTable(int index, std::function<void(TableReader*reader)>tableReader) {
        lua_pushvalue(state, offset + index);
        TableReader reader(lua, state);
        tableReader(&reader);
        lua_pop(state, 1);
    }
    
    FunctionRef getFunctionRef(int index) {
        lua_pushvalue(state, offset + index);
        int ref=luaL_ref(state, LUA_REGISTRYINDEX);
        return {ref};
    }
    TableRef getTableRef(int index) {
        lua_pushvalue(state, offset + index);
        int ref=luaL_ref(state, LUA_REGISTRYINDEX);
        return {ref};
    }
};

class ExecBuilder: ValuesListWriter {
    Lua*lua;
    lua_State*state;
    bool selfRemove;
public:
    ExecBuilder(Lua*lua, lua_State*state, bool selfRemove):ValuesListWriter(lua, state) { this->lua=lua; this->state=state; this->selfRemove=selfRemove; }

    ExecBuilder&pushInt(int value) { ValuesListWriter::pushInt(value); return *this; }
    ExecBuilder&pushDouble(double value) { ValuesListWriter::pushDouble(value); return *this; }
    ExecBuilder&pushBool(bool value) { ValuesListWriter::pushBool(value); return *this; }
    ExecBuilder&pushString(wxString value) { ValuesListWriter::pushString(value); return *this; }
    ExecBuilder&pushUserData(void*value) { ValuesListWriter::pushUserData(value); return *this; }
    ExecBuilder&pushNil() { ValuesListWriter::pushNil(); return *this; }
    ExecBuilder&pushFunction(FunctionRef ref, bool freeRef) { ValuesListWriter::pushFunction(ref, freeRef); return *this; }
    ExecBuilder&pushTable(TableRef ref, bool freeRef) { ValuesListWriter::pushTableRef(ref, freeRef); return *this; }
    ExecBuilder&pushTable(std::function<void(TableWriter*)>tableWriter) { ValuesListWriter::pushTable(tableWriter); return *this; }
    
    bool exec(int expectedReturnValuesCount, std::function<void(bool status, ValuesListReader*result, wxString&errorMessage)>onComplete) {
        if (lua_pcall(state, getValuesCount(), expectedReturnValuesCount, 0) != 0) {
            wxString errorMessage=lua_tostring(state, -1);
            ValuesListReader returnValuesReader(lua, state, 0, 0);
            onComplete(false, &returnValuesReader, errorMessage);
            lua_pop(state, 1);
            return false;
        }
        ValuesListReader returnValuesReader(lua, state, -expectedReturnValuesCount, expectedReturnValuesCount);
        wxString message("");
        onComplete(true, &returnValuesReader, message);
        lua_pop(state, expectedReturnValuesCount);
        if (selfRemove) {
            delete this;
        }
        return true;
    }
};

int genericLuaNativeFunctionHandler(lua_State*state);
void* getPointerFromLuaRegistry(lua_State*state, wxString name);

class Lua {
private:
    lua_State*state;
    std::unordered_map<int, NativeFunction> nativeFunctionMap;
    std::vector<std::function<char*(char*)>>luaModulesReaders;
public:
    friend class ValuesListWriter;
    friend class ValuesListReader;
    friend class TableWriter;
    
    Lua(bool loadAllLuaStdLibs) {
        state = luaL_newstate();
        putPointerInRegistry("wrapper", this);
        if(loadAllLuaStdLibs) luaL_openlibs(state);
        lua_newtable(state);
        lua_setglobal(state, "LuaWrapperFFI");
        configureCustomModuleReader();
    }
    ~Lua() {
        if(state!=NULL)lua_close(state);
    }
    bool evalFile(wxString source, wxString fileName) {
        if(luaL_loadstring(state, source.ToUTF8().data())!= LUA_OK||lua_pcall(state, 0, LUA_MULTRET, 0)!= LUA_OK) {
            wxPrintf("Lua error in file %s. Message: %s\n", fileName, wxString(lua_tostring(state, lua_gettop(state))));
            lua_pop(state, 1);
            return false;
        }
        return true;
    }
    bool evalExpression(wxString source) {
        return evalExpression(source, [](bool state, auto result){});
    }
    bool evalExpression(wxString source, std::function<void(bool state, wxString&result)>onComplete) {
        const char*s=source.ToUTF8().data();
        if((luaL_loadstring(state, s)!=LUA_OK)||(lua_pcall(state, 0, 1, 0) != LUA_OK)) {
            wxString errorMessage=wxString::Format("Lua error. Message: %s\n", wxString(lua_tostring(state, lua_gettop(state))));
            onComplete(false, errorMessage);
            lua_pop(state, 1);
            return false;
        }
        
        int returnValuesCount=lua_gettop(state);
        if (returnValuesCount>0) {
            wxString message("");
            onComplete(true, message);
            lua_pop(state, 1);
        }
        return true;
    }
    void putPointerInRegistry(wxString name, void* pointer) {
        lua_pushstring(state, name.ToUTF8().data());
        lua_pushlightuserdata(state, pointer);
        lua_settable(state, LUA_REGISTRYINDEX);
    }
    void* getPointerFromRegistry(wxString name) {
        return getPointerFromLuaRegistry(state, name);
    }
    void editGlobalTable(std::function<void(TableReaderWriter*)>readerWriterLambda) {
        lua_pushglobaltable(state);
        TableReaderWriter readerWriter(this, state);
        readerWriterLambda(&readerWriter);
        lua_pop(state, 1);
    }
    void editTable(TableRef ref, std::function<void(TableReaderWriter*)>readerWriterLambda) {
        lua_rawgeti(state, LUA_REGISTRYINDEX, ref.ref);
        TableReaderWriter readerWriter(this, state);
        readerWriterLambda(&readerWriter);
        lua_pop(state, 1);
    }
    bool globalPresent(wxString key) {
        lua_getglobal(state, key.ToUTF8().data());
        bool isPresent = !lua_isnil(state, -1);
        lua_pop(state, 1);
        return isPresent;
    }
    int globalInt(wxString key) {
        lua_getglobal(state, key.ToUTF8().data());
        int value = (int)lua_tointeger(state, -1);
        lua_pop(state, 1);
        return value;
    }
    double globalDouble(wxString key) {
        lua_getglobal(state, key.ToUTF8().data());
        double value = lua_tonumber(state, -1);
        lua_pop(state, 1);
        return value;
    }
    bool globalBool(wxString key) {
        lua_getglobal(state, key.ToUTF8().data());
        bool value = lua_toboolean(state, -1);
        lua_pop(state, 1);
        return value;
    }
    wxString globalString(wxString key) {
        lua_getglobal(state, key.ToUTF8().data());
        wxString value = wxString(lua_tostring(state, -1));
        lua_pop(state, 1);
        return value;
    }
    
    void* globalUserData(wxString key) {
        lua_getglobal(state, key.ToUTF8().data());
        void*value=lua_touserdata(state, -1);
        lua_pop(state, 1);
        return value;
    }
    ExecBuilder globalFunctionExec(wxString functionName) {
        lua_pushglobaltable(state);
        lua_pushstring(state, functionName.ToUTF8().data());
        lua_gettable(state, -2);
        lua_remove(state, -2);
        return ExecBuilder(this, state, false);
    }
    
    ExecBuilder functionRefExec(FunctionRef functionRef) {
        lua_rawgeti(state, LUA_REGISTRYINDEX, functionRef.ref);
        return ExecBuilder(this, state, false);
    }
    
    TableRef createNewLuaTable(){
        lua_newtable(state);
        int ref=luaL_ref(state, LUA_REGISTRYINDEX);
        return {ref};
    }
    /**
     Register native function in predefined table LuaWrapperFFI
     */
    void registerNativeFunction(wxString functionName, NativeFunction nativeFunction) {
        int functionId = (int)nativeFunctionMap.size();
        nativeFunctionMap[functionId]=nativeFunction;
        lua_getglobal(state, "LuaWrapperFFI");
        lua_pushinteger(state, functionId);
        lua_pushcclosure(state, genericLuaNativeFunctionHandler, 1);
        lua_setfield(state, -2, functionName.ToUTF8().data());
        lua_pop(state, 1);
    }
    /**
        customModuleReader can be a lambda that accepts path to file/module and responds with module content
     */
    void registerLuaModuleReader(std::function<char*(char*)>customModuleReader) {
        this->luaModulesReaders.push_back(customModuleReader);
    }
    
    int getLuaModulesReadersCount() {
        return (int)this->luaModulesReaders.size();
    }
    
    std::function<char*(char*)> getLuaModuleReader(int index) {
        return this->luaModulesReaders[index];
    }
    
    NativeFunction getNativeFunction(int functionId) {
        return nativeFunctionMap[functionId];
    }
    void functionRefRemove(FunctionRef ref){
        luaL_unref(state, LUA_REGISTRYINDEX, ref.ref);
    }
    void tableRefRemove(TableRef ref){
        luaL_unref(state, LUA_REGISTRYINDEX, ref.ref);
    }
    void inheritTable(TableRef childRef, TableRef parentRef) {
        lua_rawgeti(state, LUA_REGISTRYINDEX, childRef.ref);
        if (!lua_istable(state, -1)) {
            luaL_error(state, "Child reference is not a table");
        }

        lua_rawgeti(state, LUA_REGISTRYINDEX, parentRef.ref);
        if (!lua_istable(state, -1)) {
            luaL_error(state, "Parent reference is not a table");
        }

        lua_newtable(state);
        lua_pushvalue(state, -2);
        lua_setfield(state, -2, "__index");
        lua_setmetatable(state, -3);

        lua_pop(state, 2);
    }
    void configureCustomModuleReader();
    void dbgPushInt(int val) {
        lua_pushinteger(state, val);
    }
    int dbgPopInt() {
        int v = lua_tonumber(state, -1);
        lua_pop(state, 1);
        return v;
    }
    wxString dbgGetDumpStack() {
        return dumpStack(state);
    }
    
    void dbgPrintDumpStack() {
        wxPrintf("%s\n", dumpStack(state));
    }
};

}
#endif
