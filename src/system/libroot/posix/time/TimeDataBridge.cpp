/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "TimeBackend.h"


namespace BPrivate {


TimeDataBridge::TimeDataBridge()
	:
	addrOfDaylight(&daylight),
	addrOfTimezone(&timezone),
	addrOfTZName(tzname)
{
}


}	// namespace BPrivate
