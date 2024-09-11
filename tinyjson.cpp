#include "tinyjson.h"
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <cmath>
#include <cstring>

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')

#define STACK_INIT_SIZE 256

#define STRING_ERROR(ret) { c->top = head; return ret; }

struct parserStack {
	const char* json;
	char* stack;
	size_t size, top;

	~parserStack()
	{
		free(this->stack);
	}

	// return old top
	void* PushSz(size_t sz)
	{
		assert(sz > 0);
		char* ret;

		if (this->top + sz >= this->size) {
			if (this->size == 0) {
				this->size = STACK_INIT_SIZE;
			}
			while (this->top + sz >= this->size) {
				this->size += this->size >> 1;
			}
			void* ptr = realloc(this->stack, this->size);
			assert(ptr);
			this->stack = (char*)ptr;
		}
		ret = this->stack + this->top;
		this->top += sz;

		return ret;
	}

	void PushChar(char ch)
	{
		*(char*)PushSz(sizeof(char)) = ch;
	}

	void* Pop(size_t sz)
	{
		assert(this->top >= sz);
		return this->stack + (this->top -= sz);
	}
};

static parseStatus ParseValue(parserStack* c, jsonValue* v);

void InitValue(jsonValue* v)
{
	v->type = TYPE_NULL;
}

void FreeValue(jsonValue* v)
{
	assert(v);
	switch (v->type)
	{
		case TYPE_STRING:
			free(v->str.s);
			break;
		case TYPE_ARRAY:
			for (size_t i = 0; i < v->arr.size; i++)
			{
				FreeValue(&v->arr.values[i]);
			}
			free(v->arr.values);
			break;
		case TYPE_OBJECT:
			for (size_t i = 0; i < v->obj.size; i++)
			{
				free(v->obj.maps[i].key);
				FreeValue(&v->obj.maps[i].value);
			}
			free(v->obj.maps);
			break;
	default:
		break;
	}

	v->type = TYPE_NULL;
}

void SetValueString(jsonValue* v, const char* s, size_t len)
{
	assert(v && (s || len == 0));
	FreeValue(v);
	v->str.s = (char*)malloc(len + 1);
	memcpy(v->str.s, s, len);
	v->str.s[len] = '\0';
	v->str.len = len;
	v->type = TYPE_STRING;
}

static void ParseWhitespace(parserStack* c) {
	const char* p = c->json;
	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
		p++;
	c->json = p;
}

static const char* ParseHex4(const char* p, unsigned* u)
{
	*u = 0;
	for (int i = 0; i < 4; i++) {
		char c = *p;
		p++;
		*u <<= 4;
		if (c >= '0' && c <= '9') {
			*u |= c - '0';
		}
		else if (c >= 'A' && c <= 'F') {
			*u |= c - ('A' - 10);
		}
		else if (c >= 'a' && c <= 'f') {
			*u |= c - ('a' - 10);
		}
		else return nullptr;
	}
	return p;
}

static void EncodeUtf8(parserStack* c, unsigned u) {
	if (u <= 0x7F)
		c->PushChar(u & 0xff);
	else if (u <= 0x7FF) {
		c->PushChar(0xC0 | ((u >> 6) & 0xFF));
		c->PushChar(0x80 | (u & 0x3F));
	}
	else if (u <= 0xFFFF) {
		c->PushChar(0xE0 | ((u >> 12) & 0xFF));
		c->PushChar(0x80 | ((u >> 6) & 0x3F));
		c->PushChar(0x80 | (u & 0x3F));
	}
	else {
		assert(u <= 0x10FFFF);
		c->PushChar(0xF0 | ((u >> 18) & 0xFF));
		c->PushChar(0x80 | ((u >> 12) & 0x3F));
		c->PushChar(0x80 | ((u >> 6) & 0x3F));
		c->PushChar(0x80 | (u & 0x3F));
	}
}

