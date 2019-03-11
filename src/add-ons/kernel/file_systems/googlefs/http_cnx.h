/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _HTTP_CNX_H
#define _HTTP_CNX_H

#include <OS.h>

struct http_cnx {
	int sock;
	status_t err; /* 404, ... */
	char *headers;
	size_t headerslen;
	char *data;
	size_t datalen;
	/* for bookkeeping */
	char *buffer;
};

struct sockaddr_in;

extern status_t http_init(void);
extern status_t http_uninit(void);

extern status_t http_create(struct http_cnx **);
extern status_t http_delete(struct http_cnx *cnx);

extern status_t http_connect(struct http_cnx *cnx, struct sockaddr_in *sin);
extern status_t http_close(struct http_cnx *cnx);

extern status_t http_get(struct http_cnx *cnx, const char *url);
//extern status_t http_post(struct http_cnx *cnx, const char *url);
/* read the actual data in the buffer */
//extern status_t http_fetch_data(struct http_cnx *cnx);

#endif /* _HTTP_CNX_H */
