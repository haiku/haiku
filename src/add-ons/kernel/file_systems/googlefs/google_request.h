/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GOOGLE_REQUEST_H
#define _GOOGLE_REQUEST_H

struct google_request {
	struct google_request *next;
	struct fs_volume *volume;
	struct fs_node *query_node; /* root folder for that query */
	char *query_string;
	struct http_cnx *cnx;
	struct google_result *results;
	long nextid;
};

/* those are quite arbitrary values */
#define GR_MAX_NAME 70
#define GR_MAX_PROTO 16
#define GR_MAX_URL 1024
#define GR_MAX_SNIPSET 256
struct google_result {
	struct google_result *next;
	long id;
	char name[GR_MAX_NAME];
	char proto[GR_MAX_PROTO];
	char url[GR_MAX_URL];
	char snipset[GR_MAX_SNIPSET];
	char cache_url[GR_MAX_URL];
	char similar_url[GR_MAX_URL];
};

extern status_t google_request_init(void);
extern status_t google_request_uninit(void);
extern status_t google_request_process(struct google_request *req);
extern status_t google_request_process_async(struct google_request *req);
extern status_t google_request_close(struct google_request *req);
extern status_t google_request_open(const char *query_string, struct fs_volume *volume, struct fs_node *query_node, struct google_request **req);
extern status_t google_request_free(struct google_request *req);

extern int google_parse_results(const char *html, size_t htmlsize, long *nextid, struct google_result **results);

#endif /* _GOOGLE_REQUEST_H */
