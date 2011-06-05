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


#include <LayoutBuilder.h>
#include <Message.h>


class TTimeBaseView : public BGroupView {
public:
								TTimeBaseView(const char* name);
	virtual						~TTimeBaseView();

	virtual	void			 	Pulse();
	virtual	void				AttachedToWindow();

			void				ChangeTime(BMessage* message);

private:
			void				_SendNotices();
			
			BMessage			fMessage;
};


#endif	// _BASE_VIEW_H
