//
//  tests.cpp
//
#include "lxw.hpp"

#ifdef LUA_XML_TEST

#define TEST_NO_MAIN

#include "accutestWrapper.hpp"

using namespace lxe;
Lua createLua(bool addGuard);
void closeLua(Lua&lua, bool addGuard);

void testLuaDbgPushPop() {
    Lua lua=createLua(false);
    lua.dbgPushInt(45);
    lua.dbgPushInt(67);
    TEST_EQUALS_INT(lua.dbgPopInt(), 67);
    TEST_EQUALS_INT(lua.dbgPopInt(), 45);
    closeLua(lua, false);
}

void testLuaSimpleExecWithoutGuards() {
    Lua lua=createLua(false);
    lua.evalExpression(R"(
      function myFunction()
          return 4
      end
    )");
    lua.globalFunctionExec("myFunction").exec(1, [](bool status, ValuesListReader*result, wxString errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 4);
    });
    
    closeLua(lua, false);
}

void testLuaSimpleExec() {
    Lua lua=createLua(true);
    lua.evalExpression(R"(
      function myFunction()
          return 4
      end
    )");
    lua.globalFunctionExec("myFunction").exec(1, [](bool status, ValuesListReader*result, wxString errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 4);
    });
    
    closeLua(lua, true);
}

void testLuaSimpleExecNestedField() {
    Lua lua=createLua(true);
    
    lua.evalExpression(R"(
    functions = {
      myFunction=function()
          return 4
      end
    }
    )");
    lua.editGlobalTable([](TableReaderWriter*rw){
        rw->getTable("functions", [](TableReaderWriter*rw){
            rw->execBuilder("myFunction")->exec(1, [](bool status, ValuesListReader *result, wxString&errorMessage){
                TEST_EQUALS_INT(result->getInt(0), 4);
            });
        });
    });

    closeLua(lua, true);
}

void testLuaSimpleExecPassArgs() {
    Lua lua=createLua(true);
    
    lua.evalExpression(R"(
      function passArgs(intArg, doubleArg, boolArg, strArg, ptrArg)
          return intArg,doubleArg,boolArg,strArg, ptrArg
      end
    )");
    void*mypointer=(void*)"hello";
    lua.globalFunctionExec("passArgs")
        .pushInt(45).pushDouble(54.4).pushBool(true).pushString("hello").pushUserData(mypointer)
        .exec(5, [mypointer](bool status, ValuesListReader*result, wxString&errorMessage) {
            TEST_EQUALS_INT(result->getInt(0), 45);
            TEST_EQUALS_DBL(result->getDouble(1), 54.4);
            TEST_EQUALS_BOOL(result->getBool(2), true);
            TEST_EQUALS_WXSTR(result->getString(3), "hello");
            TEST_EQUALS_PTR(result->getUserData(4), mypointer);
    });
    
    closeLua(lua, true);
}

void testLuaExecDifferentResponseValues() {
    Lua lua=createLua(true);
    lua.evalExpression(R"(
      function retInt()
          return 4
      end
      function retString()
          return "hello"
      end
      function retDouble()
          return 7.43
      end
      function retBool()
          return true
      end
    )");
    lua.globalFunctionExec("retInt").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 4);
    });
    lua.globalFunctionExec("retDouble").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_DBL(result->getDouble(0), 7.43);
    });
    lua.globalFunctionExec("retBool").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_BOOL(result->getBool(0), true);
    });
    lua.globalFunctionExec("retString").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_WXSTR(result->getString(0), "hello");
    });
    
    closeLua(lua, true);
}

void testLuaExecMultipleResults() {
    Lua lua=createLua(true);
    lua.evalExpression(R"(
      function myFunction()
          return 4,8
      end
    )");
    lua.globalFunctionExec("myFunction").exec(2, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 4);
        TEST_EQUALS_INT(result->getInt(1), 8);
    });
    closeLua(lua, true);
}

void testLuaSetField() {
    Lua lua=createLua(true);
    lua.editGlobalTable([](TableReaderWriter*rw) {
        rw->put("abc", 43);
    });
    lua.evalExpression(R"(
      function testFunction()
          return abc
      end
    )");
    lua.globalFunctionExec("testFunction").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 43);
    });
    
    closeLua(lua, true);
}

