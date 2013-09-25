/*
 * Copyright 2011-2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 *		Rene Gollent <rene@gollent.com>
 */


#include <package/FetchFileJob.h>

#include <stdio.h>
#include <curl/curl.h>
#include <sys/wait.h>

#include <Path.h>


namespace BPackageKit {

namespace BPrivate {


FetchFileJob::FetchFileJob(const BContext& context, const BString& title,
	const BString& fileURL, const BEntry& targetEntry)
	:
	inherited(context, title),
	fFileURL(fileURL),
	fTargetEntry(targetEntry),
	fTargetFile(&targetEntry, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY)
{
}


FetchFileJob::~FetchFileJob()
{
}


status_t
FetchFileJob::Execute()
{
	status_t result = fTargetFile.InitCheck();
	if (result != B_OK)
		return result;

	CURL* handle = curl_easy_init();

	if (handle == NULL)
		return B_NO_MEMORY;

	result = curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0);

	result = curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION,
		&_ProgressCallback);
	if (result != CURLE_OK)
		return B_BAD_VALUE;

	result = curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, this);
	if (result != CURLE_OK)
		return B_ERROR;

	result = curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION,
		&_WriteCallback);
	if (result != CURLE_OK)
		return B_ERROR;

	result = curl_easy_setopt(handle, CURLOPT_WRITEDATA, this);
	if (result != CURLE_OK)
		return B_ERROR;

	result = curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION,
		&_ProgressCallback);
	if (result != CURLE_OK)
		return B_ERROR;

	result = curl_easy_setopt(handle, CURLOPT_URL, fFileURL.String());
	if (result != CURLE_OK)
		return B_ERROR;

	result = curl_easy_perform(handle);
	curl_easy_cleanup(handle);
	if (result != CURLE_OK) {
		// TODO: map more curl error codes to ours for more
		// precise error reporting
		return B_ERROR;
	}

/*	if (WIFSIGNALED(cmdResult)
		&& (WTERMSIG(cmdResult) == SIGINT || WTERMSIG(cmdResult) == SIGQUIT)) {
		return B_CANCELED;
	} */

	return B_OK;
}


int
FetchFileJob::_ProgressCallback(void *clientp, double dltotal, double dlnow,
	double ultotal,	double ulnow)
{
	return 0;
}


size_t
FetchFileJob::_WriteCallback(void *buffer, size_t size, size_t nmemb,
	void *userp)
{
	FetchFileJob* job = reinterpret_cast<FetchFileJob*>(userp);
	ssize_t dataWritten = job->fTargetFile.Write(buffer, size * nmemb);
	return size_t(dataWritten);
}


void
FetchFileJob::Cleanup(status_t jobResult)
{
	if (jobResult != B_OK)
		fTargetEntry.Remove();
}


}	// namespace BPrivate

}	// namespace BPackageKit
