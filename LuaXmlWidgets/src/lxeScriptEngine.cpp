//
//  lxeScriptEngine.cpp
//

#include "lxe.hpp"
/*
using namespace lxe;

const char*LUA_LXML_GUI_REGISTRY_INDEX="REG_LXML_GUI";
void registerNativeFunctions(lua_State*state);

ScriptEngine::ScriptEngine(class Engine*engine) {
    this->engine = engine;
    state = luaL_newstate();
    luaL_openlibs(state);
    putPointerToRegistryIndex(LUA_LXML_GUI_REGISTRY_INDEX, this);
    registerNativeFunctions(state);
    prepareSystemTables();
}

ScriptEngine::~ScriptEngine() {
    lua_close(state);
}

void ScriptEngine::loadFile(wxString source, wxString fileName) {
    if (luaL_loadstring(state, source.ToUTF8().data())!= LUA_OK||lua_pcall(state, 0, LUA_MULTRET, 0)!= LUA_OK) {
        wxPrintf("Lua error in file %s. Message: %s\n",fileName, wxString(lua_tostring(state, lua_gettop(state))));
        lua_pop(state, 1);
    }
}

void ScriptEngine::executeExpression(wxString source) {
    const char*s=source.ToUTF8().data();
    if(luaL_loadstring(state, s)==LUA_OK) {
        if (lua_pcall(state, 0, 1, 0) != LUA_OK) {
            wxPrintf("Lua error. Message: %s\n", wxString(lua_tostring(state, lua_gettop(state))));
            lua_pop(state, 1);
        }
    } else {
        wxPrintf("Lua error. Message: %s\n", wxString(lua_tostring(state, lua_gettop(state))));
        lua_pop(state, 1);
    }
}

void ScriptEngine::runFunctionFromAttribute(const TagAttribute&tagAttribute, int argsCount, int resultsCount, std::function<void(ScriptEngine*)>argProvider,std::function<void(ScriptEngine*)>resultReceiver) {
    if (tagAttribute.getType() == TA_FUNCTION) {
        putReferenceOnStackTop(tagAttribute.getInt());
    } else if(tagAttribute.getType() == TA_STRING) {
        lua_getglobal(state, tagAttribute.getString().ToUTF8().data());
    }
    runFunctionFromStackTop(argsCount, resultsCount, argProvider, resultReceiver);
}

void ScriptEngine::runFunction(wxString functionName, int argsCount, int resultsCount, std::function<void(ScriptEngine*)>argProvider,std::function<void(ScriptEngine*)>resultReceiver) {
    lua_getglobal(state, functionName.ToUTF8().data());
    runFunctionFromStackTop(argsCount, resultsCount, argProvider, resultReceiver);
}

void ScriptEngine::runFunctionFromStackTop(int argsCount, int resultsCount, std::function<void(ScriptEngine*)>argProvider,std::function<void(ScriptEngine*)>resultReceiver) {
    argProvider(this);
    if (lua_pcall(state, argsCount, resultsCount, 0) != 0) {
        printf("Error running function 'main': %s\n", lua_tostring(state, -1));
        return;
    }
    currentFunctionResultsCount = resultsCount;
    resultReceiver(this);
}

void ScriptEngine::putStringOnStack(wxString&string) {
    lua_pushstring(state, string.ToUTF8().data());
}

void ScriptEngine::putLongOnStack(long long value) {
    lua_pushinteger(state, value);
}

void ScriptEngine::putDoubleOnStack(double value) {
    lua_pushnumber(state, value);
}

void ScriptEngine::putBoolOnStack(bool value) {
    lua_pushboolean(state, value);
}

void ScriptEngine::putNilOnStack() {
    lua_pushnil(state);
}

long long ScriptEngine::getLongResultFromStack(int index) {
    return lua_tointeger(state, -currentFunctionResultsCount+index);
}

double ScriptEngine::getDoubleResultFromStack(int index) {
    return lua_tonumber(state, -currentFunctionResultsCount+index);
}

wxString ScriptEngine::getStringResultFromStack(int index) {
    return wxString::FromUTF8(lua_tostring(state, -currentFunctionResultsCount+index));
}

bool ScriptEngine::getBoolResultFromStack(int index) {
    return lua_toboolean(state, -currentFunctionResultsCount+index);
}

void ScriptEngine::putPointerToRegistryIndex(const char*key, void*pointer) {
    lua_pushstring(state, key);
    lua_pushlightuserdata(state, pointer);
    lua_settable(state, LUA_REGISTRYINDEX);
}

void* ScriptEngine::getPointerFromRegistryIndex(const char* key) {
    lua_pushstring(state, key);
    lua_gettable(state, LUA_REGISTRYINDEX);
    if(lua_islightuserdata(state, -1)) {
        void*ptr=lua_touserdata(state, -1);
        lua_pop(state, 1);
        return ptr;
    } else {
        lua_pop(state, 1);
        return NULL;
    }
}

void ScriptEngine::removeReference(int reference) {
    luaL_unref(state, LUA_REGISTRYINDEX, reference);
}

int ScriptEngine::createReferenceFromStackTop() {
    return luaL_ref(state, LUA_REGISTRYINDEX);
}

void ScriptEngine::putReferenceOnStackTop(int reference) {
    lua_rawgeti(state, LUA_REGISTRYINDEX, reference);
}

static void dumpStack (lua_State *state) {
    int top=lua_gettop(state);
    for (int i=1; i <= top; i++) {
        printf("%d\t%s\t", i, luaL_typename(state,i));
        switch (lua_type(state, i)) {
            case LUA_TNUMBER:
                printf("%g\n",lua_tonumber(state,i));
                break;
            case LUA_TSTRING:
                printf("%s\n",lua_tostring(state,i));
                break;
            case LUA_TBOOLEAN:
                printf("%s\n", (lua_toboolean(state, i) ? "true" : "false"));
                break;
            case LUA_TNIL:
                printf("%s\n", "nil");
                break;
            default:
                printf("%p\n",lua_topointer(state,i));
                break;
        }
    }
    printf("\n");
}

void registerCFunctionInTopTable(lua_State*state, const char*functionName, lua_CFunction fn) {
    lua_pushcfunction(state, fn);
    lua_setfield(state, -2, functionName);
}

void pushDomElementObjectToLuaStack(lua_State*state, DomElement*domElement) {
    if(domElement==NULL) {
        lua_pushnil(state);
        return;
    }
    
    lua_getglobal(state, "DomElement_new");
    lua_pushnumber(state, domElement->getHandle());
    if (lua_pcall(state, 1, 1, 0) != 0) {
        printf("Error running function 'pushDomElementObjectToLuaStack': %s\n", lua_tostring(state, -1));
        return;
    }
}

ScriptEngine*ffi_GetScriptEngine(lua_State*state) {
    lua_pushstring(state, LUA_LXML_GUI_REGISTRY_INDEX);
    lua_gettable(state, LUA_REGISTRYINDEX);
    void*ptr=lua_touserdata(state, -1);
    lua_pop(state, 1);
    return (ScriptEngine*)ptr;
}

int ffi_Document_getElementById(lua_State*state) {
    wxString id(lua_tostring(state, -1));
    ScriptEngine*scriptEngine=ffi_GetScriptEngine(state);
    
    DomElement*domElement=scriptEngine->getEngine()->getDomElementById(id);
    pushDomElementObjectToLuaStack(state, domElement);
    return 1;
}

DomElement*getSelfDomElement(lua_State*state) {
    lua_getfield(state, 1, "handle");
    long long handle=lua_tonumber(state, -1);
    lua_pop(state, 1);
    ScriptEngine*scriptEngine=ffi_GetScriptEngine(state);
    DomElement*domElement=scriptEngine->getEngine()->getDomElementByHandle(handle);
    if(domElement==NULL) {
        luaL_error(state,"DomElement with handle=%d does not exist", handle);
        return NULL;
    }
    return domElement;
}

int pushAttributeToStack(TagAttribute*attribute, lua_State*state) {
    TagAttributeType type=attribute->getType();
    switch(type) {
        case TA_NULL:{
            return 0;
        }
        case TA_INT:{
            int val=attribute->getInt();
            lua_pushnumber(state, val);
            return 1;
        }
        case TA_DOUBLE:{
            double val=attribute->getDouble();
            lua_pushnumber(state, val);
            return 1;
        }
        case TA_STRING:{
            wxString val=attribute->getString();
            lua_pushstring(state, val.ToUTF8().data());
            return 1;
        }
        case TA_BOOL:{
            bool val=attribute->getBool();
            lua_pushboolean(state, val);
            return 1;
        }
        case TA_FUNCTION:{
            int ref=attribute->getInt();
            lua_rawgeti(state, LUA_REGISTRYINDEX, ref);
            return 1;
        }
    }
}

void extractTagAttributeValueFromLuaStack(lua_State*state, int index, TagAttribute&attribute) {
    switch (lua_type(state, index)) {
        case LUA_TNUMBER: {
            attribute.setDouble(lua_tonumber(state, index));
            break;
        }
        case LUA_TSTRING: {
            wxString strValue=lua_tostring(state, index);
            attribute.setString(strValue);
            break;
        }
        case LUA_TBOOLEAN: {
            attribute.setBool(lua_toboolean(state, index));
            break;
        }
        case LUA_TFUNCTION: {
            lua_pushvalue (state, index);//copy function from argument to top of stack
            int ref=luaL_ref(state, LUA_REGISTRYINDEX);
            attribute.setFunction(ref);
            break;
        }
        case LUA_TNIL: {
            attribute.setNull();
            break;
        }
        default:
            break;
    }
}

int ffi_DomElementPrototype_setAttribute(lua_State*state) {
    DomElement*domElement = getSelfDomElement(state);
    wxString attributeName = wxString(lua_tostring(state, 2));
    try {
        TagAttribute attribute;
        extractTagAttributeValueFromLuaStack(state, 3, attribute);
        if(attribute.getType()==TA_FUNCTION) {
            if(domElement->hasSettedAttribute(attributeName)) {
                if(domElement->getAttributeType(attributeName)==TA_FUNCTION) {
                    int ref=domElement->getAttribute(attributeName, 0);
                    luaL_unref(state, LUA_REGISTRYINDEX, ref); //remove old ref
                }
            }
        }
        domElement->setAttribute(attributeName, attribute);
    } catch(RuntimeException&ex) {
        luaL_error(state,"Cannot set attribute '%s'. Error message: %s", attributeName.ToUTF8().data(), ex.getErrorMessage().ToUTF8().data());
    }
    return 0;
}

int ffi_DomElementPrototype_getAttribute(lua_State*state) {
    DomElement*domElement=getSelfDomElement(state);
    wxString attributeName=wxString(lua_tostring(state, 2));
    TagAttribute attribute;
    if(domElement->getDynamicAttributeValue(attributeName, attribute)) {
        return pushAttributeToStack(&attribute, state);
    }
    
    if(!domElement->hasSettedAttribute(attributeName)) {
        return 0;//nil
    }
    
    attribute=domElement->getComputedAttribute(attributeName);
    return pushAttributeToStack(&attribute, state);
}

int ffi_DomElementPrototype_hasAttribute(lua_State*state) {
    DomElement*domElement = getSelfDomElement(state);
    wxString attributeName = wxString(lua_tostring(state, 2));
    lua_pushboolean(state, domElement->hasSettedAttribute(attributeName));
    return 1;
}

void readLuaTableArgumentAsAttributeHashMap(lua_State*state, int argIndex, AttributesMap*attributes) {
    lua_pushnil(state);
    while (lua_next(state, argIndex) != 0) {
        if (lua_istable(state, -1)) {
            // Handle nested table
            luaL_error(state, "arguments table cannot have nested tables");
        } else {
            // Extract key and value
            const char* key = lua_tostring(state, -2);
            TagAttribute attribute;
            extractTagAttributeValueFromLuaStack(state, -1, attribute);
            (*attributes)[wxString::FromAscii(key)]=attribute;
        }
        lua_pop(state, 1);
    }
}

std::vector<wxString>readLuaTableArgumentAsStringArray(lua_State*state, int argIndex) {
    std::vector<wxString> result;
    // Ensure the argument is a table
    luaL_checktype(state, argIndex, LUA_TTABLE);

    // Get the number of elements in the table
    int n = (int)lua_rawlen(state, argIndex);

    for (int i = 1; i <= n; ++i) {
        lua_rawgeti(state, argIndex, i); // Get the i-th element
        if (lua_type(state, -1) == LUA_TSTRING) {
            const char* str = lua_tostring(state, -1);
            result.push_back(str); // Add the string to the vector
        } else {
            // Handle non-string elements (raise an error)
            luaL_error(state, "Expected indexed array with string elements");
        }

        lua_pop(state, 1); // Pop the element from the stack
    }

    return result;
}

int ffi_DomElementPrototype_createElement(lua_State*state) {
    DomElement*parentDomElement = getSelfDomElement(state);
    wxString tagName = wxString(lua_tostring(state, 2));
    try {
        if (!lua_istable(state, 3)) {
            luaL_error(state, "createElement function expects tag name and table with attributes. For example createElement('Label', {text=\"Hello world\"})");
        }
        AttributesMap attributes;
        readLuaTableArgumentAsAttributeHashMap(state, 3, &attributes);
        
        Tag tag(0);
        tag.initAsTag(tagName, attributes);
        DomElement*newDomElement = parentDomElement->getEngine()->recursivelyInitElement(&tag, parentDomElement);
        pushDomElementObjectToLuaStack(state, newDomElement);
        return 1;
    } catch (RuntimeException&ex) {
        luaL_error(state, "createElement function for tag '%s' threw an exception: %s", tagName.ToUTF8().data(), ex.getErrorMessage().ToUTF8().data());
        return 0;
    }
}

int ffi_DomElementPrototype_remove(lua_State*state) {
    DomElement*domElement = getSelfDomElement(state);
    domElement->getEngine()->removeDomElement(domElement);
    delete domElement;
    return 0;
}

int ffi_Global_MesssageBox(lua_State*state) {
    wxString text = wxString(lua_tostring(state, 1));
    wxString caption;
    if (!lua_isnone(state, 2)) {
        caption = wxString(lua_tostring(state, 2));
    } else {
        caption = "Message";
    }
    
    std::vector<wxString> buttonsLabels;
    std::vector<int>buttonsIds;
    int buttonStyle=0;
    if (!lua_isnone(state, 3)) {
        
        buttonsLabels = readLuaTableArgumentAsStringArray(state, 3);
        if (buttonsLabels.size() == 0) {
            luaL_error(state, "MessageBox function expects non empty button labels array as optional 3 parameter");
            return 0;
        }
        if (buttonsLabels.size() > 3) {
            luaL_error(state, "MessageBox function expects button labels array as optional 3 parameter, it cannot accept more than 3 buttons, but you have provided %d", buttonsLabels.size());
            return 0;
        }
    } else {
        buttonsLabels.push_back(wxString("OK"));
        
    }
    
    switch (buttonsLabels.size()) {
        case 1: buttonStyle = wxOK;buttonsIds={wxID_OK};break;
        case 2: buttonStyle = wxYES_NO;buttonsIds={wxID_YES, wxID_NO};break;
        case 3: buttonStyle = wxYES_NO|wxCANCEL;buttonsIds={wxID_YES, wxID_NO, wxID_CANCEL};break;
    }
    
    wxString iconString = "none";
    if (!lua_isnone(state, 4)) {
        iconString = wxString(lua_tostring(state, 4));
    }

    int iconStyle = selector(iconString, {"none", "error", "warning", "question", "information"}, {wxICON_NONE, wxICON_ERROR, wxICON_WARNING, wxICON_QUESTION, wxICON_INFORMATION});
    if (iconStyle==-1) {
        luaL_error(state, "Unknown icon parameter in MessageBox '%s', accepted only none|error|warning|question|information", iconString.ToUTF8().data());
    }
    buttonStyle|=iconStyle;
    
    wxMessageDialog dlg(NULL, text, caption, buttonStyle);
    switch (buttonsLabels.size()) {
        case 1: dlg.SetOKLabel(buttonsLabels[0]);break;
        case 2: dlg.SetYesNoLabels(buttonsLabels[0], buttonsLabels[1]);break;
        case 3: dlg.SetYesNoCancelLabels(buttonsLabels[0], buttonsLabels[1], buttonsLabels[2]);break;
    }
    int resultId=dlg.ShowModal();
    for (int i=0;i<buttonsIds.size();i++) {
        if(buttonsIds[i]==resultId) {
            lua_pushinteger(state, i);
            return 1;
        }
    }
    lua_pushinteger(state, -1);
    return 1;
}

void registerNativeFunctions(lua_State*state) {
    lua_newtable(state);
    registerCFunctionInTopTable(state, "ffi_DomElementPrototype_setAttribute", ffi_DomElementPrototype_setAttribute);
    registerCFunctionInTopTable(state, "ffi_DomElementPrototype_getAttribute", ffi_DomElementPrototype_getAttribute);
    registerCFunctionInTopTable(state, "ffi_DomElementPrototype_hasAttribute", ffi_DomElementPrototype_hasAttribute);
    registerCFunctionInTopTable(state, "ffi_DomElementPrototype_createElement", ffi_DomElementPrototype_createElement);
    registerCFunctionInTopTable(state, "ffi_DomElementPrototype_remove", ffi_DomElementPrototype_remove);
    registerCFunctionInTopTable(state, "ffi_Document_getElementById", ffi_Document_getElementById);
    registerCFunctionInTopTable(state, "ffi_Global_MesssageBox", ffi_Global_MesssageBox);
    lua_setglobal(state, "FfiFunctions");
}

extern const char*wrapped;
extern const char*wrapped_size;

void ScriptEngine::prepareSystemTables() {
    loadFile("", "systemTables.lua");
}
*/
