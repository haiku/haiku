/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include <errno.h>
#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <OS.h>
#include <KernelExport.h>
#include "google_request.h"
#include "string_utils.h"

#define TESTME

#ifdef _KERNEL_MODE
#define printf dprintf
#undef TESTME
#endif

#define DBG "googlefs: parse_html: "

#ifdef TESTME
#define BUFSZ (128*1024)
int dbgstep = 0;
#define PRST printf(DBG "step %d\n", dbgstep++)
#else
#define PRST {}
#endif

//old
//#define G_BEGIN_URL "<p class=g><a class=l href=\""
//#define G_BEGIN_URL "<div class=g><a class=l href=\""
//#define G_BEGIN_URL "<div class=g><a href=\""
#define G_BEGIN_URL "<h3 class=\"r\"><a href=\""
//#define G_END_URL "\">"
#define G_END_URL "\">"
//#define G_BEGIN_NAME 
#define G_END_NAME "</a>"
#define G_BEGIN_SNIPSET /*"<td class=j>"*/"<font size=-1>"
#define G_END_SNIPSET "<br>"
#define G_BEGIN_CACHESIM " <a class=fl href=\""
#define G_END_CACHESIM "\">"
#define G_URL_PREFIX "http://www.google.com"

int google_parse_results(const char *html, size_t htmlsize, long *nextid, struct google_result **results)
{
	struct google_result *res = NULL, *nres = NULL, *prev = NULL;
	char *p, *q;
	char *nextresult = NULL;
	long numres = 0;
	long maxres = 1000;
	//long startid = 0;
	int done = 0;
	int err = ENOMEM;

	if (!html || !results)
		return EINVAL;
	/* sanity checks */
	printf(DBG"sanity check...\n");
	PRST;
	/* google now sends <!doctype html><head> sometimes... */
	if (strstr(html, "<!doctype html><head>") != html) {
		if (strstr(html, "<html><head>") != html) {
			if (strstr(html, "<!doctype html><html ") != html)
				return EINVAL;
		}
	}
	PRST;
//	p = strstr(html, "<title>Google Search:");
	p = strstr(html, "Google");
	if (!p) return EINVAL;
	PRST;
	p = strstr(html, "<body");
	if (!p) return EINVAL;
	PRST;
	
	/*
	p = strstr(html, "Search Results<");
	if (!p) return EINVAL;
	PRST;
	*/

	
	printf(DBG"parsing...\n");
	do {
		char *item;
		unsigned long itemlen;
		char *tmp;
		char *urlp;
		int i;
#ifdef TESTME
		dbgstep = 0;
#endif
		nres = malloc(sizeof(struct google_result));
		if (!nres) {
			// XXX: cleanup!
			goto err0;
		}
		memset(nres, 0, sizeof(struct google_result));
		nres->id = (*nextid)++; //- 1;

		PRST;
		/* find url */
		// <p class=g><a href=URL>
		if (!p) break;
		if (nextresult)
			p = nextresult;
		else
			p = strstr(p, G_BEGIN_URL);
		if (!p) break;
		PRST;
		p+= strlen(G_BEGIN_URL);
		nextresult = strstr(p, G_BEGIN_URL);
		//printf(DBG"[%ld] found token 1\n", numres);
		item = p;
		p = strstr(p, G_END_URL);
		if (!p) break;
		PRST;
		p+= strlen(G_END_URL);
		//printf(DBG"[%ld] found token 2\n", numres);
		itemlen = GR_MAX_URL-1;
		urlp = nres->url;
		if (!strncmp(item, "/url?", 5)) {
			strcpy(urlp, G_URL_PREFIX);
			itemlen -= strlen(G_URL_PREFIX);
			urlp += strlen(G_URL_PREFIX);
			printf("plop\n");
		}
		itemlen = MIN(itemlen, p - item - strlen(G_END_URL));
		strncpy(urlp, item, itemlen);
		urlp[itemlen] = '\0';
		
		/* find name */
		//<b>Google</b> Web APIs - FAQ</a><table
		item = p;
		p = strstr(p, G_END_NAME);
		if (!p) break;
		PRST;
		p+= strlen(G_END_NAME);
		//printf(DBG"[%ld] found token 3\n", numres);
		itemlen = p - item - strlen(G_END_NAME);
		//itemlen = MIN(GR_MAX_NAME-1, itemlen);
		itemlen = MIN(GR_MAX_NAME*4-1, itemlen);
		q = malloc(itemlen+1);
		if (!q)
			goto err0;
		strncpy(q, item, itemlen);
		q[itemlen] = '\0';
		/* strip <*b> off */
		PRST;
		while ((tmp = strstr(q, "<b>")))
			strcpy(tmp, tmp + 3);
		while ((tmp = strstr(q, "</b>")))
			strcpy(tmp, tmp + 4);
		/* strip <*em> off */
		PRST;
		while ((tmp = strstr(q, "<em>")))
			strcpy(tmp, tmp + 4);
		while ((tmp = strstr(q, "</em>")))
			strcpy(tmp, tmp + 5);
		/* strip &foo; */
		tmp = unentitify_string(q);
		free(q);
		if (!tmp)
			goto err0;
		strncpy(nres->name, tmp, GR_MAX_NAME-1);
		nres->name[GR_MAX_NAME-1] = '\0';
		free(tmp);
		PRST;
		
#if 0
		/* find snipset */
		//<td class=j><font size=-1><b>...</b> a custom Java client library, documentation on <b>how</b> <b>to</b> use the <b>...</b> You can find it at http://<b>api</b>.<b>google</b>.com/GoogleSearch.wsdl <b>...</b> need to get started is in <b>googleapi</b>.jar <b>...</b> <br>
		if (!p) break;
		q = strstr(p, G_BEGIN_SNIPSET);
		if (q && (!nextresult || (q < nextresult))) {
			p = q;
			p+= strlen(G_BEGIN_SNIPSET);
			//printf(DBG"[%ld] found token 4\n", numres);
			item = p;
			p = strstr(p, G_END_SNIPSET);
			if (!p) break;
			p+= strlen(G_END_SNIPSET);
			//printf(DBG"[%ld] found token 5\n", numres);
			itemlen = p - item - strlen(G_END_SNIPSET);
			itemlen = MIN(GR_MAX_URL-1, itemlen);
			strncpy(nres->snipset, item, itemlen);
			nres->snipset[itemlen] = '\0';
			/* strip &foo; */
			tmp = unentitify_string(nres->snipset);
			if (!tmp)
				break;
			strncpy(nres->snipset, tmp, GR_MAX_SNIPSET-1);
			nres->snipset[GR_MAX_SNIPSET-1] = '\0';
			free(tmp);
			/* strip <*b> off */
			while ((tmp = strstr(nres->snipset, "<b>")))
				strcpy(tmp, tmp + 3);
			while ((tmp = strstr(nres->snipset, "</b>")))
				strcpy(tmp, tmp + 4);
			while ((tmp = strstr(nres->snipset, "\r")))
				strcpy(tmp, tmp + 1);
			while ((tmp = strstr(nres->snipset, "\n")))
				*tmp = ' ';
		}

#endif
		/* find cache/similar url */
		//  <a class=fl href="http://216.239.59.104/search?q=cache:vR7BaPWutnkJ:www.google.com/apis/api_faq.html+google+api++help+%22frequently+asked%22+-plop&hl=en&lr=lang_en&ie=UTF-8">Cached</a> 
		for (i = 0; i < 2; i++) {
			if (!p) break;
			q = strstr(p, G_BEGIN_CACHESIM);
			if (q && nextresult && (q > nextresult)) {
				p = q;
				printf(DBG"[%ld] cache/sim beyond next\n", numres);
				p = nextresult; /* reset */
			} else if (q && (!nextresult || (q < nextresult))) {
				//int iscache;
				p = q;
				p+= strlen(G_BEGIN_CACHESIM);
				//printf(DBG"[%ld] found token 6\n", numres);
				item = p;
				p = strstr(p, G_END_CACHESIM);
				if (!p) break;
				p+= strlen(G_END_CACHESIM);
				//printf(DBG"[%ld] found token 7\n", numres);
				itemlen = p - item - strlen(G_END_CACHESIM);
				itemlen = MIN(GR_MAX_URL-1, itemlen);
				if (!strncmp(p, "Cached", 6)) {
					strncpy(nres->cache_url, item, itemlen);
					nres->cache_url[itemlen] = '\0';
				} else if (!strncmp(p, "Similar", 7)) {
					strncpy(nres->similar_url, item, itemlen);
					nres->similar_url[itemlen] = '\0';
				}
//				 else
//					break;
			}
		}
		
		numres++;
		if (!prev)
			res = nres;
		else
			prev->next = nres;
		prev = nres;
		nres = NULL;
	} while (!done || numres < maxres);
	*results = res;
	return numres;
err0:
	free(nres);
	while (res) {
		nres = res->next;
		free(res);
		res = nres;
	}
	return err;
}

