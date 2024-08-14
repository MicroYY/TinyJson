#include "tinyjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL */
#include <errno.h>
#include <cmath>

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
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

static int parse_number(lept_context* c, lept_value* v) {
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
        if (*p != '\0' && *p != '.' && *p != 'e' && *p != 'E') {
            return LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
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

    errno = 0;
    char* end;
    v->n = strtod(c->json, &end);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL)) {
        return LEPT_PARSE_NUMBER_TOO_BIG;
    }
    if (c->json == end) {
        return LEPT_PARSE_INVALID_VALUE;
    }
    c->json = end;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return ParseLiteral(c, v, "true", LEPT_TRUE);
        case 'f':  return ParseLiteral(c, v, "false", LEPT_FALSE);
        case 'n':  return ParseLiteral(c, v, "null", LEPT_NULL);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return parse_number(c, v);
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    int ret;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            ret =  LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}