/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include "PackageInfoErrorListener.h"

#include <stdio.h>


PackageInfoErrorListener::PackageInfoErrorListener(const BString& errorContext)
	:
	fErrorContext(errorContext)
{
}


void
PackageInfoErrorListener::OnError(const BString& message, int line, int column)
{
	fprintf(stderr, "%s: Parse error in line %d:%d: %s\n",
		fErrorContext.String(), line, column, message.String());
}
