/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include "string_utils.h"

//#define TESTME

#ifdef _KERNEL_MODE
#define printf dprintf
#undef TESTME
#endif



char *urlify_string(const char *str)
{
	char *dst, *d;
	const char *p;
	const char *allowed = "abcdefghijklmnopqrstuvwxyz" \
						  "ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
						  "0123456789" \
						  "-_.!~*'()"; /* cf. RFC 2396 */
	const char *hex = "0123456789ABCDEF";
	if (!str)
		return NULL;
	// hacky, but safe
	dst = malloc(strlen(str)*3);
	if (!dst)
		return NULL;
	for (p = str, d = dst; *p; p++) {
		if (strchr(allowed, *p))
			*d++ = *p;
		else if (*p == ' ') {
			*d++ = '+';
		} else {
			/* use hex value */
			*d++ = '%';
			*d++ = hex[(*(unsigned char *)p >> 4) & 0x0F];
			*d++ = hex[(*(unsigned char *)p) & 0x0F];
		}
	}
	*d = '\0';
	return dst;
}

// cf. http://www.htmlhelp.com/reference/html40/entities/

static const char *entities_tab[][2] = { 
{ "lt", "<" },
{ "gt", ">" },
{ "amp", "&" },
{ "nbsp", " " },
{ "quot", "\"" },
{ "raquo", "»" },
//{ "laquo", "" },
{ "ccedil", "ç" },
// grave
{ "agrave", "à" },
{ "egrave", "è" },
// acute
//{ "aacute", "" },
{ "eacute", "é" },
// circ
{ "acirc", "â" },
{ "ecirc", "ê" },
{ "icirc", "î" },
{ "ocirc", "ô" },
{ "ucirc", "û" },
{ "copy", "©" },
{ "trade", "™" },
//{ "", "" },
{ NULL, NULL },
};

char *unentitify_string(const char *str)
{
	char *dst, *d;
	const char *p;
	const char *hex = "0123456789abcdef";
	int i;
	if (!str)
		return NULL;
	// hacky, but safe
	dst = malloc(strlen(str)+2);
	if (!dst)
		return NULL;
	for (p = str, d = dst; *p; p++) {
		if (*p != '&')
			*d++ = *p;
		/* those case convert to binary, but won't check for valid multibyte UTF-8 sequences */
		else if ((p[1] == '#') && p[2] && p[3] && (p[4] == ';') && 
				isdigit(p[2]) && 
				isdigit(p[3])) {
			/* &#nn; */
			char c = ((p[2]) - '0') * 10 +
					 ((p[3]) - '0');
			*d++ = c;
			p += 4;
		} else if ((p[1] == '#') && p[2] && p[3] && p[4] && (p[5] == ';') && 
				isdigit(p[2]) && 
				isdigit(p[3]) && 
				isdigit(p[4])) {
			/* &#nnn; */
			char c = ((p[2]) - '0') * 100 + 
					 ((p[3]) - '0') * 10 +
					 ((p[4]) - '0');
			*d++ = c;
			p += 5;
		} else if ((p[1] == '#') && (p[2] == 'x') && p[3] && p[4] && (p[5] == ';') && 
				strchr(hex, tolower(p[3])) && 
				strchr(hex, tolower(p[4]))) {
			/* &#xnn; */
			char c = (strchr(hex, tolower(p[3])) - hex) << 4 |
					 (strchr(hex, tolower(p[4])) - hex);
			*d++ = c;
			p += 5;
		} else {
			char buf[20];
			strncpy(buf, p+1, 20);
			buf[19] = '\0';
			if (!strchr(buf, ';')) {
				*d++ = *p;
				continue;
			}
			*(strchr(buf, ';')) = '\0';
			for (i = 0; entities_tab[i][0]; i++) {
				if (!strcmp(buf, entities_tab[i][0])) {
					strcpy(d, entities_tab[i][1]);
					d += strlen(d);
					p += strlen(entities_tab[i][0]) + 1;
					break;
				}
			}
			if (!entities_tab[i][0]) /* not found */
				*d++ = '&';
		}
	}
	*d = '\0';
	return dst;
}

#ifdef TESTME
int main(int argc, char **argv)
{
	char *p;
	if (argc < 2)
		return 1;
	p = unentitify_string(argv[1]);
	printf("'%s'\n", p);
	free(p);
	free(malloc(10));
	return 0;
}
#endif
