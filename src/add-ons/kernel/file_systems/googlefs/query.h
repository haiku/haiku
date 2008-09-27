/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _K_QUERY_H
#define _K_QUERY_H

#include <OS.h>

#ifndef _QUERY_H
/* already defined in Query.h */
typedef enum {
	B_INVALID_OP = 0,
	B_EQ,
	B_GT,
	B_GE,
	B_LT,
	B_LE,
	B_NE,
	B_CONTAINS,
	B_BEGINS_WITH,
	B_ENDS_WITH,
	B_AND = 0x101,
	B_OR,
	B_NOT,
	_B_RESERVED_OP_ = 0x100000
} query_op;
#endif

struct query_exp;

struct query_term {
	struct query_exp *exp;
	char *str;
/*	uint32 type;
	union {
		int32 int32v;
		uint32 uint32v;
		int64 int64v;
		uint64 uint64v;
		bigtime_t bigtimev;
		char *strv;
	} val;
*/
};

typedef struct query_exp {
	query_op op;
	struct query_term lv;
	struct query_term rv;
} query_exp;

/* this one dups the string */
extern char *query_unescape_string(const char *q, const char **newq, char delim);

/* this oen is in-place */
extern char *query_strip_bracketed_Cc(char *str);

extern status_t query_parse(const char *query, query_exp **tree);

#endif /* _K_QUERY_H */
