/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CHANNEL_SLIDER_H
#define _CHANNEL_SLIDER_H


#include <ChannelControl.h>


class BChannelSlider : public BChannelControl {
public:
								BChannelSlider(BRect area, const char* name,
									const char* label, BMessage* message,
									int32 channels = 1,
									uint32 resizeMode
										= B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW);
								BChannelSlider(BRect area, const char* name,
									const char* label, BMessage* message,
									enum orientation orientation,
									int32 channels = 1,
									uint32 resizeMode
										= B_FOLLOW_LEFT | B_FOLLOW_TOP,
									uint32 flags = B_WILL_DRAW);
								BChannelSlider(const char* name,
									const char* label, BMessage* message,
									enum orientation orientation,
									int32 channels = 1,
									uint32 flags = B_WILL_DRAW);
								BChannelSlider(BMessage* archive);
	virtual						~BChannelSlider();

	static	BArchivable*		Instantiate(BMessage* from);
	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;

	virtual	void				AttachedToWindow();
	virtual	void				AllAttached();
	virtual	void				DetachedFromWindow();
	virtual	void				AllDetached();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				WindowActivated(bool state);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				KeyUp(const char* bytes, int32 numBytes);
	virtual	void				FrameResized(float width, float height);

	virtual	void				SetFont(const BFont* font,
									uint32 mask = B_FONT_ALL);
	virtual	void				MakeFocus(bool focusState = true);

	virtual	void				GetPreferredSize(float* _width, float* _height);

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

	virtual	void				SetEnabled(bool on);

	virtual	orientation			Orientation() const;
			void				SetOrientation(enum orientation orientation);

	virtual	int32				MaxChannelCount() const;
	virtual	bool				SupportsIndividualLimits() const;

	virtual	void				DrawChannel(BView* into, int32 channel,
									BRect area, bool pressed);

	virtual	void				DrawGroove(BView* into, int32 channel,
									BPoint leftTop, BPoint rightBottom);

	virtual	void				DrawThumb(BView* into, int32 channel,
									BPoint where, bool pressed);

	virtual	const BBitmap*		ThumbFor(int32 channel, bool pressed);
	virtual	BRect				ThumbFrameFor(int32 channel);
	virtual	float				ThumbDeltaFor(int32 channel);
	virtual	float				ThumbRangeFor(int32 channel);

private:
	// FBC padding
								BChannelSlider(const BChannelSlider&);
			BChannelSlider&		operator=(const BChannelSlider&);


	virtual	void				_Reserved_BChannelSlider_0(void*, ...);
	virtual	void				_Reserved_BChannelSlider_1(void*, ...);
	virtual	void				_Reserved_BChannelSlider_2(void*, ...);
	virtual	void				_Reserved_BChannelSlider_3(void*, ...);
	virtual	void				_Reserved_BChannelSlider_4(void*, ...);
	virtual	void				_Reserved_BChannelSlider_5(void*, ...);
	virtual	void				_Reserved_BChannelSlider_6(void*, ...);
	virtual	void				_Reserved_BChannelSlider_7(void*, ...);

private:
			void				_InitData();
			void				_FinishChange(bool update = false);
			void				_UpdateFontDimens();
			void				_DrawThumbs();
			void				_DrawGrooveFrame(BView* where,
									const BRect& area);
			void				_MouseMovedCommon(BPoint point, BPoint point2);

private:
			float				fBaseLine;
			float				fLineFeed;
			BBitmap*			fLeftKnob;
			BBitmap*			fMidKnob;
			BBitmap*			fRightKnob;
			BBitmap*			fBacking;
			BView*				fBackingView;
			bool				fIsVertical;
			bool				_padding_[3];
			BPoint				fClickDelta;

			int32				fCurrentChannel;
			bool				fAllChannels;
			int32*				fInitialValues;
			float				fMinPoint;
			int32				fFocusChannel;

			uint32				_reserved_[12];
};


#endif // _CHANNEL_SLIDER_H