void testLuaExecPassTableIntKeys() {
    Lua lua=createLua(true);
    lua.evalExpression(R"(
      function myFunction(tbl)
          return tbl[1], tbl[2], tbl[3], tbl[4]
      end
    )");
    lua.globalFunctionExec("myFunction").pushTable([](TableWriter*table) {
        table->put(1, 4)
            .put(2, 8.2)
            .put(3, wxString("hello"))
            .put(4, true);
    }).exec(4, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 4);
        TEST_EQUALS_DBL(result->getDouble(1), 8.2);
        TEST_EQUALS_WXSTR(result->getString(2), wxString("hello"));
        TEST_EQUALS_BOOL(result->getBool(3), true);
    });

    closeLua(lua, true);
}

void testLuaExecPassTableStringKeys() {
    Lua lua=createLua(true);
    lua.evalExpression(R"(
            function myFunction(tbl)
                return tbl['int'], tbl['dbl'], tbl['str'], tbl['bool']
            end
    )");
    lua.globalFunctionExec("myFunction").pushTable([](TableWriter*table) {
        table->put("int", 4)
            .put("dbl", 8.2)
            .put("str", wxString("hello"))
            .put("bool", true);
    }).exec(4, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 4);
        TEST_EQUALS_DBL(result->getDouble(1), 8.2);
        TEST_EQUALS_WXSTR(result->getString(2), wxString("hello"));
        TEST_EQUALS_BOOL(result->getBool(3), true);
    });

    closeLua(lua, true);
}


void testLuaExecPassNestedTable() {
    Lua lua=createLua(true);
    lua.evalExpression(R"(
      function myFunctionStrKey(tbl)
          return tbl['nested']['a']
      end
      function myFunctionIntKey(tbl)
          return tbl[58]['a']
      end
    )");
    {
        lua.globalFunctionExec("myFunctionStrKey").pushTable([](TableWriter*table) {
            table->putTable("nested", [](TableWriter*nested) {
                nested->put("a", 13);
            });
        }).exec(4, [](bool status, ValuesListReader*result, wxString&errorMessage) {
            TEST_EQUALS_INT(result->getInt(0), 13);
        });
    }
    {
        lua.globalFunctionExec("myFunctionIntKey").pushTable([](TableWriter*table) {
            table->putTable(58, [](TableWriter*nested) {
                nested->put("a", 13);
            });
        }).exec(4, [](bool status, ValuesListReader*result, wxString&errorMessage) {
            TEST_EQUALS_INT(result->getInt(0), 13);
        });
    }
    
    closeLua(lua, true);
}

void testLuaExecReturnTableNumericKeys() {
    Lua lua=createLua(true);
    lua.evalExpression(R"(
      function myFunction()
          return {2, 3.3, 'Hello', true}
      end
    )");
    lua.globalFunctionExec("myFunction").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        result->getTable(0, [](TableReader*reader) {
            TEST_EQUALS_INT(reader->getInt(1), 2);
            TEST_EQUALS_DBL(reader->getDouble(2), 3.3);
            TEST_EQUALS_WXSTR(reader->getString(3), wxString("Hello"));
            TEST_EQUALS_BOOL(reader->getBool(4), true);
        });
    });
    
    closeLua(lua, true);
}

void testLuaExecReturnTableStringKeys() {
    Lua lua=createLua(true);
    lua.evalExpression(R"(
      function myFunction()
         return {keyInt=2, keyDbl=3.3, keyStr='Hello', keyBool=true}
      end
    )");
    lua.globalFunctionExec("myFunction").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        result->getTable(0, [](TableReader*reader) {
            TEST_EQUALS_INT(reader->getInt("keyInt"), 2);
            TEST_EQUALS_DBL(reader->getDouble("keyDbl"), 3.3);
            TEST_EQUALS_WXSTR(reader->getString("keyStr"), wxString("Hello"));
            TEST_EQUALS_BOOL(reader->getBool("keyBool"), true);
        });
    });
    
    closeLua(lua, true);
}

