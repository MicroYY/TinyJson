#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tinyjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format) \
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")
#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")
#define EXPECT_EQ_STRING(expect, actual, alength) \
    EXPECT_EQ_BASE(sizeof(expect) - 1 == alength && memcmp(expect, actual, alength) == 0, expect, actual, "%s")
#define EXPECT_TRUE(actual) EXPECT_EQ_BASE((actual), "true", "false", "%s")
#define EXPECT_FALSE(actual) EXPECT_EQ_BASE((!actual), "false", "true", "%s")

#if defined(_MSC_VER)
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%Iu")
#else
#define EXPECT_EQ_SIZE_T(expect, actual) EXPECT_EQ_BASE((expect) == (actual), (size_t)expect, (size_t)actual, "%zu")
#endif

static void test_parse_null() {
    jsonValue v;
    v.type = TYPE_FALSE;
    EXPECT_EQ_INT(PARSE_OK, ParseJsonString(&v, "null"));
    EXPECT_EQ_INT(TYPE_NULL, GetValueType(&v));
}

static void test_parse_true() {
    jsonValue v;
    v.type = TYPE_FALSE;
    EXPECT_EQ_INT(PARSE_OK, ParseJsonString(&v, "true"));
    EXPECT_EQ_INT(TYPE_TRUE, GetValueType(&v));
}

static void test_parse_false() {
    jsonValue v;
    v.type = TYPE_TRUE;
    EXPECT_EQ_INT(PARSE_OK, ParseJsonString(&v, "false"));
    EXPECT_EQ_INT(TYPE_FALSE, GetValueType(&v));
}

#define TEST_NUMBER(expect, json)\
    do {\
        jsonValue v;\
        EXPECT_EQ_INT(PARSE_OK, ParseJsonString(&v, json));\
        EXPECT_EQ_INT(TYPE_NUMBER, GetValueType(&v));\
        EXPECT_EQ_DOUBLE(expect, GetValueNumber(&v));\
    } while(0)

#define TEST_ERROR(error, json)\
    do {\
        jsonValue v;\
        v.type = TYPE_FALSE;\
        EXPECT_EQ_INT(error, ParseJsonString(&v, json));\
        EXPECT_EQ_INT(TYPE_NULL, GetValueType(&v));\
    } while(0)

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */

    TEST_NUMBER(0E+10, "0E+10");
    TEST_NUMBER(0E-10, "0E-10");
    TEST_NUMBER(-0E+10, "-0E+10");
    TEST_NUMBER(-0E-10, "-0E-10");
    TEST_NUMBER(1.0000000000000002, "1.0000000000000002"); /* the smallest number > 1 */
    TEST_NUMBER(4.9406564584124654e-324, "4.9406564584124654e-324"); /* minimum denormal */
    TEST_NUMBER(-4.9406564584124654e-324, "-4.9406564584124654e-324");
    TEST_NUMBER(2.2250738585072009e-308, "2.2250738585072009e-308");  /* Max subnormal double */
    TEST_NUMBER(-2.2250738585072009e-308, "-2.2250738585072009e-308");
    TEST_NUMBER(2.2250738585072014e-308, "2.2250738585072014e-308");  /* Min normal positive double */
    TEST_NUMBER(-2.2250738585072014e-308, "-2.2250738585072014e-308");
    TEST_NUMBER(1.7976931348623157e+308, "1.7976931348623157e+308");  /* Max double */
    TEST_NUMBER(-1.7976931348623157e+308, "-1.7976931348623157e+308");
}

