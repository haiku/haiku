// IFSSaver.h

#ifndef IFS_SAVER_H
#define IFS_SAVER_H

#include <Locker.h>
// on PPC the full path is needed
#include <add-ons/screen_saver/ScreenSaver.h>
#include <View.h>

#include "IFS.h"

class BCheckBox;
class BSlider;

class IFSSaver : public BScreenSaver, public BHandler {
 public:
								IFSSaver(BMessage *message,
											image_id image);
	virtual						~IFSSaver();

								// BScreenSaver
	virtual	void				StartConfig(BView *view);
	virtual	status_t			StartSaver(BView *view, bool preview);
	virtual	void				StopSaver();

	virtual	void				DirectConnected(direct_buffer_info* info);
	virtual	void				Draw(BView* view, int32 frame);
	virtual	void				DirectDraw(int32 frame);

	virtual	status_t			SaveState(BMessage* into) const;

								// BHandler
	virtual	void				MessageReceived(BMessage* message);

 private:
			void				_Init(BRect bounds);
			void				_Cleanup();

	IFS*						fIFS;

	bool						fIsPreview;
	BRect						fBounds;

	BLocker						fLocker;

	BCheckBox*					fAdditiveCB;
	BSlider*					fSpeedS;

	buffer_info					fDirectInfo;
	int32						fLastDrawnFrame;

	// config settings
	bool						fAdditive;
	int32						fSpeed;
};


#endif	//	IFS_SAVER_H
