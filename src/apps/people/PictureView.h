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

private:
			BBitmap*			fPicture;
			BBitmap*			fOriginalPicture;
			BBitmap*			fDefaultPicture;
};

#endif
