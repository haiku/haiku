/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Peter Wagner <pwagner@stanfordalumni.org>
 */
#ifndef __CALC_VIEW__
#define __CALC_VIEW__

#include <View.h>
#include <Message.h>
#include "FrameView.h"
#include "CalcEngine.h"

class _EXPORT CalcView : public BView
{
public:
	static void About(void);


	static float TheWidth() { return 272; }
	static float TheHeight() { return 284; }
	static BRect TheRect() { return TheRect(0,0); }
	static BRect TheRect(const BPoint &pt) { return TheRect(pt.x,pt.y); }
	static BRect TheRect(float x, float y) { return BRect(x,y,x+TheWidth(),y+TheHeight()); }
	static float ExtraHeight() { return 43; }


					CalcView(BRect frame);
	virtual			~CalcView();


	// replicant
					CalcView(BMessage *msg);
	virtual status_t Archive(BMessage *msg, bool deep) const;
	static BArchivable* Instantiate(BMessage *msg);

	virtual	void	AttachedToWindow();
	virtual	void	AllAttached();

	virtual void	Draw(BRect updateRect);
	virtual void	KeyDown(const char *bytes, int32 numBytes);
	virtual	void	MouseDown(BPoint where);
	virtual void	WindowActivated(bool state);
	virtual	void	MakeFocus(bool focusState = true);

	virtual	void	MessageReceived(BMessage *message);

	void			SendKeystroke(ulong key);

	virtual bool	TextWidthOK(const char *str);

	BView 			*AddExtra();

private:
	void Init(void);

	CalcEngine calc;
	bool is_replicant;

	int current_highlight;
	enum { state_unknown=0, state_active, state_inactive };
	void ChangeHighlight();


	FrameView *lcd_frame;
	BStringView	*lcd;
	BStringView *binary_sign_view;
	BStringView *binary_exponent_view;
	BStringView *binary_number_view;

};

#endif
