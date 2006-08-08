/*
 * Copyright 2005, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef STATUS_VIEW__H
#define STATUS_VIEW__H

#include <View.h>
#include <PPPInterface.h>


class PPPStatusView : public BView {
	public:
		PPPStatusView(BRect rect, ppp_interface_id id);
		
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
