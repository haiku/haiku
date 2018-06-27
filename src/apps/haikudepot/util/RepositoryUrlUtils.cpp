/*
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "RepositoryUrlUtils.h"


void
RepositoryUrlUtils::NormalizeUrl(BUrl& url)
{
	if (url.Protocol() == "https")
		url.SetProtocol("http");

	BString path(url.Path());

	if (path.EndsWith("/"))
		url.SetPath(path.Truncate(path.Length() - 1));
}


bool
RepositoryUrlUtils::EqualsNormalized(const BString& url1, const BString& url2)
{
	if (url1.IsEmpty())
		return false;

	BUrl normalizedUrl1(url1);
	NormalizeUrl(normalizedUrl1);
	BUrl normalizedUrl2(url2);
	NormalizeUrl(normalizedUrl2);

	return normalizedUrl1 == normalizedUrl2;
}


/*! Matches on either the identifier URL of the repo or the 'base' URL that was
    used to access the repository over the internet.  The use of the 'base' URL
    is deprecated.
*/

bool
RepositoryUrlUtils::EqualsOnUrlOrBaseUrl(const BString& url1,
	const BString& url2, const BString& baseUrl1,
	const BString& baseUrl2)
{
	return (!url1.IsEmpty() && url1 == url2)
		|| EqualsNormalized(baseUrl1, baseUrl2);
}