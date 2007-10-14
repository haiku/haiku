/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 */
#ifndef TIMEBASE_H
#define TIMEBASE_H


#include <Message.h>
#include <View.h>


class TTimeBaseView: public BView {
	public:
						TTimeBaseView(BRect frame, const char *name);
		virtual 		~TTimeBaseView();
		
		virtual void 	Pulse();
		virtual void 	AttachedToWindow();

		void 			ChangeTime(BMessage *message);

	private:
		void			_SendNotices();

	private:
		BMessage 		fMessage;
};

#endif	// TIMEBASE_H

