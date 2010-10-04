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

protected:
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Draw(BRect updateRect);
	virtual void				FrameResized(float width, float height);

private:
			void				_RestartPulsing();
			void				_Compute(const BRect& bounds, BPoint& center,
									int32& count, float& step) const;
			void				_DrawBow(int32 index, const BPoint& center,
									int32 count, float step);
			void				_SetColor(int32 index, int32 count);
			bool				_IsDisabled(int32 index, int32 count) const;

private:
			int32				fPercent;
			BMessageRunner*		fPulse;
			int32				fPhase;
			int32				fMax;
};


#endif	// RADIO_VIEW_H
