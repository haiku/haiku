/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the MIT License.
*/
#ifndef FILE_WINDOW_H
#define FILE_WINDOW_H


#include "ProbeWindow.h"

class ProbeView;


class FileWindow : public ProbeWindow {
	public:
		FileWindow(BRect rect, entry_ref *ref, const BMessage *settings = NULL);

		virtual bool QuitRequested();
		virtual bool Contains(const entry_ref &ref, const char *attribute);

	private:
		ProbeView	*fProbeView;
};

#endif	/* FILE_WINDOW_H */
