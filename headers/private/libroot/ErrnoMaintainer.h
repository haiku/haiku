/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ERRNO_MAINTAINER_H
#define _ERRNO_MAINTAINER_H


#include <errno.h>


namespace BPrivate {


/**
 * A helper class resetting errno to 0 if it has been set during the execution
 * of ICU methods. Any changes of errno shall only be done by our callers.
 */
class ErrnoMaintainer {
public:
	ErrnoMaintainer()
		: fErrnoUponEntry(errno)
	{
	}

	~ErrnoMaintainer()
	{
		errno = fErrnoUponEntry;
	}

private:
	int fErrnoUponEntry;
};


}	// namespace BPrivate


#endif	// _ERRNO_MAINTAINER_H
