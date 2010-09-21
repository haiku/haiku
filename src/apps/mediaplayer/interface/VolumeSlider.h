/*
 * Copyright 2006-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef VOLUME_SLIDER_H
#define VOLUME_SLIDER_H


#include <Slider.h>


class VolumeSlider : public BSlider {
public:
								VolumeSlider(const char* name,
									int32 minValue, int32 maxValue,
									int32 snapValue, BMessage* message = NULL);

	virtual						~VolumeSlider();

	// BSlider interface
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

	virtual BRect				ThumbFrame() const;
	virtual	void				DrawThumb();

	virtual	BSize				MinSize();

	// VolumeSlider
			void				SetMuted(bool mute);
			bool				IsMuted() const
									{ return fMuted; }

			float				PreferredBarThickness() const;

private:
			float				_PointForValue(int32 value) const;

private:
			bool				fMuted;

			int32				fSnapValue;
			bool				fSnapping;
			float				fMinSnap;
			float				fMaxSnap;
};

#endif	// VOLUME_SLIDER_H
