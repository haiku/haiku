/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _TIME_BACKEND_H
#define _TIME_BACKEND_H


#include <SupportDefs.h>


namespace BPrivate {


struct TimeDataBridge {
	int*	addrOfDaylight;
	long*	addrOfTimezone;
	char**	addrOfTZName;

	TimeDataBridge();
};


class TimeBackend {
public:
								TimeBackend();
	virtual						~TimeBackend();

	virtual	const status_t		TZSet() = 0;

	virtual void				Initialize(TimeDataBridge* dataBridge) = 0;

	static	status_t			LoadBackend();
};


extern TimeBackend* gTimeBackend;


}	// namespace BPrivate


#endif	// _TIME_BACKEND_H
