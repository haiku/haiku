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
#include "google_request.h"

#include "googlefs.h"
#include "lists2.h"
#include "settings.h"
#include "string_utils.h"

#include <UrlProtocolRoster.h>
#include <UrlRequest.h>

using namespace BPrivate::Network;

#define DO_PUBLISH
//#define FAKE_INPUT "/boot/home/devel/drivers/googlefs/log2.html"

//#define TESTURL "http://www.google.com/search?as_q=google+api+&num=50&hl=en&ie=ISO-8859-1&btnG=Google+Search&as_epq=frequently+asked&as_oq=help&as_eq=plop&lr=lang_en&as_ft=i&as_filetype=&as_qdr=m3&as_nlo=&as_nhi=&as_occt=any&as_dt=i&as_sitesearch="

//#define BASEURL "http://www.google.com/search?as_q="
//#define Q "google+api+"
//#define EXTRAURL "&num=50&hl=en&ie=ISO-8859-1&btnG=Google+Search&as_epq=frequently+asked&as_oq=help&as_eq=plop&lr=lang_en&as_ft=i&as_filetype=&as_qdr=m3&as_nlo=&as_nhi=&as_occt=any&as_dt=i&as_sitesearch="

#define TESTURL "http://www.google.com/search?hl=en&ie=UTF-8&num=50&q=beos"
#define BASEURL "https://www.google.com/search?hl=en&ie=UTF-8&oe=UTF-8&gws_rd=cr"
#define FMT_NUM "&num=%u"
#define FMT_Q "&q=%s"

/* parse_google_html.c */
extern int google_parse_results(const char *html, size_t htmlsize, long *nextid, struct google_result **results);


status_t google_request_process(struct google_request *req)
{
	struct BUrlRequest *cnx = NULL;
	struct google_result *res;
	status_t err;
	int count;
	char *p = NULL;
	char *url = NULL;
	BMallocIO output;
	thread_id t;
	
	err = ENOMEM;
	req->cnx = cnx;
#ifndef FAKE_INPUT
	p = urlify_string(req->query_string);
	if (!p)
		goto err_con;
	
	err = ENOMEM;
	url = (char*)malloc(strlen(BASEURL)+strlen(FMT_NUM)+10+strlen(FMT_Q)+strlen(p)+2);
	if (!url)
		goto err_url;
	strcpy(url, BASEURL);
	sprintf(url+strlen(url), FMT_NUM, (unsigned int)max_results);
	sprintf(url+strlen(url), FMT_Q, p);
	
	fprintf(stderr, "google_request: final URL: %s\n", url);
	
	cnx = BUrlProtocolRoster::MakeRequest(url, &output, NULL);
	if (cnx == NULL)
		return ENOMEM;

	t = cnx->Run();
	wait_for_thread(t, &err);
	
	fprintf(stderr, "google_request: buffer @ %p, len %ld\n", output.Buffer(), output.BufferLength());
	{
		int fd;
		// debug output
		fd = open("/tmp/google.html", O_CREAT|O_TRUNC|O_RDWR, 0644);
		write(fd, output.Buffer(), output.BufferLength());
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
	err = count = google_parse_results((const char*)output.Buffer(), output.BufferLength(),
		&req->nextid, &req->results);
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
	// request is kept and deleted in google_request_close
	return B_OK;


err_get:
	free(url);
err_url:
	free(p);
err_con:
	delete cnx;
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
	delete(req->cnx);
	req->cnx = NULL;
	return B_OK;
}

status_t google_request_open(const char *query_string, struct fs_volume *volume, struct fs_node *query_node, struct google_request **req)
{
	struct google_request *r;
	if (!req)
		return EINVAL;
	r = (google_request*)malloc(sizeof(struct google_request));
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
