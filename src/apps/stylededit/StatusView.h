/*
 * Copyright 2002-2012, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Vlad Slepukhin
 *		Siarzhuk Zharski
 */
#ifndef STATUS_VIEW_H
#define STATUS_VIEW_H


#include <String.h>
#include <View.h>


enum {
	kPositionCell,
	kEncodingCell,
	kFileStateCell,
	kStatusCellCount
};


class BScrollView;

class StatusView : public BView {
public:
							StatusView(BScrollView* fScrollView);
							~StatusView();

			void			SetStatus(BMessage* mesage);
	virtual	void			AttachedToWindow();
	virtual void			GetPreferredSize(float* _width, float* _height);
	virtual	void			ResizeToPreferred();
	virtual	void			Draw(BRect bounds);
	virtual	void			MouseDown(BPoint point);

private:
			void			_ValidatePreferredSize();

private:
			BScrollView*	fScrollView;
			BSize			fPreferredSize;
			BString			fCellText[kStatusCellCount];
			float			fCellWidth[kStatusCellCount];
			bool			fReadOnly;
			BString			fEncoding;
};

#endif  // STATUS_VIEW_H
