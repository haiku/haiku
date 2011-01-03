/*
 * Copyright 2004, pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef MONITOR_H
#define MONITOR_H


#include <SupportDefs.h>
#include <stdarg.h>


class Monitor {
	public:
		virtual ~Monitor() {}

		virtual void Update(const char *text, ...);
		virtual void Update(float progress, const char *text = NULL, ...);

		virtual void Done(status_t status);

	private:
		virtual void update(float progress, const char *text, va_list args) = 0;
};


inline void 
Monitor::Update(const char *text, ...)
{
	va_list args;

	va_start(args, text);
	update(-1.f, text, args);
	va_end(args);
}


inline void 
Monitor::Update(float progress, const char *text, ...)
{
	va_list args;

	va_start(args, text);
	update(progress, text, args);
	va_end(args);
}


inline void 
Monitor::Done(status_t /*status*/)
{
}

#endif	/* MONITOR_H */
