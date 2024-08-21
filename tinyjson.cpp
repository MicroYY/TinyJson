
#include <stdio.h>
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */
#include <errno.h>
#include <cmath>
#include <cstring>
#include "tinyjson.h"

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')

#define STACK_INIT_SIZE 256

#define PUTC(c, ch) do { *(char*)ContextPush(c, sizeof(char)) = (ch); } while(0)

typedef struct {
	const char* json;
	char* stack;
	size_t size, top;
}lept_context;

static int ParseValue(lept_context* c, lept_value* v);

double GetNumber(const lept_value* v) {
	assert(v != NULL && v->type == LEPT_NUMBER);
	return v->num;
}
void SetNumber(lept_value* v, double n) {
	FreeValue(v);
	v->num = n;
	v->type = LEPT_NUMBER;
}

const char* GetString(const lept_value* v)
{
	assert(v && v->type == LEPT_STRING);
	return v->str.s;
}

size_t GetStringLength(const lept_value* v)
{
	assert(v && v->type == LEPT_STRING);
	return v->str.len;
}

void SetString(lept_value* v, const char* s, size_t len)
{
	assert(v && (s || len == 0));
	FreeValue(v);
	v->str.s = (char*)malloc(len + 1);
	memcpy(v->str.s, s, len);
	v->str.s[len] = '\0';
	v->str.len = len;
	v->type = LEPT_STRING;
}

void SetBoolean(lept_value* v, bool b)
{
	FreeValue(v);
	v->type = b ? LEPT_TRUE : LEPT_FALSE;
}

