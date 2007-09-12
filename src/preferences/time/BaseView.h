/*
 * Copyright 2004-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Mike Berg (inseculous)
 *		Julun <host.haiku@gmx.de>
 */
#ifndef TIMEBASE_H
#define TIMEBASE_H


#include <View.h>
#include <Message.h>


class TTimeBaseView: public BView {
	public:
						TTimeBaseView(BRect frame, const char *name);
		virtual 		~TTimeBaseView();
		
		virtual void 	Pulse();
		virtual void 	AttachedToWindow();

		void 			SetGMTime(bool gmtTime);
		void 			ChangeTime(BMessage *message);

	protected:
		virtual void 	DispatchMessage();

	private:
		bool 			fIsGMT;
		BMessage 		fMessage;
};

#endif	// TIMEBASE_H

