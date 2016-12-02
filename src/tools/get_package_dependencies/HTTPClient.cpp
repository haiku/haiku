/*
 * Copyright 2016, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alexander von Gluck IV <kallisti5@unixzen.com>
 */


#include "HTTPClient.h"

#include <File.h>


HTTPClient::HTTPClient()
	:
	fCurl(NULL)
{
	fCurl = curl_easy_init();
	curl_easy_setopt(fCurl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(fCurl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(fCurl, CURLOPT_ACCEPT_ENCODING, "deflate");
	curl_easy_setopt(fCurl, CURLOPT_WRITEFUNCTION, &_WriteCallback);
}


HTTPClient::~HTTPClient()
{
	curl_easy_cleanup(fCurl);
}


size_t
HTTPClient::_WriteCallback(void *buffer, size_t size, size_t nmemb,
	void *userp)
{
	BFile* target = reinterpret_cast<BFile*>(userp);
	ssize_t dataWritten = target->Write(buffer, size * nmemb);
	return size_t(dataWritten);
}


status_t
HTTPClient::Get(BString* url, BEntry* destination)
{
	BFile target(destination, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);

	curl_easy_setopt(fCurl, CURLOPT_URL, url->String());
	curl_easy_setopt(fCurl, CURLOPT_WRITEDATA, &target);

    CURLcode res = curl_easy_perform(fCurl);

	// Check result
	if (res != CURLE_OK) {
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
		return B_ERROR;
	}

	return B_OK;
}
