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
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "null"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse_true() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "true"));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(&v));
}

static void test_parse_false() {
    lept_value v;
    v.type = LEPT_TRUE;
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "false"));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(&v));
}

#define TEST_NUMBER(expect, json)\
    do {\
        lept_value v;\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, GetNumber(&v));\
    } while(0)

#define TEST_ERROR(error, json)\
    do {\
        lept_value v;\
        v.type = LEPT_FALSE;\
        EXPECT_EQ_INT(error, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));\
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
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?");

    /* invalid number */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
}

static void test_parse_root_not_singular() {
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "null x");

    /* invalid number */
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0123"); /* after zero should be '.' , 'E' , 'e' or nothing */
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x0");
    TEST_ERROR(LEPT_PARSE_ROOT_NOT_SINGULAR, "0x123");
}

static void test_parse_number_too_big() {
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
}

static void test_parse_missing_quotation_mark() {
    TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"");
    TEST_ERROR(LEPT_PARSE_MISS_QUOTATION_MARK, "\"abc");
}

static void test_parse_invalid_string_escape() {
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\v\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\'\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_ESCAPE, "\"\\x12\"");
}

static void test_parse_invalid_string_char() {
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x01\"");
    TEST_ERROR(LEPT_PARSE_INVALID_STRING_CHAR, "\"\x1F\"");
}

static void test_access_null() {
    lept_value v;
    InitValue(&v);
    SetString(&v, "a", 1);
    SetNull(&v);
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
    FreeValue(&v);
}


#define TEST_STRING(expect, json)\
    do {\
        lept_value v;\
        InitValue(&v);\
        EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, json));\
        EXPECT_EQ_INT(LEPT_STRING, lept_get_type(&v));\
        EXPECT_EQ_STRING(expect, GetString(&v), GetStringLength(&v));\
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
    lept_value v;
    InitValue(&v);
    SetString(&v, "a", 1);
    SetBoolean(&v, true);
    EXPECT_TRUE(GetBoolean(&v));
    SetBoolean(&v, false);
    EXPECT_FALSE(GetBoolean(&v));
    FreeValue(&v);
}

static void test_access_number() {
    lept_value v;
    InitValue(&v);
    SetString(&v, "a", 1);
    SetNumber(&v, 1234.5);
    EXPECT_EQ_DOUBLE(1234.5, GetNumber(&v));
    FreeValue(&v);
}

static void test_access_string() {
    lept_value v;
    InitValue(&v);
    SetString(&v, "", 0);
    EXPECT_EQ_STRING("", GetString(&v), GetStringLength(&v));
    SetString(&v, "Hello", 5);
    EXPECT_EQ_STRING("Hello", GetString(&v), GetStringLength(&v));
    FreeValue(&v);
}

static void test_parse_invalid_unicode_hex() {
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u01\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u012\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u/000\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\uG000\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0/00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u0G00\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00/0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u00G0\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000/\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_HEX, "\"\\u000G\"");
}

static void test_parse_invalid_unicode_surrogate() {
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uDBFF\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\\\\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uDBFF\"");
    TEST_ERROR(LEPT_PARSE_INVALID_UNICODE_SURROGATE, "\"\\uD800\\uE000\"");
}

static void test_parse_miss_comma_or_square_bracket() {
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1}");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[1 2");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET, "[[]");
}

static void test_parse_array() {
    lept_value v;

    InitValue(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(0, GetArraySize(&v));
    FreeValue(&v);

    InitValue(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ null , false , true , 123 , \"abc\" ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(5, GetArraySize(&v));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(GetArrayElement(&v, 0)));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(GetArrayElement(&v, 1)));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(GetArrayElement(&v, 2)));
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(GetArrayElement(&v, 3)));
    EXPECT_EQ_INT(LEPT_STRING, lept_get_type(GetArrayElement(&v, 4)));
    EXPECT_EQ_DOUBLE(123.0, GetNumber(GetArrayElement(&v, 3)));
    EXPECT_EQ_STRING("abc", GetString(GetArrayElement(&v, 4)), GetStringLength(GetArrayElement(&v, 4)));
    FreeValue(&v);

    size_t i, j;
    InitValue(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, "[ [ ] , [ 0 ] , [ 0 , 1 ] , [ 0 , 1 , 2 ] ]"));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(4, GetArraySize(&v));
    for (i = 0; i < 4; i++) {
        lept_value* a = GetArrayElement(&v, i);
        EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(a));
        EXPECT_EQ_SIZE_T(i, GetArraySize(a));
        for (j = 0; j < i; j++) {
            lept_value* e = GetArrayElement(a, j);
            EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
            EXPECT_EQ_DOUBLE((double)j, GetNumber(e));
        }
    }
    FreeValue(&v);
}

