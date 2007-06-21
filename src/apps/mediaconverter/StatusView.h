// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#ifndef STATUS_VIEW_H
#define STATUS_VIEW_H


#include <String.h>
#include <View.h>


class StatusView : public BView {
	public:
								StatusView(BRect frame,
									uint32 resizingMode);
	virtual						~StatusView();

	protected:
	virtual void				AttachedToWindow();
	virtual void				Draw(BRect update);
	virtual void				MessageReceived(BMessage* message);

	public:
			void				SetStatus(const char* text);
			const char*			Status();

	private:
			BRect				fStatusRect;
			BString				fStatusString;
};


#endif // STATUS_VIEW_H