void testLuaGetGlobalField() {
    Lua lua=createLua(true);
    lua.evalExpression(R"(
      valInt=32
      valDbl=56.65
      valStr="Hello"
      valBool=true
    )");
    
    TEST_EQUALS_BOOL(lua.globalPresent("someField"), false);
    TEST_EQUALS_DBL(lua.globalPresent("valDbl"), true);
    TEST_EQUALS_INT(lua.globalInt("valInt"), 32);
    TEST_EQUALS_DBL(lua.globalDouble("valDbl"), 56.65);
    TEST_EQUALS_WXSTR(lua.globalString("valStr"), "Hello");
    TEST_EQUALS_BOOL(lua.globalBool("valBool"), true);
    
    closeLua(lua, true);
}

void testLuaGetGlobalNestField() {
    Lua lua=createLua(true);
    lua.evalExpression(R"(
      fields={
        valStr="Hello"
      }
    )");
    lua.editGlobalTable([](TableReader*rw){
        rw->getTable("fields", [](TableReaderWriter*rw){
            TEST_EQUALS_WXSTR(rw->getString("valStr"), "Hello");
        });
    });
    
    
    closeLua(lua, true);
}

void testLuaGetGlobalFieldAndExec() {
    Lua lua=createLua(true);
    lua.evalExpression(R"(
       function myFunction()
           return 56;
       end
       x=function()
           return 45
       end
    )");
    lua.globalFunctionExec("myFunction").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 56);
    });
    lua.globalFunctionExec("x").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 45);
    });
    
    closeLua(lua, true);
}

void testLuaRegisterNativeFunction() {
    Lua lua=createLua(true);
    lua.registerNativeFunction("generateString", [](ValuesListReader*args, ValuesListWriter*retValues) {
        retValues->pushString("GeneratedString");
    });
    lua.evalExpression(R"(
       function myFunction()
           return LuaWrapperFFI.generateString();
       end
    )");
    
    lua.globalFunctionExec("myFunction").exec(1, [](bool status, ValuesListReader *result, wxString&errorMessage) {
        TEST_EQUALS_WXSTR(result->getString(0), "GeneratedString");
    });
    
    closeLua(lua, true);
}

void testLuaRegisterNativeFunction2() {
    Lua lua=createLua(true);
    lua.registerNativeFunction("myMath", [](ValuesListReader*args, ValuesListWriter*retValues) {
        int arg1=args->getInt(0);
        int arg2=args->getInt(1);
        retValues->pushInt(arg1+arg2)->pushInt(arg1-arg2);
    });
    lua.registerNativeFunction("myConcatenation", [](ValuesListReader*args, ValuesListWriter*retValues) {
        wxString arg1=args->getString(0);
        wxString arg2=args->getString(1);
        retValues->pushString(arg1+arg2);
    });
    lua.evalExpression(R"(
       function myMath(x,y)
           return LuaWrapperFFI.myMath(x,y);
       end
       function myConcatenation(x,y)
           return LuaWrapperFFI.myConcatenation(x,y);
       end
    )");
    lua.globalFunctionExec("myMath").pushInt(19).pushInt(6).exec(2, [](bool status, ValuesListReader *result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 25);
        TEST_EQUALS_INT(result->getInt(1), 13);
    });
    lua.globalFunctionExec("myConcatenation").pushString("hello ").pushString("world").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_WXSTR(result->getString(0), wxString("hello world"));
    });
    closeLua(lua, true);
}

void testLuaRegisterNativeFunctionNestedTable() {
    Lua lua=createLua(true);
    lua.registerNativeFunction("nestedTableInArg", [](ValuesListReader*args, ValuesListWriter*retValues) {
        int val;
        args->getTable(0, [&val, retValues](TableReader*table) {
            val=table->getInt("myField");
        });
        retValues->pushInt(val);
    });
    lua.evalExpression(R"(
       function nestedTableInArg(x)
           return LuaWrapperFFI.nestedTableInArg(x)+1;
       end
    )");
    lua.globalFunctionExec("nestedTableInArg").pushTable([](TableWriter*writer) {
        writer->put("myField", 34);
    }).exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 35);
    });
    closeLua(lua, true);
}