static void test_parse_miss_key() {
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{1:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{true:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{false:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{null:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{[]:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{{}:1,");
    TEST_ERROR(LEPT_PARSE_MISS_KEY, "{\"a\":1,");
}

static void test_parse_miss_colon() {
    TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\"}");
    TEST_ERROR(LEPT_PARSE_MISS_COLON, "{\"a\",\"b\"}");
}

static void test_parse_miss_comma_or_curly_bracket() {
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1]");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":1 \"b\"");
    TEST_ERROR(LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET, "{\"a\":{}");
}

static void test_parse_object() {
    lept_value v;
    size_t i;

    InitValue(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v, " { } "));
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(0, GetObjectSize(&v));
    FreeValue(&v);

    InitValue(&v);
    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,
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
    EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(&v));
    EXPECT_EQ_SIZE_T(7, GetObjectSize(&v));
    EXPECT_EQ_STRING("n", GetObjectKey(&v, 0), GetObjectKeyLength(&v, 0));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(GetObjectValue(&v, 0)));
    EXPECT_EQ_STRING("f", GetObjectKey(&v, 1), GetObjectKeyLength(&v, 1));
    EXPECT_EQ_INT(LEPT_FALSE, lept_get_type(GetObjectValue(&v, 1)));
    EXPECT_EQ_STRING("t", GetObjectKey(&v, 2), GetObjectKeyLength(&v, 2));
    EXPECT_EQ_INT(LEPT_TRUE, lept_get_type(GetObjectValue(&v, 2)));
    EXPECT_EQ_STRING("i", GetObjectKey(&v, 3), GetObjectKeyLength(&v, 3));
    EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(GetObjectValue(&v, 3)));
    EXPECT_EQ_DOUBLE(123.0, GetNumber(GetObjectValue(&v, 3)));
    EXPECT_EQ_STRING("s", GetObjectKey(&v, 4), GetObjectKeyLength(&v, 4));
    EXPECT_EQ_INT(LEPT_STRING, lept_get_type(GetObjectValue(&v, 4)));
    EXPECT_EQ_STRING("abc", GetString(GetObjectValue(&v, 4)), GetStringLength(GetObjectValue(&v, 4)));
    EXPECT_EQ_STRING("a", GetObjectKey(&v, 5), GetObjectKeyLength(&v, 5));
    EXPECT_EQ_INT(LEPT_ARRAY, lept_get_type(GetObjectValue(&v, 5)));
    EXPECT_EQ_SIZE_T(3, GetArraySize(GetObjectValue(&v, 5)));
    for (i = 0; i < 3; i++) {
        lept_value* e = GetArrayElement(GetObjectValue(&v, 5), i);
        EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(e));
        EXPECT_EQ_DOUBLE(i + 1.0, GetNumber(e));
    }
    EXPECT_EQ_STRING("o", GetObjectKey(&v, 6), GetObjectKeyLength(&v, 6));
    {
        lept_value* o = GetObjectValue(&v, 6);
        EXPECT_EQ_INT(LEPT_OBJECT, lept_get_type(o));
        for (i = 0; i < 3; i++) {
            lept_value* ov = GetObjectValue(o, i);
            EXPECT_TRUE('1' + i == GetObjectKey(o, i)[0]);
            EXPECT_EQ_SIZE_T(1, GetObjectKeyLength(o, i));
            EXPECT_EQ_INT(LEPT_NUMBER, lept_get_type(ov));
            EXPECT_EQ_DOUBLE(i + 1.0, GetNumber(ov));
        }
    }
    FreeValue(&v);
}

static void test_parser_model()
{
    lept_value v;
    InitValue(&v);

    EXPECT_EQ_INT(LEPT_PARSE_OK, lept_parse(&v,
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
    test_parse();
    test_access();
    test_parser_model();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}