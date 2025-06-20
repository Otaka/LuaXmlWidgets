//
//  accutestWrapper.h
//

#ifndef accutestWrapper_h
#define accutestWrapper_h

#define ACUTEST_DISABLE_COLOR //I have added this to acutest.h to allow disabling terminal colorizing
#include "acutest.hpp"

#define TEST(functionName) { #functionName, functionName }

#define TEST_NOT_NULL(val1){TEST_ASSERT_((val1)!=NULL, "value is null");}
#define TEST_EQUALS_INT(val1, val2){int III1=(val1); int III2=(val2); TEST_ASSERT_((III1)==(III2), "values are not equal v1=%d v2=%d", (III1), (III2));}
#define TEST_EQUALS(val1, val2){ TEST_ASSERT_((val1)==(val2), "values are not equal");}
#define TEST_NOT_EQUALS_INT(val1, val2){int III1=(val1); int III2=(val2); TEST_ASSERT_((III1)!=(III2), "values are equal v1=%d v2=%d, but should not", (III1), (III2));}
#define TEST_SMALLER_INT(val1, val2){int III1=(val1); int III2=(val2); TEST_ASSERT_((III1)<(III2), "Value (%d) should be less than value(%d)", (III1), (III2));}
#define TEST_SMALLER_FLOAT(val1, val2){float III1=(val1); float III2=(val2); TEST_ASSERT_((III1)<(III2), "Value (%f) should be less than value(%d)", (III1), (III2));}
#define TEST_BIGGER_INT(val1, val2){int III1=(val1); int III2=(val2); TEST_ASSERT_((III1)>(III2), "Value (%d) should be less than value(%d)", (III1), (III2));}
#define TEST_BIGGER_FLOAT(val1, val2){float III1=(val1); float III2=(val2); TEST_ASSERT_((III1)>(III2), "Value (%f) should be less than value(%d)", (III1), (III2));}
#define TEST_EQUALS_PTR(val1, val2){void* III1=(val1); void* III2=(val2); TEST_ASSERT_((III1)==(III2), "pointers are not equals v1=%p v2=%p", (III1), (III2));}
#define TEST_EQUALS_DBL(val1, val2){double III1=(val1); double III2=(val2); TEST_ASSERT_((III1)==(III2), "values are not equals v1=%f v2=%f", (III1), (III2));}
#define TEST_EQUALS_BOOL(val1, val2){bool III1=(val1); bool III2=(val2); TEST_ASSERT_((III1)==(III2), "values are not equals v1=%s v2=%s", (III1? "true" : "false"), (III2? "true" : "false"));}
#define TEST_EQUALS_STR(val1, val2){std::string III1=(val1); std::string III2=(val2); TEST_ASSERT_((III1)==(III2), "values are not equals:\nv1=%s\nv2=%s\n", (III1.c_str()), (III2.c_str()));}

#define TEST_EQUALS_WXSTR(val1, val2){wxString III1=(val1); wxString III2=(val2); TEST_ASSERT_((III1)==(III2), "values are not equals:\nv1=%s\nv2=%s\n", (III1.ToUTF8().data()), (III2.ToUTF8().data()));}

#endif /* accutestWrapper_h */