void testLuaRegisterNativeFunctionNestedNestedTable() {
    Lua lua=createLua(true);
    lua.registerNativeFunction("nestedNestedTableInArg", [](ValuesListReader*args, ValuesListWriter*retValues) {
        int fieldValue;
        args->getTable(0, [&fieldValue](TableReader*reader) {
            reader->getTable("x", [&fieldValue](TableReader*reader1) {
                fieldValue = reader1->getInt("innerField");
            });
        });
        int argInt=args->getInt(1);
        retValues->pushInt(fieldValue)->pushInt(argInt);
    });
    lua.evalExpression(R"(
       function nestedNestedTableInArg()
           return LuaWrapperFFI.nestedNestedTableInArg({x={innerField=43}}, 101);
       end
    )");
    lua.globalFunctionExec("nestedNestedTableInArg").exec(2,[](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 43);
        TEST_EQUALS_INT(result->getInt(1), 101);
    });
    
    closeLua(lua, true);
}

void testLuaRegisterNativeFunctionDirectlyInField() {
    Lua lua=createLua(true);
    lua.editGlobalTable([](TableReaderWriter*rw) {
        rw->putNativeFunction("myTestFunction", [](ValuesListReader*args, ValuesListWriter*retValues) {
            retValues->pushInt(56);
        });
    });
    lua.globalFunctionExec("myTestFunction").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 56);
    });
    
    closeLua(lua, true);
}

void testLuaRegisterNativeFunctionDirectlyInNestedField() {
    Lua lua=createLua(true);
    lua.editGlobalTable([](TableReaderWriter*rw) {
        rw->putTable("functions", [](TableWriter*rw) {
            rw->putNativeFunction("nestedFunction", [](ValuesListReader*args, ValuesListWriter*retValues) {
                int v=args->getInt(0);
                retValues->pushInt(v);
            });
        });
        
    });
    lua.evalExpression(R"(
       function tst()
           return functions.nestedFunction(34);
       end
    )");
    lua.globalFunctionExec("tst").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 34);
    });
    
    closeLua(lua, true);
}

void testLuaFunctionRefExec() {
    Lua lua=createLua(true);
    lua.registerNativeFunction("acceptAndExecFunction", [&lua](ValuesListReader*args, ValuesListWriter*retValues) {
        FunctionRef function=args->getFunctionRef(0);
        lua.functionRefExec(function).pushInt(100).exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage){
            TEST_EQUALS_INT(result->getInt(0), 103);
        });
    });

    lua.evalExpression(R"(
        LuaWrapperFFI.acceptAndExecFunction(function(value)return value+3 end)
    )");
    
    closeLua(lua, true);
}

void testLuaFunctionPassing() {
    Lua lua=createLua(true);
    //native function just pass the passed function through itself and also writes it into the global variable myNativeFunction
    lua.registerNativeFunction("acceptAndReturnFunction", [&lua](ValuesListReader*args, ValuesListWriter*retValues) {
        FunctionRef function=args->getFunctionRef(0);
        lua.editGlobalTable([function](TableReaderWriter*rw){
            rw->put("myNativeFunction", function, false);
        });
        retValues->pushFunction(function, true);
    });

    lua.evalExpression(R"(
       function tst()
           f = LuaWrapperFFI.acceptAndReturnFunction(function() return 54 end)
           return f()
       end
    )");

    lua.globalFunctionExec("tst").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 54);
    });

    lua.globalFunctionExec("myNativeFunction").exec(1, [](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 54);
    });
    
    closeLua(lua, true);
}

void testLuaTableRefPassing() {
    Lua lua=createLua(true);
    TableRef ref=lua.createNewLuaTable();
    lua.editTable(ref, [](TableReaderWriter*rw){
        rw->put("x", 1);
        rw->put("y", 2);
    });
    lua.evalExpression(R"(
       function tst(x)
           return x.x, x.y
       end
    )");
    lua.globalFunctionExec("tst")
        .pushTable(ref, false)
        .exec(2, [](bool status, ValuesListReader*result, wxString&errorMessage) {
            TEST_EQUALS_INT(result->getInt(0), 1);
            TEST_EQUALS_INT(result->getInt(1), 2);
    });

    closeLua(lua, true);
}

