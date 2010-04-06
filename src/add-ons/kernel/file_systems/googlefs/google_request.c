/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */

#include <OS.h>
#include <KernelExport.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <malloc.h>
#include <sys/socket.h>
#include "http_cnx.h"
#include "googlefs.h"
#include "google_request.h"
#include "lists2.h"
#include "settings.h"
#include "string_utils.h"

#define DO_PUBLISH
//#define FAKE_INPUT "/boot/home/devel/drivers/googlefs/log2.html"

//#define TESTURL "http://www.google.com/search?as_q=google+api+&num=50&hl=en&ie=ISO-8859-1&btnG=Google+Search&as_epq=frequently+asked&as_oq=help&as_eq=plop&lr=lang_en&as_ft=i&as_filetype=&as_qdr=m3&as_nlo=&as_nhi=&as_occt=any&as_dt=i&as_sitesearch="

//#define BASEURL "http://www.google.com/search?as_q="
//#define Q "google+api+"
//#define EXTRAURL "&num=50&hl=en&ie=ISO-8859-1&btnG=Google+Search&as_epq=frequently+asked&as_oq=help&as_eq=plop&lr=lang_en&as_ft=i&as_filetype=&as_qdr=m3&as_nlo=&as_nhi=&as_occt=any&as_dt=i&as_sitesearch="

#define TESTURL "http://www.google.com/search?hl=en&ie=UTF-8&num=50&q=beos"
//#define BASEURL "http://www.google.com/search?hl=en&ie=UTF-8&num=50&q="
#define BASEURL "/search?hl=en&ie=UTF-8&oe=UTF-8"
#define FMT_NUM "&num=%u"
#define FMT_Q "&q=%s"

/* parse_google_html.c */
extern int google_parse_results(const char *html, size_t htmlsize, struct google_result **results);

// move that to ksocket inlined
static int kinet_aton(const char *in, struct in_addr *addr)
{
	int i;
	unsigned long a;
	uint32 inaddr = 0L;
	const char *p = in;
	for (i = 0; i < 4; i++) {
		a = strtoul(p, (char **)&p, 10);
		if (!p)
			return -1;
		inaddr = (inaddr >> 8) | ((a & 0x0ff) << 24);
		*(uint32 *)addr = inaddr;
		if (!*p)
			return 0;
		p++;
	}
	return 0;
}


status_t google_request_init(void)
{
	status_t err;
	err = http_init();
	return err;
}

status_t google_request_uninit(void)
{
	status_t err;
	err = http_uninit();
	return err;
}

status_t google_request_process(struct google_request *req)
{
	struct sockaddr_in sin;
	struct http_cnx *cnx = NULL;
	struct google_result *res;
	status_t err;
	int count;
	char *p = NULL;
	char *url = NULL;
	
	err = http_create(&cnx);
	if (err)
		return err;
	err = ENOMEM;
	req->cnx = cnx;
#ifndef FAKE_INPUT
	sin.sin_len = sizeof(struct sockaddr_in);
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (kinet_aton(google_server, &sin.sin_addr) < 0)
		goto err_cnx;
	sin.sin_port = htons(google_server_port);
	err = http_connect(cnx, &sin);
	dprintf("google_request: http_connect: error 0x%08lx\n", err);
	if (err)
		goto err_cnx;
	
	err = ENOMEM;
	p = urlify_string(req->query_string);
	if (!p)
		goto err_con;
	
	err = ENOMEM;
	url = malloc(strlen(BASEURL)+strlen(FMT_NUM)+10+strlen(FMT_Q)+strlen(p)+2);
	if (!url)
		goto err_url;
	strcpy(url, BASEURL);
	sprintf(url+strlen(url), FMT_NUM, (unsigned int)max_results);
	sprintf(url+strlen(url), FMT_Q, p);
	
	dprintf("google_request: final URL: %s\n", url);
	
	err = http_get(cnx, url);
	dprintf("google_request: http_get: error 0x%08lx\n", err);
	if (err < 0)
		goto err_url2;
	dprintf("google_request: http_get: HEADERS %ld:%s\n", cnx->headerslen, cnx->headers);
	//dprintf("DATA: %d:%s\n", cnx->datalen, cnx->data);
	
	dprintf("google_request: buffer @ %p, len %ld\n", cnx->data, cnx->datalen);
	{
		int fd;
		// debug output
		fd = open("/tmp/google.html", O_CREAT|O_TRUNC|O_RDWR, 0644);
		write(fd, cnx->data, cnx->datalen);
		close(fd);
	}
#else
	{
		int fd;
		struct stat st;
		// debug output
		fd = open(FAKE_INPUT, O_RDONLY, 0644);
		if (fd < 0)
			return -1;
		if (fstat(fd, &st) < 0) {
			close(fd);
			return -1;
		}
		cnx->datalen = st.st_size;
		cnx->data = malloc(cnx->datalen);
		if (!cnx->data)
			return ENOMEM;
		if (read(fd, cnx->data, cnx->datalen) < cnx->datalen)
			return -1;
		close(fd);
	}
#endif /* FAKE_INPUT */	
	err = count = google_parse_results(req->cnx->data, req->cnx->datalen, &req->results);
	if (err < 0)
		goto err_get;
#ifdef DO_PUBLISH
	while ((res = SLL_DEQUEUE(req->results, next))) {
		res->next = NULL;
		googlefs_push_result_to_query(req, res);
	}
#endif
	free(url);
	free(p);
	/* close now */
	http_close(cnx);
	
	return B_OK;


err_get:
err_url2:
	free(url);
err_url:
	free(p);
err_con:
	http_close(cnx);
err_cnx:
	http_delete(cnx);
	req->cnx = NULL;
	return err;
}

status_t google_request_process_async(struct google_request *req)
{
	return ENOSYS;
}

status_t google_request_close(struct google_request *req)
{
	if (!req)
		return EINVAL;
	if (!req->cnx)
		return B_OK;
	http_close(req->cnx);
	http_delete(req->cnx);
	req->cnx = NULL;
	return B_OK;
}

status_t google_request_open(const char *query_string, struct fs_volume *volume, struct fs_node *query_node, struct google_request **req)
{
	struct google_request *r;
	if (!req)
		return EINVAL;
	r = malloc(sizeof(struct google_request));
	if (!r)
		return ENOMEM;
	memset(r, 0, sizeof(struct google_request));
	r->query_string = strdup(query_string);
	r->volume = volume;
	r->query_node = query_node;
	*req = r;
	return B_OK;
}

status_t google_request_free(struct google_request *req)
{
	if (!req)
		return EINVAL;
	free(req->query_string);
	free(req);
	return B_OK;
}
