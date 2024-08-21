#ifndef LEPTJSON_H__
#define LEPTJSON_H__

#include <stdio.h>

enum lept_type 
{
    LEPT_NULL,
    LEPT_FALSE,
    LEPT_TRUE,
    LEPT_NUMBER,
    LEPT_STRING,
    LEPT_ARRAY,
    LEPT_OBJECT
};

enum ParseStatus{
    LEPT_PARSE_OK = 0,
    LEPT_PARSE_EXPECT_VALUE,
    LEPT_PARSE_INVALID_VALUE,
    LEPT_PARSE_ROOT_NOT_SINGULAR,
    LEPT_PARSE_NUMBER_TOO_BIG,
    LEPT_PARSE_MISS_QUOTATION_MARK,
    LEPT_PARSE_INVALID_STRING_ESCAPE,
    LEPT_PARSE_INVALID_STRING_CHAR,
    LEPT_PARSE_INVALID_UNICODE_HEX,
    LEPT_PARSE_INVALID_UNICODE_SURROGATE,
    LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET,
    LEPT_PARSE_MISS_KEY,
    LEPT_PARSE_MISS_COLON,
    LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET
};

struct lept_member;

struct lept_value {
    lept_type type;
    union {
        struct { lept_member* members; size_t size; } obj;
        struct { lept_value* values; size_t size; } arr;
        struct { char* s; size_t len; } str;
        double num;
    };
};

struct lept_member {
    char* key;
    size_t keyLen;
    lept_value value;
};

void FreeValue(lept_value* v);
#define InitValue(v) (v)->type = LEPT_NULL;
#define SetNull(v) FreeValue(v)

int lept_parse(lept_value* v, const char* json);
lept_type lept_get_type(const lept_value* v);

double GetNumber(const lept_value* v);
void SetNumber(lept_value* v, double n);

const char* GetString(const lept_value* v);
size_t GetStringLength(const lept_value* v);
void SetString(lept_value* v, const char* s, size_t len);

void SetBoolean(lept_value* v, bool b);
bool GetBoolean(const lept_value* v);

size_t GetArraySize(const lept_value* v);
lept_value* GetArrayElement(const lept_value* v, size_t index);

size_t GetObjectSize(const lept_value* v);
const char* GetObjectKey(const lept_value* v, size_t index);
size_t GetObjectKeyLength(const lept_value* v, size_t index);
lept_value* GetObjectValue(const lept_value* v, size_t index);

#endif /* LEPTJSON_H__ */