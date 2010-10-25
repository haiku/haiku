/*
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef RADIO_VIEW_H
#define RADIO_VIEW_H


#include <View.h>


class BMessageRunner;


class RadioView : public BView {
public:
								RadioView(BRect frame, const char* name,
									int32 resizingMode);
	virtual						~RadioView();

			void				SetPercent(int32 percent);
			void				SetMax(int32 max);

			void				StartPulsing();
			void				StopPulsing();
			bool				IsPulsing() const
									{ return fPulse != NULL; }

	static	void				Draw(BView* view, BRect rect, int32 percent,
									int32 count);
	static	int32				DefaultMax();

protected:
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Draw(BRect updateRect);
	virtual void				FrameResized(float width, float height);

private:
			void				_RestartPulsing();

	static	void				_Compute(const BRect& bounds, BPoint& center,
									int32& count, int32 max, float& step);
	static	void				_DrawBow(BView* view, int32 index,
									const BPoint& center, int32 count,
									float step);
	static	void				_SetColor(BView* view, int32 percent,
									int32 phase, int32 index, int32 count);
	static	bool				_IsDisabled(int32 percent, int32 index,
									int32 count);

private:
			int32				fPercent;
			BMessageRunner*		fPulse;
			int32				fPhase;
			int32				fMax;
};


#endif	// RADIO_VIEW_H
