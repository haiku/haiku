/*
 * Copyright 2001-2006, Haiku, Inc.
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

class _BMCFilter_ : public BMessageFilter {
public:
	_BMCFilter_(BMenuField *menuField, uint32 what);
	~_BMCFilter_();
	filter_result Filter(BMessage *message, BHandler **handler);

private:
	_BMCFilter_ &operator=(const _BMCFilter_ &);
	BMenuField *fMenuField;
};


//------------------------------------------------------------------------------
class _BMCMenuBar_ : public BMenuBar {

public:
					_BMCMenuBar_(BRect frame, bool fixed_size,
						BMenuField *menuField);
					_BMCMenuBar_(BMessage *data);
virtual				~_BMCMenuBar_();

static	BArchivable	*Instantiate(BMessage *data);

virtual	void		AttachedToWindow();
virtual	void		Draw(BRect updateRect);
virtual	void		FrameResized(float width, float height);
virtual	void		MessageReceived(BMessage* msg);
virtual	void		MakeFocus(bool focused = true);

	void		TogglePopUpMarker(bool show) { fShowPopUpMarker = show; }
	bool		IsPopUpMarkerShown() const { return fShowPopUpMarker; }
	
private:
		_BMCMenuBar_&operator=(const _BMCMenuBar_ &);
		
		BMenuField	*fMenuField;
		bool		fFixedSize;	
		BMessageRunner	*fRunner;
		bool		fShowPopUpMarker;
};
//------------------------------------------------------------------------------

#endif // _BMC_PRIVATE_H