static void test_parse_expect_value() {
    TEST_ERROR(PARSE_ERR_EXPECT_VALUE, "");
    TEST_ERROR(PARSE_ERR_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(PARSE_ERR_INVALID_VALUE, "nul");
    TEST_ERROR(PARSE_ERR_INVALID_VALUE, "?");

    /* invalid number */
    TEST_ERROR(PARSE_ERR_INVALID_VALUE, "+0");
    TEST_ERROR(PARSE_ERR_INVALID_VALUE, "+1");
    TEST_ERROR(PARSE_ERR_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(PARSE_ERR_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(PARSE_ERR_INVALID_VALUE, "INF");
    TEST_ERROR(PARSE_ERR_INVALID_VALUE, "inf");
    TEST_ERROR(PARSE_ERR_INVALID_VALUE, "NAN");
    TEST_ERROR(PARSE_ERR_INVALID_VALUE, "nan");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(PARSE_ERR_ROOT_NOT_SINGULAR, "null x");

    /* invalid number */
    TEST_ERROR(PARSE_ERR_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' , 'E' , 'e' or nothing */
    TEST_ERROR(PARSE_ERR_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(PARSE_ERR_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
    TEST_ERROR(PARSE_ERR_NUMBER_OVERFLOW, "1e309");
    TEST_ERROR(PARSE_ERR_NUMBER_OVERFLOW, "-1e309");
}

static void test_parse_missing_quotation_mark() {
    TEST_ERROR(PARSE_ERR_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(PARSE_ERR_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(PARSE_ERR_INVALID_ESCAPE_CHAR, "\"\\v\"");
    TEST_ERROR(PARSE_ERR_INVALID_ESCAPE_CHAR, "\"\\'\"");
    TEST_ERROR(PARSE_ERR_INVALID_ESCAPE_CHAR, "\"\\0\"");
    TEST_ERROR(PARSE_ERR_INVALID_ESCAPE_CHAR, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(PARSE_ERR_CONTROL_CHAR, "\"\x01\"");
    TEST_ERROR(PARSE_ERR_CONTROL_CHAR, "\"\x1F\"");
}

static void test_access_null() {
    jsonValue v;
    InitValue(&v);
    SetValueString(&v, "a", 1);
    FreeValue(&v);
    EXPECT_EQ_INT(TYPE_NULL, GetValueType(&v));
    FreeValue(&v);
}


#define TEST_STRING(expect, json)\
    do {\
        jsonValue v;\
        InitValue(&v);\
        EXPECT_EQ_INT(PARSE_OK, ParseJsonString(&v, json));\
        EXPECT_EQ_INT(TYPE_STRING, GetValueType(&v));\
        EXPECT_EQ_STRING(expect, GetValueString(&v), GetValueStringLength(&v));\
        FreeValue(&v);\
    } while(0)

static void test_parse_string() {
    TEST_STRING("", "\"\"");
    TEST_STRING("Hello", "\"Hello\"");
    TEST_STRING("Hello\nWorld", "\"Hello\\nWorld\"");
    TEST_STRING("/", "\"\\/\"");
    TEST_STRING("\\", "\"\\\\\"");
    TEST_STRING("//", "\"\\//\"");
    TEST_STRING("\" \\ / \b \f \n \r \t", "\"\\\" \\\\ \\/ \\b \\f \\n \\r \\t\"");
}

static void test_access_boolean() {
    jsonValue v;
    InitValue(&v);
    SetValueString(&v, "a", 1);
    SetValueBoolean(&v, true);
    EXPECT_TRUE(GetValueBoolean(&v));
    SetValueBoolean(&v, false);
    EXPECT_FALSE(GetValueBoolean(&v));
    FreeValue(&v);
}

static void test_access_number() {
    jsonValue v;
    InitValue(&v);
    SetValueString(&v, "a", 1);
    SetValueNumber(&v, 1234.5);
    EXPECT_EQ_DOUBLE(1234.5, GetValueNumber(&v));
    FreeValue(&v);
}

static void test_access_string() {
    jsonValue v;
    InitValue(&v);
    SetValueString(&v, "", 0);
    EXPECT_EQ_STRING("", GetValueString(&v), GetValueStringLength(&v));
    SetValueString(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", GetValueString(&v), GetValueStringLength(&v));
    FreeValue(&v);
}

//static void test_parse_invalid_unicode_hex() {
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
//}
//
//static void test_parse_invalid_unicode_surrogate() {
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
//    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
//}

static void test_parse_miss_comma_or_square_bracket() {
    TEST_ERROR(PARSE_ERR_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
    TEST_ERROR(PARSE_ERR_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_ERROR(PARSE_ERR_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
    TEST_ERROR(PARSE_ERR_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse_array() {
    jsonValue v;

    InitValue(&v);
    EXPECT_EQ_INT(PARSE_OK, ParseJsonString(&v, "[ ]"));
    EXPECT_EQ_INT(TYPE_ARRAY, GetValueType(&v));
    EXPECT_EQ_SIZE_T(0, GetValueArraySize(&v));
    FreeValue(&v);

    InitValue(&v);
    EXPECT_EQ_INT(PARSE_OK, ParseJsonString(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(TYPE_ARRAY, GetValueType(&v));
    EXPECT_EQ_SIZE_T(5, GetValueArraySize(&v));
    EXPECT_EQ_INT(TYPE_NULL, GetValueType(GetValueArrayElement(&v, 0)));
    EXPECT_EQ_INT(TYPE_FALSE, GetValueType(GetValueArrayElement(&v, 1)));
    EXPECT_EQ_INT(TYPE_TRUE, GetValueType(GetValueArrayElement(&v, 2)));
    EXPECT_EQ_INT(TYPE_NUMBER, GetValueType(GetValueArrayElement(&v, 3)));
    EXPECT_EQ_INT(TYPE_STRING, GetValueType(GetValueArrayElement(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, GetValueNumber(GetValueArrayElement(&v, 3)));
    EXPECT_EQ_STRING("abc", GetValueString(GetValueArrayElement(&v, 4)), GetValueStringLength(GetValueArrayElement(&v, 4)));
    FreeValue(&v);

    size_t i, j;
    InitValue(&v);
    EXPECT_EQ_INT(PARSE_OK, ParseJsonString(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(TYPE_ARRAY, GetValueType(&v));
    EXPECT_EQ_SIZE_T(4, GetValueArraySize(&v));
    for (i = 0; i < 4; i++) {
        jsonValue* a = GetValueArrayElement(&v, i);
        EXPECT_EQ_INT(TYPE_ARRAY, GetValueType(a));
        EXPECT_EQ_SIZE_T(i, GetValueArraySize(a));
        for (j = 0; j < i; j++) {
            jsonValue* e = GetValueArrayElement(a, j);
            EXPECT_EQ_INT(TYPE_NUMBER, GetValueType(e));
            EXPECT_EQ_DOUBLE((double)j, GetValueNumber(e));
        }
    }
    FreeValue(&v);
}

static void test_parse_miss_key() {
    TEST_ERROR(PARSE_ERR_MISS_KEY, "{:1,");
    TEST_ERROR(PARSE_ERR_MISS_KEY, "{1:1,");
    TEST_ERROR(PARSE_ERR_MISS_KEY, "{true:1,");
    TEST_ERROR(PARSE_ERR_MISS_KEY, "{false:1,");
    TEST_ERROR(PARSE_ERR_MISS_KEY, "{null:1,");
    TEST_ERROR(PARSE_ERR_MISS_KEY, "{[]:1,");
    TEST_ERROR(PARSE_ERR_MISS_KEY, "{{}:1,");
    TEST_ERROR(PARSE_ERR_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
    TEST_ERROR(PARSE_ERR_MISS_COLON, "{\"a\"}");
    TEST_ERROR(PARSE_ERR_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
    TEST_ERROR(PARSE_ERR_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
    TEST_ERROR(PARSE_ERR_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
    TEST_ERROR(PARSE_ERR_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_ERROR(PARSE_ERR_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void test_parse_object() {
    jsonValue v;
    size_t i;

    InitValue(&v);
    EXPECT_EQ_INT(PARSE_OK, ParseJsonString(&v, " { } "));
    EXPECT_EQ_INT(TYPE_OBJECT, GetValueType(&v));
    EXPECT_EQ_SIZE_T(0, GetValueObjectSize(&v));
    FreeValue(&v);

    InitValue(&v);
    EXPECT_EQ_INT(PARSE_OK, ParseJsonString(&v,
        " { "
        "\"n\" : null , "
        "\"f\" : false , "
        "\"t\" : true , "
        "\"i\" : 123 , "
        "\"s\" : \"abc\", "
        "\"a\" : [ 1, 2, 3 ],"
        "\"o\" : { \"1\" : 1, \"2\" : 2, \"3\" : 3 }"
        " } "
    ));
    EXPECT_EQ_INT(TYPE_OBJECT, GetValueType(&v));
    EXPECT_EQ_SIZE_T(7, GetValueObjectSize(&v));
    EXPECT_EQ_STRING("n", GetValueObjectKey(&v, 0), GetValueObjectKeyLength(&v, 0));
    EXPECT_EQ_INT(TYPE_NULL, GetValueType(GetValueObjectValue(&v, 0)));
    EXPECT_EQ_STRING("f", GetValueObjectKey(&v, 1), GetValueObjectKeyLength(&v, 1));
    EXPECT_EQ_INT(TYPE_FALSE, GetValueType(GetValueObjectValue(&v, 1)));
    EXPECT_EQ_STRING("t", GetValueObjectKey(&v, 2), GetValueObjectKeyLength(&v, 2));
    EXPECT_EQ_INT(TYPE_TRUE, GetValueType(GetValueObjectValue(&v, 2)));
    EXPECT_EQ_STRING("i", GetValueObjectKey(&v, 3), GetValueObjectKeyLength(&v, 3));
    EXPECT_EQ_INT(TYPE_NUMBER, GetValueType(GetValueObjectValue(&v, 3)));
    EXPECT_EQ_DOUBLE(123.0, GetValueNumber(GetValueObjectValue(&v, 3)));
    EXPECT_EQ_STRING("s", GetValueObjectKey(&v, 4), GetValueObjectKeyLength(&v, 4));
    EXPECT_EQ_INT(TYPE_STRING, GetValueType(GetValueObjectValue(&v, 4)));
    EXPECT_EQ_STRING("abc", GetValueString(GetValueObjectValue(&v, 4)), GetValueStringLength(GetValueObjectValue(&v, 4)));
    EXPECT_EQ_STRING("a", GetValueObjectKey(&v, 5), GetValueObjectKeyLength(&v, 5));
    EXPECT_EQ_INT(TYPE_ARRAY, GetValueType(GetValueObjectValue(&v, 5)));
    EXPECT_EQ_SIZE_T(3, GetValueArraySize(GetValueObjectValue(&v, 5)));
    for (i = 0; i < 3; i++) {
        jsonValue* e = GetValueArrayElement(GetValueObjectValue(&v, 5), i);
        EXPECT_EQ_INT(TYPE_NUMBER, GetValueType(e));
        EXPECT_EQ_DOUBLE(i + 1.0, GetValueNumber(e));
    }
    EXPECT_EQ_STRING("o", GetValueObjectKey(&v, 6), GetValueObjectKeyLength(&v, 6));
    {
        jsonValue* o = GetValueObjectValue(&v, 6);
        EXPECT_EQ_INT(TYPE_OBJECT, GetValueType(o));
        for (i = 0; i < 3; i++) {
            jsonValue* ov = GetValueObjectValue(o, i);
            EXPECT_TRUE('1' + i == GetValueObjectKey(o, i)[0]);
            EXPECT_EQ_SIZE_T(1, GetValueObjectKeyLength(o, i));
            EXPECT_EQ_INT(TYPE_NUMBER, GetValueType(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, GetValueNumber(ov));
        }
    }
    FreeValue(&v);
}

static void test_parser_model()
{
    jsonValue v;

    InitValue(&v);
    EXPECT_EQ_INT(PARSE_OK, ParseJsonString(&v,
        " [ "
            " { "
                "\"vfi\": [ "
                    " { "
                        "\"model weights\": ["
                            " { "
                                "\"platform\": \"common\","
                                "\"blocks_0_conv_0_0_w_bin\" : [0, 0] ,"
                                "\"blocks_0_deconv_0_b_bin\" : [0, 100]"
                            " } "
                       " ] "
                    " } "
                " ] "
            " }, "

            " { "
                "\"vfi\": [ "
                    " { "
                        "\"model weights\": ["
                            " { "
                                "\"platform\": \"common\","
                                "\"blocks_0_conv_0_0_w_bin\" : [0, 0] ,"
                                "\"blocks_0_deconv_0_b_bin\" : [0, 100]"
                            " } "
                        " ] "
                    " } "
                " ] "
            " } "
        " ] "
    ));
    FreeValue(&v);

    InitValue(&v);
    EXPECT_EQ_INT(PARSE_OK, ParseJsonString(&v,
        "{ \
        \"vfi\": [\
        {\
            \"model weights\": [\
            {\
                \"platform\": \"common\",\
                    \"blocks_0_conv_0_0_w\" : [0, 2304] ,\
                    \"blocks_0_deconv_0_b\" : [2304, 128] ,\
                    \"blocks_0_deconv_0_w\" : [2432, 7168] ,\
                    \"blocks_1_conv_0_0_b\" : [9600, 384] ,\
                    \"blocks_1_conv_0_0_w\" : [9984, 5184] ,\
                    \"blocks_1_deconv_0_b\" : [15168, 128] ,\
                    \"blocks_1_deconv_0_w\" : [15296, 4096] ,\
                    \"blocks_2_conv_0_0_b\" : [19392, 192] ,\
                    \"blocks_2_conv_0_0_w\" : [19584, 2592] ,\
                    \"blocks_2_deconv_0_b\" : [22176, 128] ,\
                    \"blocks_2_deconv_0_w\" : [22304, 2048] ,\
                    \"decoder_residual_1_shortcut_b\" : [24352, 64] ,\
                    \"decoder_residual_2_shortcut_b\" : [24416, 64] ,\
                    \"decoder_residual_3_shortcut_b\" : [24480, 192] ,\
                    \"multiplier0\" : [24672, 132]\
            },\
                {\
                    \"platform\": \"xe\",\
                    \"blocks_0_conv_1_0_b\" : [24804, 2240] ,\
                    \"blocks_0_conv_1_0_w\" : [27044, 258048] ,\
                    \"blocks_0_conv_block_0_0_b\" : [285092, 2240] ,\
                    \"blocks_0_conv_block_0_0_w\" : [287332, 451584] ,\
                    \"blocks_0_conv_block_1_0_b\" : [738916, 2240] ,\
                    \"blocks_0_conv_block_1_0_w\" : [741156, 451584] ,\
                    \"blocks_0_conv_block_2_0_b\" : [1192740, 2240] ,\
                    \"blocks_0_conv_block_2_0_w\" : [1194980, 451584] ,\
                    \"blocks_0_conv_block_3_0_b\" : [1646564, 2240] ,\
                    \"blocks_0_conv_block_3_0_w\" : [1648804, 451584] ,\
                    \"blocks_0_conv_block_4_0_b\" : [2100388, 2240] ,\
                    \"blocks_0_conv_block_4_0_w\" : [2102628, 451584] ,\
                    \"blocks_0_conv_block_5_0_b\" : [2554212, 2240] ,\
                    \"blocks_0_conv_block_5_0_w\" : [2556452, 451584] ,\
                    \"blocks_0_conv_block_6_0_b\" : [3008036, 2240] ,\
                    \"blocks_0_conv_block_6_0_w\" : [3010276, 451584] ,\
                    \"blocks_0_conv_block_7_0_b\" : [3461860, 2240] ,\
                    \"blocks_0_conv_block_7_0_w\" : [3464100, 451584] ,\
                    \"blocks_1_conv_1_0_b\" : [3915684, 1280] ,\
                    \"blocks_1_conv_1_0_w\" : [3916964, 73728] ,\
                    \"blocks_1_conv_block_0_0_b\" : [3990692, 1280] ,\
                    \"blocks_1_conv_block_0_0_w\" : [3991972, 147456] ,\
                    \"blocks_1_conv_block_1_0_b\" : [4139428, 1280] ,\
                    \"blocks_1_conv_block_1_0_w\" : [4140708, 147456] ,\
                    \"blocks_1_conv_block_2_0_b\" : [4288164, 1280] ,\
                    \"blocks_1_conv_block_2_0_w\" : [4289444, 147456] ,\
                    \"blocks_1_conv_block_3_0_b\" : [4436900, 1280] ,\
                    \"blocks_1_conv_block_3_0_w\" : [4438180, 147456] ,\
                    \"blocks_1_conv_block_4_0_b\" : [4585636, 1280] ,\
                    \"blocks_1_conv_block_4_0_w\" : [4586916, 147456] ,\
                    \"blocks_1_conv_block_5_0_b\" : [4734372, 1280] ,\
                    \"blocks_1_conv_block_5_0_w\" : [4735652, 147456] ,\
                    \"blocks_1_conv_block_6_0_b\" : [4883108, 1280] ,\
                    \"blocks_1_conv_block_6_0_w\" : [4884388, 147456] ,\
                    \"blocks_1_conv_block_7_0_b\" : [5031844, 1280] ,\
                    \"blocks_1_conv_block_7_0_w\" : [5033124, 147456] ,\
                    \"blocks_2_conv_1_0_b\" : [5180580, 640] ,\
                    \"blocks_2_conv_1_0_w\" : [5181220, 18432] ,\
                    \"blocks_2_conv_block_0_0_b\" : [5199652, 640] ,\
                    \"blocks_2_conv_block_0_0_w\" : [5200292, 36864] ,\
                    \"blocks_2_conv_block_1_0_b\" : [5237156, 640] ,\
                    \"blocks_2_conv_block_1_0_w\" : [5237796, 36864] ,\
                    \"blocks_2_conv_block_2_0_b\" : [5274660, 640] ,\
                    \"blocks_2_conv_block_2_0_w\" : [5275300, 36864] ,\
                    \"blocks_2_conv_block_3_0_b\" : [5312164, 640] ,\
                    \"blocks_2_conv_block_3_0_w\" : [5312804, 36864] ,\
                    \"blocks_2_conv_block_4_0_b\" : [5349668, 640] ,\
                    \"blocks_2_conv_block_4_0_w\" : [5350308, 36864] ,\
                    \"blocks_2_conv_block_5_0_b\" : [5387172, 640] ,\
                    \"blocks_2_conv_block_5_0_w\" : [5387812, 36864] ,\
                    \"blocks_2_conv_block_6_0_b\" : [5424676, 640] ,\
                    \"blocks_2_conv_block_6_0_w\" : [5425316, 36864] ,\
                    \"blocks_2_conv_block_7_0_b\" : [5462180, 640] ,\
                    \"blocks_2_conv_block_7_0_w\" : [5462820, 36864]\
                },\
                {\
                    \"platform\": \"xe2\",\
                    \"blocks_0_conv_1_0_b\" : [5499684, 2240] ,\
                    \"blocks_0_conv_1_0_w\" : [5501924, 258048] ,\
                    \"blocks_0_conv_block_0_0_b\" : [5759972, 2240] ,\
                    \"blocks_0_conv_block_0_0_w\" : [5762212, 451584] ,\
                    \"blocks_0_conv_block_1_0_b\" : [6213796, 2240] ,\
                    \"blocks_0_conv_block_1_0_w\" : [6216036, 451584] ,\
                    \"blocks_0_conv_block_2_0_b\" : [6667620, 2240] ,\
                    \"blocks_0_conv_block_2_0_w\" : [6669860, 451584] ,\
                    \"blocks_0_conv_block_3_0_b\" : [7121444, 2240] ,\
                    \"blocks_0_conv_block_3_0_w\" : [7123684, 451584] ,\
                    \"blocks_0_conv_block_4_0_b\" : [7575268, 2240] ,\
                    \"blocks_0_conv_block_4_0_w\" : [7577508, 451584] ,\
                    \"blocks_0_conv_block_5_0_b\" : [8029092, 2240] ,\
                    \"blocks_0_conv_block_5_0_w\" : [8031332, 451584] ,\
                    \"blocks_0_conv_block_6_0_b\" : [8482916, 2240] ,\
                    \"blocks_0_conv_block_6_0_w\" : [8485156, 451584] ,\
                    \"blocks_0_conv_block_7_0_b\" : [8936740, 2240] ,\
                    \"blocks_0_conv_block_7_0_w\" : [8938980, 451584] ,\
                    \"blocks_1_conv_1_0_b\" : [9390564, 1280] ,\
                    \"blocks_1_conv_1_0_w\" : [9391844, 73728] ,\
                    \"blocks_1_conv_block_0_0_b\" : [9465572, 1280] ,\
                    \"blocks_1_conv_block_0_0_w\" : [9466852, 147456] ,\
                    \"blocks_1_conv_block_1_0_b\" : [9614308, 1280] ,\
                    \"blocks_1_conv_block_1_0_w\" : [9615588, 147456] ,\
                    \"blocks_1_conv_block_2_0_b\" : [9763044, 1280] ,\
                    \"blocks_1_conv_block_2_0_w\" : [9764324, 147456] ,\
                    \"blocks_1_conv_block_3_0_b\" : [9911780, 1280] ,\
                    \"blocks_1_conv_block_3_0_w\" : [9913060, 147456] ,\
                    \"blocks_1_conv_block_4_0_b\" : [10060516, 1280] ,\
                    \"blocks_1_conv_block_4_0_w\" : [10061796, 147456] ,\
                    \"blocks_1_conv_block_5_0_b\" : [10209252, 1280] ,\
                    \"blocks_1_conv_block_5_0_w\" : [10210532, 147456] ,\
                    \"blocks_1_conv_block_6_0_b\" : [10357988, 1280] ,\
                    \"blocks_1_conv_block_6_0_w\" : [10359268, 147456] ,\
                    \"blocks_1_conv_block_7_0_b\" : [10506724, 1280] ,\
                    \"blocks_1_conv_block_7_0_w\" : [10508004, 147456] ,\
                    \"blocks_2_conv_1_0_b\" : [10655460, 640] ,\
                    \"blocks_2_conv_1_0_w\" : [10656100, 18432] ,\
                    \"blocks_2_conv_block_0_0_b\" : [10674532, 640] ,\
                    \"blocks_2_conv_block_0_0_w\" : [10675172, 36864] ,\
                    \"blocks_2_conv_block_1_0_b\" : [10712036, 640] ,\
                    \"blocks_2_conv_block_1_0_w\" : [10712676, 36864] ,\
                    \"blocks_2_conv_block_2_0_b\" : [10749540, 640] ,\
                    \"blocks_2_conv_block_2_0_w\" : [10750180, 36864] ,\
                    \"blocks_2_conv_block_3_0_b\" : [10787044, 640] ,\
                    \"blocks_2_conv_block_3_0_w\" : [10787684, 36864] ,\
                    \"blocks_2_conv_block_4_0_b\" : [10824548, 640] ,\
                    \"blocks_2_conv_block_4_0_w\" : [10825188, 36864] ,\
                    \"blocks_2_conv_block_5_0_b\" : [10862052, 640] ,\
                    \"blocks_2_conv_block_5_0_w\" : [10862692, 36864] ,\
                    \"blocks_2_conv_block_6_0_b\" : [10899556, 640] ,\
                    \"blocks_2_conv_block_6_0_w\" : [10900196, 36864] ,\
                    \"blocks_2_conv_block_7_0_b\" : [10937060, 640] ,\
                    \"blocks_2_conv_block_7_0_w\" : [10937700, 36864]\
                }\
            ]\
        }\
        ]\
    }"
    ));
    FreeValue(&v);
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_string();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
    test_parse_missing_quotation_mark();
    test_parse_invalid_string_escape();
    test_parse_invalid_string_char();

    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();

    // todo
    //test_parse_invalid_unicode_hex();
    //test_parse_invalid_unicode_surrogate();

    test_parse_array();

    test_parse_object();

    test_parse_miss_comma_or_square_bracket();
    test_parse_miss_key();
    test_parse_miss_colon();
    test_parse_miss_comma_or_curly_bracket();
}

static void test_access() {
    test_access_null();
    test_access_boolean();
    test_access_number();
    test_access_string();
}

int main() {
    while (1) {
        test_parse();
        test_access();
        test_parser_model();
        printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    }
    system("pause");
    return main_ret;
}