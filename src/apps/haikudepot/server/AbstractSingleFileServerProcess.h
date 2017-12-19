/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef ABSTRACT_SINGLE_FILE_SERVER_PROCESS_H
#define ABSTRACT_SINGLE_FILE_SERVER_PROCESS_H


#include "AbstractServerProcess.h"


class AbstractSingleFileServerProcess : public AbstractServerProcess {
public:
								AbstractSingleFileServerProcess(
									AbstractServerProcessListener* listener,
									uint32 options);
	virtual						~AbstractSingleFileServerProcess();

protected:
	virtual status_t			RunInternal();

	virtual status_t			ProcessLocalData() = 0;

	virtual	BString				UrlPathComponent() = 0;

	virtual	BPath&				LocalPath() = 0;
};

#endif // ABSTRACT_SINGLE_FILE_SERVER_PROCESS_H