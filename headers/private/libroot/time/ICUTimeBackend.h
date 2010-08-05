/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICU_TIME_BACKEND_H
#define _ICU_TIME_BACKEND_H


#include <TimeBackend.h>


namespace BPrivate {


class ICUTimeBackend : public TimeBackend {
public:
								ICUTimeBackend();
	virtual						~ICUTimeBackend();

	virtual	const status_t		TZSet();

	virtual void				Initialize(TimeDataBridge* dataBridge);

private:
			TimeDataBridge		fDataBridge;
};


}	// namespace BPrivate


#endif	// _ICU_TIME_BACKEND_H
