/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef PROBE_VIEW_H
#define PROBE_VIEW_H


#include <View.h>
#include <Path.h>

class BTextControl;
class BStringView;
class BSlider;

class HeaderView;


class ProbeView : public BView {
	public:
		ProbeView(BRect rect, entry_ref *ref, const char *attribute = NULL);
		virtual ~ProbeView();

		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);

	private:
		bool			fIsDevice;
		HeaderView		*fHeaderView;
};

#endif	/* PROBE_WINDOW_H */
