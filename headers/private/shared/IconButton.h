/*
 * Copyright 2006-2011, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef ICON_BUTTON_H
#define ICON_BUTTON_H


//! GUI class that loads an image from disk and shows it as clickable button.


#include <Control.h>
#include <String.h>


class BBitmap;
class BMimeType;


namespace BPrivate {


class BIconButton : public BControl {
public:
								BIconButton(const char* name,
									const char* label = NULL,
									BMessage* message = NULL,
									BHandler* target = NULL);
	virtual						~BIconButton();

	// BView interface
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();

	virtual	void				Draw(BRect updateRect);
	virtual	bool				ShouldDrawBorder() const;
	virtual	void				DrawBorder(BRect& frame,
									const BRect& updateRect,
									const rgb_color& backgroundColor,
									uint32 controlLookFlags);
	virtual	void				DrawBackground(BRect& frame,
									const BRect& updateRect,
									const rgb_color& backgroundColor,
									uint32 controlLookFlags);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* message);
	virtual	void				GetPreferredSize(float* width,
												 float* height);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();


	// BInvoker interface
	virtual	status_t			Invoke(BMessage* message = NULL);

	// BControl interface
	virtual	void				SetValue(int32 value);
	virtual	void				SetEnabled(bool enable);

	// BIconButton
			bool				IsValid() const;

			void				SetPressed(bool pressed);
			bool				IsPressed() const;

			status_t			SetIcon(int32 resourceID);
			status_t			SetIcon(const char* pathToBitmap);
			status_t			SetIcon(const BBitmap* bitmap);
			status_t			SetIcon(const BMimeType* fileType,
									bool small = true);
			status_t			SetIcon(const unsigned char* bitsFromQuickRes,
									uint32 width, uint32 height,
									color_space format,
									bool convertToBW = false);
			void				ClearIcon();
			void				TrimIcon(bool keepAspect = true);

			BBitmap*			Bitmap() const;
									// caller has to delete the returned bitmap

protected:
			bool				IsInside() const;
			void				SetInside(bool inside);

private:
			BBitmap*			_ConvertToRGB32(const BBitmap* bitmap) const;
			status_t			_MakeBitmaps(const BBitmap* bitmap);
			void				_DeleteBitmaps();
			void				_SendMessage() const;
			void				_Update();
			void				_SetTracking(bool state);
			void				_SetFlags(uint32 flags, bool set);
			bool				_HasFlags(uint32 flags) const;

private:
			uint32				fButtonState;
			BBitmap*			fNormalBitmap;
			BBitmap*			fDisabledBitmap;
			BBitmap*			fClickedBitmap;
			BBitmap*			fDisabledClickedBitmap;

			BHandler*			fTargetCache;
};


} // namespac BPrivate


using BPrivate::BIconButton;


#endif // ICON_BUTTON_H
