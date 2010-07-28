/*
 * Copyright 2004-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg <mike@berg-net.us>
 *		Julun <host.haiku@gmx.de>
 */
#ifndef _BASE_VIEW_H
#define _BASE_VIEW_H


#include <Message.h>
#include <View.h>


class TTimeBaseView: public BView {
public:
								TTimeBaseView(BRect frame, const char* name);
	virtual						~TTimeBaseView();

	virtual	void			 	Pulse();
	virtual	void				AttachedToWindow();

			void				ChangeTime(BMessage* message);

private:
			void				_SendNotices();

private:
			BMessage			fMessage;
};


#endif	// _BASE_VIEW_H