static parseStatus ParseStringRaw(parserStack* c, char** str, size_t& len)
{
	assert(*c->json == '\"');
	c->json++;

	size_t head = c->top;

	const char* p = c->json;
	while (1) {
		char ch = *(p++);
		switch (ch) {
		case '\\':
			switch (*(p++)) {
			case 'n':  c->PushChar('\n'); break;
			case '\"': c->PushChar('\"'); break;
			case '\\': c->PushChar('\\'); break;
			case '/':  c->PushChar('/');  break;
			case 'b':  c->PushChar('\b'); break;
			case 'f':  c->PushChar('\f'); break;
			case 'r':  c->PushChar('\r'); break;
			case 't':  c->PushChar('\t'); break;
			case 'u':
			{
				unsigned H = 0, L = 0, codePoint = 0;
				if (!(p = ParseHex4(p, &H)))
					STRING_ERROR(PARSE_ERR_INVALID_UNICODE_HEX);
				if (H >= 0xD800 && H <= 0xDBFF) { /* surrogate pair */
					if (*p++ != '\\')
						STRING_ERROR(PARSE_ERR_INVALID_UNICODE_SURROGATE);
					if (*p++ != 'u')
						STRING_ERROR(PARSE_ERR_INVALID_UNICODE_SURROGATE);
					if (!(p = ParseHex4(p, &L)))
						STRING_ERROR(PARSE_ERR_INVALID_UNICODE_HEX);
					if (L < 0xDC00 || L > 0xDFFF)
						STRING_ERROR(PARSE_ERR_INVALID_UNICODE_SURROGATE);
					codePoint = (((H - 0xD800) << 10) | (L - 0xDC00)) + 0x10000;
					EncodeUtf8(c, codePoint);
				}
				else {
					
				}
				break;
			}
			default:
				STRING_ERROR(PARSE_ERR_INVALID_ESCAPE_CHAR);
			}
			break;
		case '\"':
			len = c->top - head;
			c->json = p;
			*str = (char*)c->Pop(len);
			return PARSE_OK;
		case '\0':
			STRING_ERROR(PARSE_ERR_MISS_QUOTATION_MARK);
		default:
			// if ch is control characters
			if ((unsigned char)ch < 0x20) {
				STRING_ERROR(PARSE_ERR_CONTROL_CHAR);
			}
			c->PushChar(ch);
		}
	}
}

static parseStatus ParseString(parserStack* c, jsonValue* v)
{
	parseStatus ret;

	char* s;
	size_t len;
	if ((ret = ParseStringRaw(c, &s, len)) == PARSE_OK) {
		SetValueString(v, s, len);
	}
	return ret;
}

static parseStatus ParseObject(parserStack* c, jsonValue* v)
{
	assert(*c->json == '{');
	c->json++;

	parseStatus ret = PARSE_OK;
	size_t size = 0;
	jsonMap m;
	m.key = nullptr;

	ParseWhitespace(c);
	if (*c->json == '}')
	{
		c->json++;
		v->type = TYPE_OBJECT;
		v->obj.maps = nullptr;
		v->obj.size = 0;
		return PARSE_OK;
	}

	while (1)
	{
		InitValue(&m.value);

		char* str;

		if (*c->json != '"')
		{
			ret = PARSE_ERR_MISS_KEY;
			break;
		}
		if ((ret = ParseStringRaw(c, &str, m.keyLen)) != PARSE_OK)
		{
			break;
		}
		void* ptr = realloc(m.key, m.keyLen + 1);
		assert(ptr);
		m.key = (char*)ptr;
		memcpy(m.key, str, m.keyLen);
		m.key[m.keyLen] = '\0';

		ParseWhitespace(c);
		if (*c->json != ':')
		{
			ret = PARSE_ERR_MISS_COLON;
			break;
		}

		c->json++;
		ParseWhitespace(c);

		if ((ret = ParseValue(c, &m.value)) != PARSE_OK)
		{
			break;
		}
		memcpy(c->PushSz(sizeof(jsonMap)), &m, sizeof(jsonMap));

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
			size_t copySz = sizeof(jsonMap) * size;
			c->json++;
			v->type = TYPE_OBJECT;
			v->obj.size = size;
			memcpy(v->obj.maps = (jsonMap*)malloc(copySz), c->Pop(copySz), copySz);
			return PARSE_OK;
		}
		else
		{
			ret = PARSE_ERR_MISS_COMMA_OR_CURLY_BRACKET;
			break;
		}
	}

	free(m.key);
	for (size_t i = 0; i < size; i++)
	{
		jsonMap* m = (jsonMap*)c->Pop(sizeof(jsonMap));
		free(m->key);
		FreeValue(&m->value);
	}
	v->type = TYPE_NULL;

	return ret;
}

static parseStatus ParseLiteral(parserStack* c, jsonValue* v, const char* literal, valueType type)
{
	assert(*c->json == literal[0]);
	c->json++;

	size_t i;
	for (i = 0; literal[i + 1] != '\0'; i++) {
		if (c->json[i] != literal[i + 1]) {
			return PARSE_ERR_INVALID_VALUE;
		}
	}
	c->json += i;
	v->type = type;
	return PARSE_OK;
}

static parseStatus ParseNumber(parserStack* c, jsonValue* v) {
	const char* p = c->json;

	bool isPositive = true;
	if (*p == '-') {
		isPositive = false;
		p++;
	}

	if (!ISDIGIT(*p)) {
		return PARSE_ERR_INVALID_VALUE;
	}

	if (*p == '0') {
		p++;
	}
	else {
		if (!ISDIGIT(*p)) {
			return PARSE_ERR_INVALID_VALUE;
		}
		for (p++; ISDIGIT(*p); p++) {
		}
	}

	if (*p == '.') {
		p++;
		if (!ISDIGIT(*p)) {
			return PARSE_ERR_INVALID_VALUE;
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
			return PARSE_ERR_INVALID_VALUE;
		}
		for (p++; ISDIGIT(*p); p++) {
		}
	}

	errno = 0;
	v->num = strtod(c->json, NULL);
	if (errno == ERANGE && (v->num == HUGE_VAL || v->num == -HUGE_VAL)) {
		return PARSE_ERR_NUMBER_OVERFLOW;
	}
	c->json = p;
	v->type = TYPE_NUMBER;
	return PARSE_OK;
}

