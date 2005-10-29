#ifndef _ADD_ON_MONITOR_H
#define _ADD_ON_MONITOR_H

#include <string>
#include <list>
#include <Looper.h>
#include <MessageRunner.h>
#include <stdio.h>

namespace BPrivate {
namespace Storage {

class AddOnMonitorHandler;

class AddOnMonitor : public BLooper {
private:
	typedef BLooper inherited;
public:
			AddOnMonitor(AddOnMonitorHandler * handler);
	virtual	~AddOnMonitor();

	virtual status_t	InitCheck();

private:
	status_t	fInitCheck;
	BMessage *	fPulseMessage;
	BMessageRunner * fPulseRunner;
};

}; // namespace Storage
}; // namespace BPrivate

using namespace BPrivate::Storage;

#endif // _ADD_ON_MONITOR_H
