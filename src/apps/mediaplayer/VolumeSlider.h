/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef VOLUME_SLIDER_H
#define VOLUME_SLIDER_H

#include <Control.h>

class VolumeSlider : public BControl {
 public:
								VolumeSlider(BRect frame,
											 const char* name,
											 int32 minValue,
											 int32 maxValue,
											 BMessage* message = NULL,
											 BHandler* target = NULL);

	virtual						~VolumeSlider();

								// BControl
	virtual	void				AttachedToWindow();
	virtual	void				SetValue(int32 value);
			void				SetValueNoInvoke(int32 value);
	virtual void				SetEnabled(bool enable);
	virtual void				Draw(BRect updateRect);
	virtual void				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);
	virtual	void				MouseUp(BPoint where);

								// VolumeSlider
			bool				IsValid() const;
			void				SetMuted(bool mute);
			bool				IsMuted() const
									{ return fMuted; }

 private:
			void				_MakeBitmaps();
			void				_DimBitmap(BBitmap* bitmap);
			int32				_ValueFor(float xPos) const;

			BBitmap*			fLeftSideBits;
			BBitmap*			fRightSideBits;
			BBitmap*			fKnobBits;
			bool				fTracking;
			bool				fMuted;
			int32				fMinValue;
			int32				fMaxValue;
};

#endif	// VOLUME_SLIDER_H
