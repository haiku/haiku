/*
 * Copyright 2011-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe <zooey@hirschkaefer.de>
 *		Rene Gollent <rene@gollent.com>
 */


#include <package/Job.h>


namespace BPackageKit {


BJob::BJob(const BContext& context, const BString& title)
	:
	BSupportKit::BJob(title),
	fContext(context)
{
}


BJob::~BJob()
{
}


}	// namespace BPackageKit