static parseStatus ParseArray(parserStack* c, jsonValue* v)
{
	assert(*c->json == '[');
	c->json++;

	parseStatus ret;

	size_t size = 0;

	ParseWhitespace(c);
	if (*c->json == ']')
	{
		c->json++;
		v->type = TYPE_ARRAY;
		v->arr.size = 0;
		v->arr.values = nullptr;
		return PARSE_OK;
	}
	while (1)
	{
		jsonValue e;
		InitValue(&e);
		ParseWhitespace(c);
		if ((ret = ParseValue(c, &e)) != PARSE_OK)
		{
			break;
		}
		memcpy(c->PushSz(sizeof(jsonValue)), &e, sizeof(jsonValue));
		size++;

		ParseWhitespace(c);

		if (*c->json == ',')
		{
			c->json++;
		}
		else if (*c->json == ']')
		{
			c->json++;
			v->type = TYPE_ARRAY;
			v->arr.size = size;
			v->arr.values = (jsonValue*)malloc(size * sizeof(jsonValue));
			memcpy(v->arr.values, c->Pop(size * sizeof(jsonValue)), size * sizeof(jsonValue));
			return PARSE_OK;
		}
		else
		{
			ret = PARSE_ERR_MISS_COMMA_OR_SQUARE_BRACKET;
			break;
		}
	}

	for (size_t i = 0; i < size; i++) {
		FreeValue((jsonValue*)c->Pop(sizeof(jsonValue)));
	}

	return ret;
}

static parseStatus ParseValue(parserStack* c, jsonValue* v) {
	switch (*c->json) {
	case 't':  return ParseLiteral(c, v, "true", TYPE_TRUE);
	case 'f':  return ParseLiteral(c, v, "false", TYPE_FALSE);
	case 'n':  return ParseLiteral(c, v, "null", TYPE_NULL);
	case '\0': return PARSE_ERR_EXPECT_VALUE;
	case '"':  return ParseString(c, v);
	case '[':  return ParseArray(c, v);
	case '{':  return ParseObject(c, v);
	default:   return ParseNumber(c, v);
	}
}

parseStatus ParseJsonString(jsonValue* v, const char* json) {
	assert(v != NULL);

	parserStack c;
	c.json = json;
	c.stack = nullptr;
	c.size = c.top = 0;

	InitValue(v);

	parseStatus ret;

	ParseWhitespace(&c);
	if ((ret = ParseValue(&c, v)) == PARSE_OK) {
		ParseWhitespace(&c);
		if (*c.json != '\0') {
			v->type = TYPE_NULL;
			ret = PARSE_ERR_ROOT_NOT_SINGULAR;
		}
	}
	assert(!c.top);
	return ret;
}

valueType GetValueType(const jsonValue* v) {
	assert(v != NULL);
	return v->type;
}

double GetValueNumber(const jsonValue* v) {
	assert(v && v->type == TYPE_NUMBER);
	return v->num;
}
void SetValueNumber(jsonValue* v, double n) {
	FreeValue(v);
	v->num = n;
	v->type = TYPE_NUMBER;
}

const char* GetValueString(const jsonValue* v)
{
	assert(v && v->type == TYPE_STRING);
	return v->str.s;
}

size_t GetValueStringLength(const jsonValue* v)
{
	assert(v && v->type == TYPE_STRING);
	return v->str.len;
}

void SetValueBoolean(jsonValue* v, bool b)
{
	FreeValue(v);
	v->type = b ? TYPE_TRUE : TYPE_FALSE;
}

bool GetValueBoolean(const jsonValue* v)
{
	assert(v && (v->type == TYPE_TRUE || v->type == TYPE_FALSE));
	return v->type == TYPE_TRUE;
}

size_t GetValueArraySize(const jsonValue* v)
{
	assert(v && v->type == TYPE_ARRAY);
	return v->arr.size;
}

jsonValue* GetValueArrayElement(const jsonValue* v, size_t index)
{
	assert(v && v->type == TYPE_ARRAY && index < v->arr.size);
	return &v->arr.values[index];
}

size_t GetValueObjectSize(const jsonValue* v)
{
	assert(v && v->type == TYPE_OBJECT);
	return v->obj.size;
}

const char* GetValueObjectKey(const jsonValue* v, size_t index)
{
	assert(v && v->type == TYPE_OBJECT);
	assert(index < v->obj.size);
	return v->obj.maps[index].key;
}

size_t GetValueObjectKeyLength(const jsonValue* v, size_t index)
{
	assert(v && v->type == TYPE_OBJECT);
	assert(index < v->obj.size);
	return v->obj.maps[index].keyLen;
}

jsonValue* GetValueObjectValue(const jsonValue* v, size_t index)
{
	assert(v && v->type == TYPE_OBJECT);
	assert(index < v->obj.size);
	return &v->obj.maps[index].value;
}