// Copyright 1999, Be Incorporated. All Rights Reserved.
// Copyright 2000-2004, Jun Suzuki. All Rights Reserved.
// Copyright 2007, Stephan Aßmus. All Rights Reserved.
// This file may be used under the terms of the Be Sample Code License.
#ifndef MEDIA_ENCODER_WINDOW_H
#define MEDIA_ENCODER_WINDOW_H


#include <Window.h>

class MediaEncoderWindow : public BWindow {
	public:
								MediaEncoderWindow(BRect frame,
									BView* pBView);
	virtual						~MediaEncoderWindow();

			void				Go();	

	protected:
	virtual	void				MessageReceived(BMessage *msg);
	virtual	bool				QuitRequested();

	private:
			BView*				fView;
			sem_id				fQuitSem;

};

#endif // MEDIA_ENCODER_WINDOW_H