void testLuaTableInheritance() {
    Lua lua=createLua(true);
    TableRef baseTable=lua.createNewLuaTable();
    lua.editTable(baseTable, [](TableReaderWriter*rw){
        rw->put("baseValueX", 1);
        rw->put("baseValueY", 2);
    });

    TableRef childTable=lua.createNewLuaTable();
    lua.inheritTable(childTable, baseTable);
    lua.editTable(childTable, [](TableReaderWriter*rw){
        rw->put("childValueX", 5);
        rw->put("childValueY", 6);
    });

    lua.tableRefRemove(baseTable);
    lua.evalExpression(R"(
       function tst(x)
           return x.childValueX, x.childValueY, x.baseValueX, x.baseValueY
       end
    )");
    lua.globalFunctionExec("tst")
        .pushTable(childTable, true)
        .exec(4, [&lua](bool status, ValuesListReader*result, wxString&errorMessage) {
            TEST_EQUALS_INT(result->getInt(0), 5);
            TEST_EQUALS_INT(result->getInt(1), 6);
            TEST_EQUALS_INT(result->getInt(2), 1);
            TEST_EQUALS_INT(result->getInt(3), 2);
    });
    
    closeLua(lua, true);
}

void testLuaRequiredCustomModule() {
    Lua lua=createLua(true);
    lua.registerLuaModuleReader([](char*name) {
        if(strcmp(name, "/s/file/module.lua")==0)
            return (char*)"function utilsIncrement(x)return x+1 end";
        return (char*)NULL;
    });
    lua.evalExpression(R"(
        require "/s/file/module.lua"
    )");
    lua.globalFunctionExec("utilsIncrement").pushInt(5)
        .exec(1, [&lua](bool status, ValuesListReader*result, wxString&errorMessage) {
        TEST_EQUALS_INT(result->getInt(0), 6);
    });
    closeLua(lua, true);
}

Lua createLua(bool addGuard) {
    Lua lua(true);
    if(addGuard) {
        lua.dbgPushInt(123456);
    }
    return lua;
}

void closeLua(Lua&lua, bool addGuard) {
    if (addGuard) {
        wxString dbgState=lua.dbgGetDumpStack();
        int dbgGuard=lua.dbgPopInt();
        if(dbgGuard!=123456) {
            wxPrintf("Stack is not empty\n%s\n", dbgState);
        }
        TEST_ASSERT(dbgGuard==123456);
    }
}

ACUTEST_MODULE_INITIALIZER(lua_module){
    ACUTEST_ADD_TEST_(testLuaDbgPushPop);
    ACUTEST_ADD_TEST_(testLuaSimpleExecWithoutGuards);
    ACUTEST_ADD_TEST_(testLuaSimpleExec);
    ACUTEST_ADD_TEST_(testLuaSimpleExecNestedField);
    ACUTEST_ADD_TEST_(testLuaSimpleExecPassArgs);
    ACUTEST_ADD_TEST_(testLuaExecDifferentResponseValues);
    ACUTEST_ADD_TEST_(testLuaExecMultipleResults);
    ACUTEST_ADD_TEST_(testLuaSetField);
    ACUTEST_ADD_TEST_(testLuaExecPassTableIntKeys);
    ACUTEST_ADD_TEST_(testLuaExecPassTableStringKeys);
    ACUTEST_ADD_TEST_(testLuaExecPassNestedTable);
    ACUTEST_ADD_TEST_(testLuaExecReturnTableNumericKeys);
    ACUTEST_ADD_TEST_(testLuaExecReturnTableStringKeys);
    ACUTEST_ADD_TEST_(testLuaGetGlobalField);
    ACUTEST_ADD_TEST_(testLuaGetGlobalNestField);
    ACUTEST_ADD_TEST_(testLuaGetGlobalFieldAndExec);
    ACUTEST_ADD_TEST_(testLuaRegisterNativeFunction);
    ACUTEST_ADD_TEST_(testLuaRegisterNativeFunction2);
    ACUTEST_ADD_TEST_(testLuaRegisterNativeFunctionNestedTable);
    ACUTEST_ADD_TEST_(testLuaRegisterNativeFunctionNestedNestedTable);
    ACUTEST_ADD_TEST_(testLuaRegisterNativeFunctionDirectlyInField);
    ACUTEST_ADD_TEST_(testLuaRegisterNativeFunctionDirectlyInNestedField);
    ACUTEST_ADD_TEST_(testLuaFunctionPassing);
    ACUTEST_ADD_TEST_(testLuaTableRefPassing);
    ACUTEST_ADD_TEST_(testLuaFunctionRefExec);
    ACUTEST_ADD_TEST_(testLuaTableInheritance);
    ACUTEST_ADD_TEST_(testLuaRequiredCustomModule);
}

#endif
