/*
 * Copyright 2004-2008, François Revol, <revol@free.fr>.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DUCKDUCKGO_REQUEST_H
#define _DUCKDUCKGO_REQUEST_H

#ifdef __cplusplus
#include <UrlRequest.h>
using namespace BPrivate::Network;
extern "C" {
#else
struct BUrlRequest;
typedef struct BUrlRequest BUrlRequest;
#endif

struct duckduckgo_request {
	struct duckduckgo_request *next;
	struct fs_volume *volume;
	struct fs_node *query_node; /* root folder for that query */
	char *query_string;
	BUrlRequest *cnx;
	struct duckduckgo_result *results;
	long nextid;
};

/* those are quite arbitrary values */
#define GR_MAX_NAME 70
#define GR_MAX_PROTO 16
#define GR_MAX_URL 1024
#define GR_MAX_SNIPSET 256
struct duckduckgo_result {
	struct duckduckgo_result *next;
	long id;
	char name[GR_MAX_NAME];
	char proto[GR_MAX_PROTO];
	char url[GR_MAX_URL];
	char snipset[GR_MAX_SNIPSET];
	char cache_url[GR_MAX_URL];
	char similar_url[GR_MAX_URL];
};

extern status_t duckduckgo_request_process(struct duckduckgo_request *req);
extern status_t duckduckgo_request_process_async(struct duckduckgo_request *req);
extern status_t duckduckgo_request_close(struct duckduckgo_request *req);
extern status_t duckduckgo_request_open(const char *query_string, struct fs_volume *volume, struct fs_node *query_node, struct duckduckgo_request **req);
extern status_t duckduckgo_request_free(struct duckduckgo_request *req);

extern int duckduckgo_parse_results(const char *html, size_t htmlsize, long *nextid, struct duckduckgo_result **results);

#ifdef __cplusplus
}
#endif

#endif /* _DUCKDUCKGO_REQUEST_H */
