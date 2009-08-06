/*
 * Copyright 2001-2009 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef _BMC_PRIVATE_H
#define _BMC_PRIVATE_H


#include <BeBuild.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MessageFilter.h>

class BMessageRunner;


class _BMCFilter_ : public BMessageFilter {
public:
								_BMCFilter_(BMenuField* menuField, uint32 what);
	virtual						~_BMCFilter_();

	virtual	filter_result		Filter(BMessage* message, BHandler** handler);

private:
			_BMCFilter_&		operator=(const _BMCFilter_&);

			BMenuField*			fMenuField;
};


class _BMCMenuBar_ : public BMenuBar {
public:
								_BMCMenuBar_(BRect frame, bool fixedSize,
									BMenuField* menuField);
								_BMCMenuBar_(bool fixedSize,
									BMenuField* menuField);
								_BMCMenuBar_(BMessage* data);
	virtual						~_BMCMenuBar_();

	static	BArchivable*		Instantiate(BMessage* data);

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);
	virtual	void				MessageReceived(BMessage* msg);
	virtual	void				MakeFocus(bool focused = true);

			void				TogglePopUpMarker(bool show)
									{ fShowPopUpMarker = show; }
			bool				IsPopUpMarkerShown() const
									{ return fShowPopUpMarker; }

	virtual BSize				MinSize();
	virtual	BSize				MaxSize();

private:
								_BMCMenuBar_&operator=(const _BMCMenuBar_&);

			void				_Init(bool setMaxContentWidth);

			BMenuField*			fMenuField;
			bool				fFixedSize;	
			BMessageRunner*		fRunner;
			bool				fShowPopUpMarker;
			float				fPreviousWidth;
};

#endif // _BMC_PRIVATE_H
