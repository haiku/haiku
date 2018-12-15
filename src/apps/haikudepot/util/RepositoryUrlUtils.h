/*
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef REPOSITORY_URL_UTILS_H
#define REPOSITORY_URL_UTILS_H


#include <Url.h>


class RepositoryUrlUtils {

public:
	static void				NormalizeUrl(BUrl& url);
	static bool				EqualsNormalized(const BString& url1,
								const BString& url2);
	static bool				EqualsNormalized(const BUrl& normalizedUrl1,
								const BString& url2);
};


#endif // REPOSITORY_URL_UTILS_H
