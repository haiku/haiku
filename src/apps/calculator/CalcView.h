/*
 * Copyright (c) 2006, Haiku, Inc.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Peter Wagner <pwagner@stanfordalumni.org>
 */
#ifndef CALC_VIEW_H
#define CALC_VIEW_H


#include <View.h>
#include <Message.h>
#include "FrameView.h"
#include "CalcEngine.h"

class BStringView;


class _EXPORT CalcView : public BView {
	public:
						CalcView(BRect frame);
						CalcView(BMessage *msg);
		virtual			~CalcView();

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

		static void		About();
	
		static float	TheWidth() { return 272; }
		static float	TheHeight() { return 284; }
		static BRect	TheRect() { return TheRect(0,0); }
		static BRect	TheRect(const BPoint &pt) { return TheRect(pt.x,pt.y); }
		static BRect	TheRect(float x, float y) { return BRect(x,y,x+TheWidth(),y+TheHeight()); }
		static float	ExtraHeight() { return 43; }

	private:
		void Init();

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

#endif	// CALC_VIEW_H
