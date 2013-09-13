/*
 * Copyright 2001-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus, superstippi@gmx.de
 *		Marc Flerackers, mflerackers@androme.be
 *		John Scipione, jscipione@gmail.com
 */
#ifndef _BMC_PRIVATE_H
#define _BMC_PRIVATE_H


#include <BeBuild.h>
#include <MenuBar.h>
#include <MessageFilter.h>


static const float kVMargin = 2.0f;


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
								_BMCMenuBar_(BMenuField* menuField);
								_BMCMenuBar_(BMessage* data);
	virtual						~_BMCMenuBar_();

	static	BArchivable*		Instantiate(BMessage* data);

	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);
	virtual	void				MakeFocus(bool focused = true);
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				SetMaxContentWidth(float width);
	virtual	void				SetEnabled(bool enabled);

			void				TogglePopUpMarker(bool show)
									{ fShowPopUpMarker = show; }
			bool				IsPopUpMarkerShown() const
									{ return fShowPopUpMarker; }

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

private:
								_BMCMenuBar_&operator=(const _BMCMenuBar_&);

			void				_Init();

			BMenuField*			fMenuField;
			bool				fFixedSize;	
			bool				fShowPopUpMarker;
			float				fPreviousWidth;
};

#endif // _BMC_PRIVATE_H
