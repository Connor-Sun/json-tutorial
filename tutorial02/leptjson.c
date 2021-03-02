#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, strtod() */

#define EXPECT(c, ch)   do { assert(*c->json == (ch)); c->json++; } while(0)

#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

/*
    static int lept_parse_true(lept_context* c, lept_value* v) {
        EXPECT(c, 't');
        if (c->json[0] != 'r' || c->json[1] != 'u' || c->json[2] != 'e')
            return LEPT_PARSE_INVALID_VALUE;
        c->json += 3;
        v->type = LEPT_TRUE;
        return LEPT_PARSE_OK;
    }

    static int lept_parse_false(lept_context* c, lept_value* v) {
        EXPECT(c, 'f');
        if (c->json[0] != 'a' || c->json[1] != 'l' || c->json[2] != 's' || c->json[3] != 'e')
            return LEPT_PARSE_INVALID_VALUE;
        c->json += 4;
        v->type = LEPT_FALSE;
        return LEPT_PARSE_OK;
    }

    static int lept_parse_null(lept_context* c, lept_value* v) {
        EXPECT(c, 'n');
        if (c->json[0] != 'u' || c->json[1] != 'l' || c->json[2] != 'l')
            return LEPT_PARSE_INVALID_VALUE;
        c->json += 3;
        v->type = LEPT_NULL;
        return LEPT_PARSE_OK;
    }
*/

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    EXPECT(c, literal[0]);
    size_t i;
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    /*
    *   自己写的部分
    *   看似更好理解，但只加一次i可以节约计算量
        for (i = 1; literal[i]; ++i) {
        if (c->json != literal[i])
            return LEPT_PARSE_INVALID_VALUE;
        c->json++;
        }
    */
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    char* start = c->json;
    /* \TODO validate number */
    /* 按顺序 ["-"] int [frac] [exp] */

    /* 负号部分 */
    if (*start == '-')
        start++;
    /* 整数部分 */
    if (*start == '0')
        start++;
    else {
        if (!ISDIGIT1TO9(*start))
            return LEPT_PARSE_INVALID_VALUE;
        while (ISDIGIT(*(++start)));
    }
    /* 小数部分 */
    if (*start == '.') {
        start++;
        if (!ISDIGIT(*start))
            return LEPT_PARSE_INVALID_VALUE;
        start++;
        while (ISDIGIT(*start))
            start++;
        // while (ISDIGIT(*(++start)));
    }    
    /* 指数部分 */
    if (*start == 'e' || *start == 'E') {
        start++;
        if (*start == '+' || *start == '-')
            start++;
        if (!ISDIGIT(*start))
            return LEPT_PARSE_INVALID_VALUE;
        start++;
        while (ISDIGIT(*start))
            start++;
        // while (ISDIGIT(*(++start)));
        // 为什么会有不一样
    }

    errno = 0;
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    c->json = start;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
