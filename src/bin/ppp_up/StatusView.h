/*
 * Copyright 2005, Waldemar Kornewald <Waldemar.Kornewald@web.de>
 * Distributed under the terms of the MIT License.
 */

#ifndef STATUS_VIEW__H
#define STATUS_VIEW__H

#include <View.h>
#include <PPPInterface.h>


class StatusView : public BView {
	public:
		StatusView(BRect rect, ppp_interface_id id);
		
		virtual void AttachedToWindow();
		virtual void MessageReceived(BMessage *message);
		virtual void Pulse();

	private:
		BButton *fButton;
		BStringView *fTime;
		BStringView *fBytesReceived, *fBytesSent, *fPacketsReceived, *fPacketsSent;
		bigtime_t fConnectedSince;
		PPPInterface fInterface;
};


#endif