bool GetBoolean(const lept_value* v)
{
	assert(v && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
	return v->type == LEPT_TRUE;
}

size_t GetArraySize(const lept_value* v)
{
	assert(v && v->type == LEPT_ARRAY);
	return v->arr.size;
}

lept_value* GetArrayElement(const lept_value* v, size_t index)
{
	assert(v && v->type == LEPT_ARRAY && index < v->arr.size);
	return &v->arr.values[index];
}

size_t GetObjectSize(const lept_value* v)
{
	assert(v && v->type == LEPT_OBJECT);
	return v->obj.size;
}

const char* GetObjectKey(const lept_value* v, size_t index)
{
	assert(v && v->type == LEPT_OBJECT);
	assert(index < v->obj.size);
	return v->obj.members[index].key;
}

size_t GetObjectKeyLength(const lept_value* v, size_t index)
{
	assert(v && v->type == LEPT_OBJECT);
	assert(index < v->obj.size);
	return v->obj.members[index].keyLen;
}

lept_value* GetObjectValue(const lept_value* v, size_t index)
{
	assert(v && v->type == LEPT_OBJECT);
	assert(index < v->obj.size);
	return &v->obj.members[index].value;
}

static void* ContextPush(lept_context* c, size_t size)
{
	assert(size > 0);

	void* ret;

	if (c->top + size >= c->size) {
		if (c->size == 0) {
			c->size = STACK_INIT_SIZE;
		}
		while (c->top + size >= c->size) {
			c->size += c->size >> 1;
		}
		c->stack = (char*)realloc(c->stack, c->size);
	}
	ret = c->stack + c->top;
	c->top += size;
	return ret;
}

static void* ContextPop(lept_context* c, size_t size)
{
	assert(c->top >= size);
	return c->stack + (c->top -= size);
}

void FreeValue(lept_value* v)
{
	assert(v);
	if (v->type == LEPT_STRING) {
		free(v->str.s);
	}
	v->type = LEPT_NULL;
}

static void ParseWhitespace(lept_context* c) {
	const char* p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
		p++;
	c->json = p;
}

static int ParseStringRaw(lept_context* c, char** str, size_t& len)
{
	EXPECT(c, '\"');

	size_t head = c->top;

	const char* p = c->json;
	while (1) {
		char ch = *(p++);
		switch (ch) {
		case '\\':
			switch (*(p++)) {
			case 'n':  PUTC(c, '\n'); break;
			case '\"': PUTC(c, '\"'); break;
			case '\\': PUTC(c, '\\'); break;
			case '/':  PUTC(c, '/'); break;
			case 'b':  PUTC(c, '\b'); break;
			case 'f':  PUTC(c, '\f'); break;
			case 'r':  PUTC(c, '\r'); break;
			case 't':  PUTC(c, '\t'); break;
			default:
				c->top = head;
				return LEPT_PARSE_INVALID_STRING_ESCAPE;
			}
			break;
		case '\"':
			len = c->top - head;
			c->json = p;
			*str = (char*)ContextPop(c, len);
			return LEPT_PARSE_OK;
		case '\0':
			c->top = head;
			return LEPT_PARSE_MISS_QUOTATION_MARK;
		default:
			// if ch is control characters
			if ((unsigned char)ch < 0x20) {
				c->top = head;
				return LEPT_PARSE_INVALID_STRING_CHAR;
			}
			PUTC(c, ch);
		}
	}
}

static int ParseString(lept_context* c, lept_value* v)
{
	int ret;

	char* s;
	size_t len;
	if ((ret = ParseStringRaw(c, &s, len)) == LEPT_PARSE_OK) {
		SetString(v, s, len);
	}
	return ret;
}

static int ParseObject(lept_context* c, lept_value* v)
{
	EXPECT(c, '{');

	int ret = 0;
	size_t size = 0;
	lept_member m;
	m.key = nullptr;

	ParseWhitespace(c);
	if (*c->json == '}')
	{
		c->json++;
		v->type = LEPT_OBJECT;
		v->obj.members = nullptr;
		v->obj.size = 0;
		return LEPT_PARSE_OK;
	}

	while (1)
	{
		InitValue(&m.value);

		char* str;

		if (*c->json != '"')
		{
			ret = LEPT_PARSE_MISS_KEY;
			break;
		}
		if ((ret = ParseStringRaw(c, &str, m.keyLen)) != LEPT_PARSE_OK)
		{
			break;
		}
		memcpy(m.key = (char*)realloc(m.key, m.keyLen + 1), str, m.keyLen);
		m.key[m.keyLen] = '\0';

		
		ParseWhitespace(c);
		if (*c->json != ':')
		{
			ret = LEPT_PARSE_MISS_COLON;
			break;
		}

		c->json++;
		ParseWhitespace(c);

		if ((ret = ParseValue(c, &m.value)) != LEPT_PARSE_OK)
		{
			break;
		}
		memcpy(ContextPush(c, sizeof(lept_member)), &m, sizeof(lept_member));

		size++;
		m.key = nullptr;

		ParseWhitespace(c);

		if (*c->json == ',')
		{
			c->json++;
			ParseWhitespace(c);
		}
		else if (*c->json == '}')
		{
			size_t copySz = sizeof(lept_member) * size;
			c->json++;
			v->type = LEPT_OBJECT;
			v->obj.size = size;
			memcpy(v->obj.members = (lept_member*)malloc(copySz), ContextPop(c, copySz), copySz);
			return LEPT_PARSE_OK;
		}
		else
		{
			ret = LEPT_PARSE_MISS_COMMA_OR_CURLY_BRACKET;
			break;
		}
	}

	free(m.key);
	for (size_t i = 0; i < size; i++)
	{
		lept_member* m = (lept_member*)ContextPop(c, sizeof(lept_member));
		free(m->key);
		FreeValue(&m->value);
	}
	v->type = LEPT_NULL;

	return ret;
}

static int ParseLiteral(lept_context* c, lept_value* v, const char* literal, lept_type type)
{
	EXPECT(c, literal[0]);

	size_t i;
	for (i = 0; literal[i + 1] != '\0'; i++) {
		if (c->json[i] != literal[i + 1]) {
			return LEPT_PARSE_INVALID_VALUE;
		}
	}
	c->json += i;
	v->type = type;
	return LEPT_PARSE_OK;
}

static int ParseNumber(lept_context* c, lept_value* v) {
	const char* p = c->json;

	bool isPositive = true;
	if (*p == '-') {
		isPositive = false;
		p++;
	}

	if (!ISDIGIT(*p)) {
		return LEPT_PARSE_INVALID_VALUE;
	}

	if (*p == '0') {
		p++;
		//if (*p != '\0' && *p != '.' && *p != 'e' && *p != 'E') {
		//    return LEPT_PARSE_ROOT_NOT_SINGULAR;
		//}
	}
	else {
		if (!ISDIGIT(*p)) {
			return LEPT_PARSE_INVALID_VALUE;
		}
		for (p++; ISDIGIT(*p); p++) {
		}
	}

	if (*p == '.') {
		p++;
		if (!ISDIGIT(*p)) {
			return LEPT_PARSE_INVALID_VALUE;
		}
		for (p++; ISDIGIT(*p); p++) {
		}
	}

	if (*p == 'e' || *p == 'E') {
		p++;
		if (*p == '+' || *p == '-') {
			p++;
		}
		if (!ISDIGIT(*p)) {
			return LEPT_PARSE_INVALID_VALUE;
		}
		for (p++; ISDIGIT(*p); p++) {
		}
	}

	errno = 0;
	v->num = strtod(c->json, NULL);
	if (errno == ERANGE && (v->num == HUGE_VAL || v->num == -HUGE_VAL)) {
		return LEPT_PARSE_NUMBER_TOO_BIG;
	}
	c->json = p;
	v->type = LEPT_NUMBER;
	return LEPT_PARSE_OK;
}

static int ParseArray(lept_context* c, lept_value* v)
{
	EXPECT(c, '[');

	int ret;

	size_t size = 0;

	ParseWhitespace(c);
	if (*c->json == ']')
	{
		c->json++;
		v->type = LEPT_ARRAY;
		v->arr.size = 0;
		v->arr.values = nullptr;
		return LEPT_PARSE_OK;
	}
	while (1)
	{
		lept_value e;
		InitValue(&e);
		ParseWhitespace(c);
		if (ret = ParseValue(c, &e) != LEPT_PARSE_OK)
		{
			break;
		}
		memcpy(ContextPush(c, sizeof(lept_value)), &e, sizeof(lept_value));
		size++;

		ParseWhitespace(c);

		if (*c->json == ',')
		{
			c->json++;
		}
		else if (*c->json == ']')
		{
			c->json++;
			v->type = LEPT_ARRAY;
			v->arr.size = size;
			v->arr.values = (lept_value*)malloc(size * sizeof(lept_value));
			memcpy(v->arr.values, ContextPop(c, size * sizeof(lept_value)), size * sizeof(lept_value));
			return LEPT_PARSE_OK;
		}
		else
		{
			ret = LEPT_PARSE_MISS_COMMA_OR_SQUARE_BRACKET;
			break;
		}
	}

	for (size_t i = 0; i < size; i++) {
		FreeValue((lept_value*)ContextPop(c, sizeof(lept_value)));
	}

	return ret;
}

static int ParseValue(lept_context* c, lept_value* v) {
	switch (*c->json) {
	case 't':  return ParseLiteral(c, v, "true", LEPT_TRUE);
	case 'f':  return ParseLiteral(c, v, "false", LEPT_FALSE);
	case 'n':  return ParseLiteral(c, v, "null", LEPT_NULL);
	case '\0': return LEPT_PARSE_EXPECT_VALUE;
	case '"':  return ParseString(c, v);
	case '[':  return ParseArray(c, v);
	case '{':  return ParseObject(c, v);
	default:   return ParseNumber(c, v);
	}
}

int lept_parse(lept_value* v, const char* json) {
	assert(v != NULL);

	lept_context c;
	c.json = json;
	c.stack = nullptr;
	c.size = c.top = 0;

	InitValue(v);

	int ret;

	ParseWhitespace(&c);
	if ((ret = ParseValue(&c, v)) == LEPT_PARSE_OK) {
		ParseWhitespace(&c);
		if (*c.json != '\0') {
			v->type = LEPT_NULL;
			ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
		}
	}
	assert(!c.top);
	free(c.stack);
	return ret;
}

lept_type lept_get_type(const lept_value* v) {
	assert(v != NULL);
	return v->type;
}
