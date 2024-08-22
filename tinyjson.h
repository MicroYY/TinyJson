#ifndef JSON_PARSER_H__
#define JSON_PARSER_H__

#include <stdio.h>

enum valueType
{
    TYPE_NULL,
    TYPE_FALSE,
    TYPE_TRUE,
    TYPE_NUMBER,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_OBJECT
};

enum parseStatus {
    PARSE_OK = 0,
    PARSE_ERR_EXPECT_VALUE,
    PARSE_ERR_INVALID_VALUE,
    PARSE_ERR_ROOT_NOT_SINGULAR,
    PARSE_ERR_NUMBER_OVERFLOW,
    PARSE_ERR_MISS_QUOTATION_MARK,
    PARSE_ERR_INVALID_ESCAPE_CHAR,
    PARSE_ERR_CONTROL_CHAR,
    PARSE_ERR_MISS_COMMA_OR_SQUARE_BRACKET,
    PARSE_ERR_MISS_KEY,
    PARSE_ERR_MISS_COLON,
    PARSE_ERR_MISS_COMMA_OR_CURLY_BRACKET
};

struct jsonMap;

struct jsonValue {
    valueType type;
    union {
        struct { jsonMap* maps; size_t size; } obj;
        struct { jsonValue* values; size_t size; } arr;
        struct { char* s; size_t len; } str;
        double num;
    };
};

struct jsonMap {
    char* key;
    size_t keyLen;
    jsonValue value;
};

void InitValue(jsonValue* v);
void FreeValue(jsonValue* v);

parseStatus ParseJsonString(jsonValue* v, const char* json);
valueType   GetValueType(const jsonValue* v);

double GetValueNumber(const jsonValue* v);
void   SetValueNumber(jsonValue* v, double n);

const char* GetValueString(const jsonValue* v);
size_t      GetValueStringLength(const jsonValue* v);
void        SetValueString(jsonValue* v, const char* s, size_t len);

void SetValueBoolean(jsonValue* v, bool b);
bool GetValueBoolean(const jsonValue* v);

size_t     GetValueArraySize(const jsonValue* v);
jsonValue* GetValueArrayElement(const jsonValue* v, size_t index);

size_t      GetValueObjectSize(const jsonValue* v);
const char* GetValueObjectKey(const jsonValue* v, size_t index);
size_t      GetValueObjectKeyLength(const jsonValue* v, size_t index);
jsonValue* GetValueObjectValue(const jsonValue* v, size_t index);

#endif /* JSON_PARSER_H__ */