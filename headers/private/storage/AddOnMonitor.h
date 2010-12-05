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
								AddOnMonitor();
									// Does not automatically run the looper.
								AddOnMonitor(AddOnMonitorHandler* handler);
									// Automatically runs the looper.
	virtual						~AddOnMonitor();

	virtual	status_t			InitCheck();

			void				SetHandler(AddOnMonitorHandler* handler);

private:
			status_t			fInitCheck;
			BMessageRunner*		fPulseRunner;
};


}; // namespace Storage
}; // namespace BPrivate


using namespace BPrivate::Storage;


#endif // _ADD_ON_MONITOR_H
