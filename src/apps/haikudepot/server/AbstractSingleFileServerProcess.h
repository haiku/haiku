/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#ifndef ABSTRACT_SINGLE_FILE_SERVER_PROCESS_H
#define ABSTRACT_SINGLE_FILE_SERVER_PROCESS_H


#include "AbstractServerProcess.h"


class AbstractSingleFileServerProcess : public AbstractServerProcess {
public:
								AbstractSingleFileServerProcess(uint32 options);
	virtual						~AbstractSingleFileServerProcess();

protected:
	virtual status_t			RunInternal();

	virtual status_t			ProcessLocalData() = 0;

	virtual	BString				UrlPathComponent() = 0;

	virtual	status_t			GetLocalPath(BPath& path) const = 0;

	virtual	status_t			GetStandardMetaDataPath(BPath& path) const;
};

#endif // ABSTRACT_SINGLE_FILE_SERVER_PROCESS_H