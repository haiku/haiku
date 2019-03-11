/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include "query.h"

//#define TESTME

#ifdef _KERNEL_MODE
#define printf dprintf
#undef TESTME
#endif

#ifdef TESTME
#include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
// ('foo'<>"bar\"")&&!(()||())

static void free_query_tree(query_exp *tree)
{
	if (!tree)
		return;
	if (tree->op >= B_AND) {
		free_query_tree(tree->lv.exp);
		free_query_tree(tree->rv.exp);
	}
	free(tree->lv.str);
	free(tree->rv.str);
	free(tree);
}

char *query_unescape_string(const char *q, const char **newq, char delim)
{
	int backslash = 0;
	int i;
	char *p, *p2;

	if (*q == '*')
		q++;

	p = malloc(10);
	if (!p)
		return NULL;
	for (i = 0; *q; i++) {
		if ((i % 10) == 9) {
			p2 = realloc(p, i+10);
			if (!p2) {
				free(p);
				return NULL;//p;
			}
			p = p2;
		}
		if (backslash) {
			backslash = 0;
			p[i] = *q;
		} else if (*q == '\\') {
			backslash = 1;
			i--;
		} else if (*q == '\0') {
			break;
		} else if (*q == delim) {
			break;
		} else {
			p[i] = *q;
			if (p[i] == '*')
				p[i] = ' ';
		}
		q++;
	}
	p[i] = '\0';
	if (i > 0 && p[i-1] == ' ')
		p[i-1] = '\0';
	if (newq)
		*newq = q;
	if (i)
		return p;
	free(p);
	return NULL;
}

char *query_strip_bracketed_Cc(char *str)
{
	char *p = str;
	char c, C;
	while (*p) {
		if (*p == '[') {
			if (p[1] && p[2] && p[3] == ']') {
				c = p[1];
				C = p[2];
				//printf("cC = %c%c\n", c, C);
				if (C >= 'a' && C <= 'z' && (c == C + 'A' - 'a'))
					*p = C;
				else if (c >= 'a' && c <= 'z' && (C == c + 'A' - 'a'))
					*p = c;
				if (*p != '[') {
					strcpy(p+1, p + 4);
				}
			}
		}
		p++;
	}
	return str;
}

enum pqs_state {
	QS_UNKNOWN,
	QS_PAREN,
	QS_QTR,
	QS_
};

/*
static const char *parse_qs_r(const char *query, query_exp *tree)
{
	int parens = 0;
	return NULL;
}
*/

status_t query_parse(const char *query, query_exp **tree)
{
	//query_exp *t;
	
	return B_OK;
}

#ifdef TESTME

const char *strqop(query_op op)
{
	switch (op) {
#define QOP(_op) case _op: return #_op
	QOP(B_INVALID_OP);
	QOP(B_EQ);
	QOP(B_GT);
	QOP(B_GE);
	QOP(B_LT);
	QOP(B_LE);
	QOP(B_NE);
	QOP(B_CONTAINS);
	QOP(B_BEGINS_WITH);
	QOP(B_ENDS_WITH);
	QOP(B_AND);
	QOP(B_OR);
	QOP(B_NOT);
#undef QOP
	default: return "B_?_OP";
	}
}

#define INDC '#'

void dump_query_tree(query_exp *tree, int indent)
{
	int i;
	if (!tree)
		return;
	if (tree->op >= B_AND) {
		for (i=0;i<indent;i++) printf("%c", INDC);
		printf(": %s {\n", strqop(tree->op));
		dump_query_tree(tree->lv.exp, indent+1);
		dump_query_tree(tree->rv.exp, indent+1);
		for (i=0;i<indent;i++) printf("%c", INDC);
		printf("}\n");
	} else {
		for (i=0;i<indent;i++) printf("%c", INDC);
		printf(": {%s} %s {%s}\n", tree->lv.str, strqop(tree->op), tree->rv.str);
	}
}

int main(int argc, char **argv)
{
	status_t err;
	query_exp *tree;
	char *p;
	if (argc < 2)
		return 1;
/*
	err = query_parse(argv[1], &tree);
	if (err) {
		printf("parse_query_string: %s\n", strerror(err));
		return 1;
	}
	dump_query_tree(tree, 0);
*/
	if (!strncmp(argv[1], "((name==\"*", 10)) {
		argv[1] += 10;
	}
	p = query_unescape_string(argv[1], NULL, '"');
	printf("'%s'\n", p);
	query_strip_bracketed_Cc(p);
	printf("'%s'\n", p);
	return 0;
}
#endif
