/*
 * Copyright 2016, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */
#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H


#include <String.h>
#include <Entry.h>
#include <curl/curl.h>


class HTTPClient {
public:
						HTTPClient();
						~HTTPClient();

			status_t	Get(BString* url, BEntry* destination);

private:
	static	size_t		_WriteCallback(void *buffer, size_t size, size_t nmemb,
							void *userp);

			CURL*		fCurl;
};


#endif /* HTTP_CLIENT_H */
