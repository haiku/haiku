/*
 * Copyright 2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Philippe Houdoin
 */
#ifndef PICTURE_VIEW_H
#define PICTURE_VIEW_H


#include <Node.h>
#include <View.h>


class BBitmap;


const uint32	kMsgLoadImage	= 'mLIM';


class PictureView : public BView {
public:
								PictureView(float width, float height,
									const entry_ref* ref);
	virtual						~PictureView();

			bool				HasChanged();
			void				Revert();
			void				Update(const entry_ref* ref);

			BBitmap*			Bitmap();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint point);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual void				WindowActivated(bool active);
	virtual	void				MakeFocus(bool focused);

private:
			void				_BeginDrag(BPoint sourcePoint);
			void				_HandleDrop(BMessage* message);
			void				_ShowPopUpMenu(BPoint screen);
			BBitmap*			_CopyPicture(uint8 alpha);

			void				_SetPicture(BBitmap* bitmap);

			BBitmap*			fPicture;
			BBitmap*			fOriginalPicture;
			BBitmap*			fDefaultPicture;
			bool				fShowingPopUpMenu;
			BRect				fPictureRect;
};

#endif
