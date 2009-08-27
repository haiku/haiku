/*
 * Copyright 2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _COLOR_CONTROL_H
#define _COLOR_CONTROL_H


#include <Control.h>

class BBitmap;


enum color_control_layout {
	B_CELLS_4x64	= 4,
	B_CELLS_8x32	= 8,
	B_CELLS_16x16	= 16,
	B_CELLS_32x8	= 32,
	B_CELLS_64x4	= 64,
};

class BTextControl;


class BColorControl : public BControl {
public:
								BColorControl(BPoint start,
									color_control_layout layout,
									float cellSize, const char* name,
									BMessage* message = NULL,
									bool useOffscreen = false);
								BColorControl(BMessage* archive);
	virtual						~BColorControl();

	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				SetLayout(BLayout* layout);

	virtual	void				SetValue(int32 color_value);
			void				SetValue(rgb_color color);
			rgb_color			ValueAsColor();

	virtual	void				SetEnabled(bool state);

	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual	void				SetCellSize(float size);
			float				CellSize() const;
	virtual	void				SetLayout(color_control_layout layout);
			color_control_layout Layout() const;

	virtual	void				WindowActivated(bool state);
	virtual	void				MouseUp(BPoint point);
	virtual	void				MouseMoved(BPoint point, uint32 code,
									const BMessage* dragMessage);
	virtual	void				DetachedFromWindow();
	virtual	void				GetPreferredSize(float* _width,
									float* _height);
	virtual	void				ResizeToPreferred();
	virtual	status_t			Invoke(BMessage* message = NULL);
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);

	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* data);

	virtual	void				MakeFocus(bool state = true);
	virtual	void				AllAttached();
	virtual	void				AllDetached();

private:
	virtual	status_t			Perform(perform_code d, void *arg);
		// this can be made public again if needed

	virtual	void				_ReservedColorControl1();
	virtual	void				_ReservedColorControl2();
	virtual	void				_ReservedColorControl3();
	virtual	void				_ReservedColorControl4();

			BColorControl&		operator=(const BColorControl &other);

			void				_InitData(color_control_layout layout,
									float size, bool useOffscreen,
									BMessage* archive = NULL);
			void				_LayoutView();
			void				_InitOffscreen();
			void				_DrawColorArea(BView* target, BRect update);
			void				_DrawSelectors(BView* target);		
			void				_ColorRamp(BRect rect, BView* target,
									rgb_color baseColor, rgb_color compColor, 
									int16 flag, bool focused, BRect update);
			BPoint				_SelectorPosition(const BRect& rampRect,
									uint8 shade) const;
			BRect				_PaletteSelectorFrame(uint8 colorIndex) const;		
			BRect				_RampFrame(uint8 rampIndex) const;
		
	private:
			BRect				fPaletteFrame;
			int16				fSelectedPaletteColorIndex;
			int16				fPreviousSelectedPaletteColorIndex;

			float				fCellSize;
			int32				fRows;
			int32				fColumns;
			bool				fPaletteMode;
			bool				_unused[3];

			BTextControl*		fRedText;
			BTextControl*		fGreenText;
			BTextControl*		fBlueText;

			BBitmap*			fBitmap;
			BView*				fOffscreenView;

			int32				fFocusedComponent;	
			uint32				_reserved[2];	
};

inline void
BColorControl::SetValue(rgb_color color)
{
	int32 c = (color.red << 24) + (color.green << 16) + (color.blue << 8);
	SetValue(c);
}

#endif // _COLOR_CONTROL_H
