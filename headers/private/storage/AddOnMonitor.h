/*
 * Copyright 2004-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ADD_ON_MONITOR_H
#define _ADD_ON_MONITOR_H


#include <list>
#include <stdio.h>
#include <string>

#include <Looper.h>
#include <MessageRunner.h>


namespace BPrivate {
namespace Storage {


class AddOnMonitorHandler;


class AddOnMonitor : public BLooper {
private:
			typedef BLooper inherited;
public:
								AddOnMonitor(AddOnMonitorHandler* handler);
	virtual						~AddOnMonitor();

	virtual	status_t			InitCheck();

private:
			status_t			fInitCheck;
			BMessageRunner*		fPulseRunner;
};


}; // namespace Storage
}; // namespace BPrivate


using namespace BPrivate::Storage;


#endif // _ADD_ON_MONITOR_H
