/*******************************************************************************
/
/	File:			ColorControl.h
/
/   Description:    BColorControl displays a palette of selectable colors.
/
/	Copyright 1996-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/


#ifndef _COLOR_CONTROL_H
#define _COLOR_CONTROL_H

#include <BeBuild.h>
#include <Control.h>

class BBitmap;

/*------------------------------------------------------------*/
/*----- layout options for the color control -----------------*/

enum color_control_layout {
	B_CELLS_4x64 = 4,
	B_CELLS_8x32 = 8,
	B_CELLS_16x16 = 16,
	B_CELLS_32x8 = 32,
	B_CELLS_64x4 = 64
};

class BTextControl;

/*----------------------------------------------------------------*/
/*----- BColorControl class --------------------------------------*/

class BColorControl : public BControl {
public:
						BColorControl(	BPoint start,
										color_control_layout layout,
										float cell_size,
										const char *name,
										BMessage *message = NULL,
										bool use_offscreen = false);
virtual					~BColorControl();

						BColorControl(BMessage *data);
static	BArchivable		*Instantiate(BMessage *data);
virtual	status_t		Archive(BMessage *data, bool deep = true) const;

virtual	void			SetValue(int32 color_value);
		void			SetValue(rgb_color color);
		rgb_color		ValueAsColor();

virtual	void			SetEnabled(bool state);

virtual	void			AttachedToWindow();
virtual	void			MessageReceived(BMessage *msg);
virtual	void			Draw(BRect updateRect);
virtual	void			MouseDown(BPoint where);
virtual	void			KeyDown(const char *bytes, int32 numBytes);

virtual	void			SetCellSize(float size);
		float			CellSize() const;
virtual	void			SetLayout(color_control_layout layout);
		color_control_layout Layout() const;

virtual void			WindowActivated(bool state);
virtual	void			MouseUp(BPoint pt);
virtual	void			MouseMoved(BPoint pt, uint32 code, const BMessage *msg);
virtual	void			DetachedFromWindow();
virtual void			GetPreferredSize(float *width, float *height);
virtual void			ResizeToPreferred();
virtual	status_t		Invoke(BMessage *msg = NULL);
virtual	void			FrameMoved(BPoint new_position);
virtual	void			FrameResized(float new_width, float new_height);

virtual BHandler		*ResolveSpecifier(BMessage *msg,
										int32 index,
										BMessage *specifier,
										int32 form,
										const char *property);
virtual status_t		GetSupportedSuites(BMessage *data);

virtual void			MakeFocus(bool state = true);
virtual void			AllAttached();
virtual void			AllDetached();


/*----- Private or reserved -----------------------------------------*/
virtual status_t		Perform(perform_code d, void *arg);

private:

virtual	void			_ReservedColorControl1();
virtual	void			_ReservedColorControl2();
virtual	void			_ReservedColorControl3();
virtual	void			_ReservedColorControl4();

		BColorControl	&operator=(const BColorControl &);

		void			LayoutView(bool calc_frame);
		void			UpdateOffscreen();
		void			UpdateOffscreen(BRect update);
		void			DrawColorArea(BView *target, BRect update);
		void			ColorRamp(	BRect r,
									BRect where,
									BView *target,
									rgb_color c,
									int16 flag,
									bool focused);
		void			KbAdjustColor(uint32 key);
		bool			key_down32(uint32 key);
		bool			key_down8(uint32 key);
static	BRect			CalcFrame(	BPoint start,
									color_control_layout layout,
									int32 size);
		void			InitData(	color_control_layout layout,
									float size,
									bool use_offscreen,
									BMessage *data = NULL);
		void			DoMouseMoved(BPoint pt);
		void			DoMouseUp(BPoint pt);

		float			fCellSize;
		int32			fRows;
		int32			fColumns;

		struct track_state {
			int32		orig_color;
			int32		cur_color;
			int32		prev_color;
			int32		bar_index;
			BRect		active_area;
			BRect		r;
			rgb_color	rgb;
			color_space	cspace;
		};

		BTextControl	*fRedText;
		BTextControl	*fGreenText;
		BTextControl	*fBlueText;
		BBitmap			*fBitmap;
		BView			*fOffscreenView;
		color_space		fLastMode;
		float			fRound;
		int32			fFocusedComponent;
		int32			fCachedIndex;
		uint32			_reserved[3];
		track_state		*fTState;
		bool			fUnused;	// fTracking;
		bool			fFocused;
		bool			fRetainCache;
		bool			fFastSet;
};

/*------------------------------------------------------------*/
/*----- inline functions -------------------------------------*/

inline void BColorControl::SetValue(rgb_color color)
{
	/* OK, no private parts */
	int32 c = (color.red << 24) + (color.green << 16) + (color.blue << 8);
	SetValue(c);
}

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _COLOR_CONTROL_H */
