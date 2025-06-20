//
//  lxeLua.cpp
//
//define this variable to make minilua to attach own implementation here
#define LUA_IMPL

#include "lxe.hpp"

namespace lxe {
int customModulesLoader(lua_State* state);

wxString dumpStack(lua_State * state) {
    wxString result("Stack:\n");
    int top=lua_gettop(state);
    for (int i=1; i <= top; i++) {
        result+= wxString::Format("%d\t%s\t", i, luaL_typename(state,i));
        int type=lua_type(state, i);
        switch (type) {
            case LUA_TNUMBER:
                result+= wxString::Format("%f\n",lua_tonumber(state,i));
                break;
            case LUA_TSTRING:
                result+= wxString::Format("%s\n",lua_tostring(state,i));
                break;
            case LUA_TBOOLEAN:
                result+= wxString::Format("%s\n", (lua_toboolean(state, i) ? "true" : "false"));
                break;
            case LUA_TUSERDATA:
                result+= wxString::Format("%p\n", (lua_touserdata(state,i)));
                break;
            case LUA_TNIL:
                result+= wxString::Format("nil\n");
                break;
            case LUA_TTABLE:
                result+= wxString::Format("table\n");
                break;
            case LUA_TFUNCTION:
                result+= wxString::Format("function\n");
                break;
            default:
                result+= wxString::Format("Unknown type\n");
                break;
        }
    }
    return result;
}

ValueType getTypeFromLuaType(int type) {
    switch(type){
        case LUA_TNIL:return LTYPE_NIL;
        case LUA_TNUMBER:return LTYPE_DOUBLE;
        case LUA_TBOOLEAN:return LTYPE_BOOL;
        case LUA_TSTRING:return LTYPE_STRING;
        case LUA_TFUNCTION:return LTYPE_FUNCTION;
        case LUA_TUSERDATA:return LTYPE_USERDATA;
        default: return LTYPE_OTHER;
    }
}

ValueType getLuaTypeOnTop(lua_State*state) {
    int type=lua_type(state, -1);
    return getTypeFromLuaType(type);
}

void TableReader::getTable(int key, std::function<void(TableReaderWriter*)>lambda) {
    lua_pushnumber(state, key);
    lua_gettable(state, -2);
    TableReaderWriter nestedTableReaderWriter(lua, state);
    lambda(&nestedTableReaderWriter);
}

void TableReader::getTable(wxString key, std::function<void(TableReaderWriter*)>lambda) {
    lua_pushstring(state, key.ToUTF8().data());
    lua_gettable(state, -2);
    TableReaderWriter nestedTableReaderWriter(lua, state);
    lambda(&nestedTableReaderWriter);
    lua_pop(state, 1);
}

ExecBuilder*TableReader::execBuilder(wxString key) {
    lua_pushstring(state, key.ToUTF8().data());
    lua_gettable(state, -2);
    return new ExecBuilder(lua, state, true);
}

int genericLuaNativeFunctionHandler(lua_State* state) {
    int functionId = (int)lua_tointeger(state, lua_upvalueindex(1));
    Lua*lua=(Lua*)getPointerFromLuaRegistry(state, "wrapper");
    NativeFunction nativeFunction = lua->getNativeFunction(functionId);
    int argsCount=lua_gettop(state);
    
    ValuesListReader argsReader(lua, state, 1, argsCount);
    ValuesListWriter returnWriter(lua, state);
    
    try {
        nativeFunction(&argsReader, &returnWriter);
        return returnWriter.getValuesCount();
    } catch (NativeError&error) {
        lua_pushstring(state, error.errorMessage.ToUTF8().data());
        return lua_error(state);
    }
}

void* getPointerFromLuaRegistry(lua_State*state, wxString name) {
    lua_pushstring(state, name.ToUTF8().data());
    lua_gettable(state, LUA_REGISTRYINDEX);
    if (!lua_islightuserdata(state, -1)) {
        luaL_error(state, "Expected light userdata");
        return nullptr;
    }
    void*pointer = lua_touserdata(state, -1);
    lua_pop(state, 1);
    return pointer;
}

TableWriter& TableWriter::putNativeFunction(wxString key, NativeFunction nativeFunction) {
    lua_pushstring(state, key.ToUTF8().data());
    
    int functionId = (int)lua->nativeFunctionMap.size();
    lua->nativeFunctionMap[functionId]=nativeFunction;
    lua_pushinteger(state, functionId);
    lua_pushcclosure(state, genericLuaNativeFunctionHandler, 1);
    
    lua_settable(state, -3);
    return *this;
}

int customModulesLoader(lua_State* state) {
    const char* moduleName = luaL_checkstring(state, 1);
    Lua*lua=(Lua*)getPointerFromLuaRegistry(state, "wrapper");
    for(int i=0;i<lua->getLuaModulesReadersCount();i++){
        char*content=lua->getLuaModuleReader(i)((char*)moduleName);
        if(content!=NULL){
            luaL_loadbuffer(state, content, strlen(content), moduleName);
            return 1;
        }
    }

    return 0;
}

void Lua::configureCustomModuleReader(){
    lua_getglobal(state, "package");
     lua_getfield(state, -1, "searchers"); // For Lua 5.3+
    lua_pushcfunction(state, customModulesLoader);
    int countOfSearchers=(int)lua_rawlen(state, -2);
    lua_rawseti(state, -2,  countOfSearchers + 1); // Clean up the stack
    lua_pop(state, 2);
}
}
