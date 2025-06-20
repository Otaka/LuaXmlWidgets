#include "lxw.hpp"

#ifdef LUA_XML_TEST

#include "accutestWrapper.hpp"

IMPORT_ACUTEST_MODULE(lua_module);
IMPORT_ACUTEST_MODULE(lxe_module);
IMPORT_ACUTEST_MODULE(layout_engine_module);

ACUTEST_MODULES(ACUTEST_MODULE(lua_module),
                ACUTEST_MODULE(lxe_module),
                ACUTEST_MODULE(layout_engine_module)
)

TEST_LIST = {0};
#endif
