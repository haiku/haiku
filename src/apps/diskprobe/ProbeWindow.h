/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef PROBE_WINDOW_H
#define PROBE_WINDOW_H


#include <Window.h>
#include <Entry.h>


class ProbeWindow : public BWindow {
	public:
		ProbeWindow(BRect rect, entry_ref *ref);
		virtual ~ProbeWindow();

		virtual void MessageReceived(BMessage *message);
		virtual bool QuitRequested();

		virtual bool Contains(const entry_ref &ref, const char *attribute) = 0;

	protected:
		const entry_ref &Ref() const { return fRef; }

	private:
		entry_ref	fRef;
};

#endif	/* PROBE_WINDOW_H */