#ifdef TESTME
int main(int argc, char **argv)
{
	struct google_result *results;
	struct google_result *tag1 = 0xaaaa5555, *res = NULL, *tag2 = 0x5555aaaa;
	size_t len;
	char *p;
	int err;
	long nextid = 0;
	
	p = malloc(BUFSZ+8);
	len = read(0, p+4, BUFSZ);
	p[BUFSZ+4-1] = '\0';
	*(uint32 *)p = 0xa5a5a5a5;
	*(uint32 *)(&p[BUFSZ+4]) = 0x5a5a5a5a;
	err = google_parse_results(p+4, len, &nextid, &results);
	printf("error 0x%08lx\n", err);
	if (err < 0)
		return 1;
	res = results;
	while (res) {
		printf("[%ld]:\nURL='%s'\nNAME='%s'\nSNIPSET='%s'\nCACHE='%s'\nSIMILAR='%s'\n\n", res->id, res->url, res->name, res->snipset, res->cache_url, res->similar_url);
		res = res->next;
	}
	printf("before = 0x%08lx:0x%08lx, after = 0x%08lx:0x%08lx\n", 0xa5a5a5a5, *(uint32 *)p, 0x5a5a5a5a, *(uint32 *)(&p[BUFSZ+4]));
	printf("before = 0x%08lx:0x%08lx, after = 0x%08lx:0x%08lx\n", 0xaaaa5555, tag1, 0x5555aaaa, tag2);
	return 0;
}
#endif
