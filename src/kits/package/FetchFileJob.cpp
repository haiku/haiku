/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 */


#include <package/FetchFileJob.h>

#include <stdlib.h>
#include <sys/wait.h>

#include <Path.h>


namespace BPackageKit {

namespace BPrivate {


FetchFileJob::FetchFileJob(const BContext& context, const BString& title,
	const BString& fileURL, const BEntry& targetEntry)
	:
	inherited(context, title),
	fFileURL(fileURL),
	fTargetEntry(targetEntry)
{
}


FetchFileJob::~FetchFileJob()
{
}


status_t
FetchFileJob::Execute()
{
	BPath targetPath;
	status_t result = fTargetEntry.GetPath(&targetPath);
	if (result != B_OK)
		return result;

	// TODO: implement for real, maybe using the "service kit"-GSOC HTTP-stuff?
	BString cmd = BString("curl -# -o ") << targetPath.Path() << " "
		<< fFileURL.String();

	int cmdResult = system(cmd.String());
	if (WIFSIGNALED(cmdResult)
		&& (WTERMSIG(cmdResult) == SIGINT || WTERMSIG(cmdResult) == SIGQUIT)) {
		return B_CANCELED;
	}

	return cmdResult == 0 ? B_OK : B_ERROR;
}


void
FetchFileJob::Cleanup(status_t jobResult)
{
	if (jobResult != B_OK)
		fTargetEntry.Remove();
}


}	// namespace BPrivate

}	// namespace BPackageKit
