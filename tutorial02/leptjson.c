#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <stdlib.h>  /* NULL, strtod() */
#include <math.h>    /* HUGE_VAL*/
#include <errno.h> 
#include <stdio.h>
#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c,lept_value* v, lept_type m){
    int i,len;
    const char* str;
    assert(m == LEPT_TRUE || m == LEPT_FALSE || m == LEPT_NULL);
    switch(m){
        case LEPT_TRUE:     len=3; str="true";  break;
        case LEPT_FALSE:    len=4; str="false"; break;
        case LEPT_NULL:     len=3; str="null";  break;
    }
    EXPECT(c,str[0]);
    for(i=0;i<len;i++)
        if(c->json[i] != str[i+1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += len;
    v->type = m;
    return LEPT_PARSE_OK;
}

#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

static int lept_parse_number(lept_context* c, lept_value* v) {
	char* end;
	const char* cur = c->json;
	if (cur[0] == 'N' && cur[1] == 'A' && cur[2] == 'N'
		|| cur[0] == 'n' && cur[1] == 'a' && cur[2] == 'n'
		|| cur[0] == 'I' && cur[1] == 'N' && cur[2] == 'F'
		|| cur[0] == 'i' && cur[1] == 'n' && cur[2] == 'f')
		return LEPT_PARSE_INVALID_VALUE;
	if (*cur == '+')
		return LEPT_PARSE_INVALID_VALUE;
	if (*cur == '-')	
		cur++;

	if (*cur == '.')	
		return LEPT_PARSE_INVALID_VALUE;
	if (*cur == '0') {
		cur++;
		if (ISDIGIT(*cur) || *cur == 'x' || *cur == 'X' )
			return LEPT_PARSE_ROOT_NOT_SINGULAR;
	}
	else if (ISDIGIT1TO9(*cur)) {
		for (cur++; ISDIGIT(*cur); cur++);
	}

	if (*cur == '.') {
		cur++;
		if (!ISDIGIT(*cur))
			return LEPT_PARSE_INVALID_VALUE;
		else
			while (ISDIGIT(*++cur));
	}

	if (*cur == 'e' || *cur == 'E') {
		cur++;
		if (*cur == '+' || *cur == '-')
			cur++;
		if (!ISDIGIT(*cur))
			return LEPT_PARSE_INVALID_VALUE;
		else
			for (cur++; ISDIGIT(*cur); cur++);
	}
	
	errno = 0;
    v->n = strtod(c->json, &end);
	if (c->json == end)	return LEPT_PARSE_INVALID_VALUE;
	if ((v->n == HUGE_VAL ||v->n == -HUGE_VAL) && errno == ERANGE)
		return LEPT_PARSE_NUMBER_TOO_BIG; 
    c->json = cur;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v,LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v,LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v,LEPT_NULL);
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
