/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef PACKAGE_INFO_ERROR_LISTENER_H
#define PACKAGE_INFO_ERROR_LISTENER_H


#include <package/PackageInfo.h>


using namespace BPackageKit;


class PackageInfoErrorListener : public BPackageInfo::ParseErrorListener {
public:
								PackageInfoErrorListener(
									const BString& errorContext);

	virtual	void				OnError(const BString& message, int line,
									int column);

private:
			BString				fErrorContext;
};


#endif	// PACKAGE_INFO_ERROR_LISTENER_H
