/*
 * Copyright 2001-2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CONTROL_H
#define _CONTROL_H

#include <Invoker.h>
#include <Message.h>	// For convenience
#include <View.h>


enum {
	B_CONTROL_OFF = 0,
	B_CONTROL_ON = 1,
	B_CONTROL_PARTIALLY_ON = 2
};

class BBitmap;
class BWindow;


class BControl : public BView, public BInvoker {
public:
			// Values for [Set]IconBitmap(). Not all types are applicable for
			// all controls.
			enum {
				B_OFF_BITMAP					= 0x00,
				B_ON_BITMAP						= 0x01,
				B_PARTIALLY_ON_BITMAP			= 0x02,

				// flag, can be combined with any of the above
				B_DISABLED_BITMAP				= 0x80,
					// disabled version of the specified bitmap
			};

			// flags for SetIconBitmap()
			enum {
				B_KEEP_BITMAP					= 0x0001,
					// transfer bitmap ownership to BControl object
			};

			// flags for SetIcon()
			enum {
				B_TRIM_BITMAP					= 0x0100,
					// crop the bitmap to the not fully transparent area, may
					// change the icon size
				B_TRIM_BITMAP_KEEP_ASPECT		= 0x0200,
					// like B_TRIM_BITMAP, but keeps the aspect ratio
				B_CREATE_ON_BITMAP				= 0x0400,
				B_CREATE_PARTIALLY_ON_BITMAP	= 0x0800,
				B_CREATE_DISABLED_BITMAPS		= 0x1000,
			};

public:
								BControl(BRect frame, const char* name,
									const char* label, BMessage* message,
									uint32 resizingMode, uint32 flags);
								BControl(const char* name, const char* label,
									BMessage* message, uint32 flags);
	virtual						~BControl();

								BControl(BMessage* archive);
	static	BArchivable*		Instantiate(BMessage* archive);
	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;

	virtual	void				WindowActivated(bool active);

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				AllAttached();
	virtual	void				AllDetached();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MakeFocus(bool focused = true);

	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MouseDown(BPoint point);
	virtual	void				MouseUp(BPoint point);
	virtual	void				MouseMoved(BPoint point, uint32 transit,
									const BMessage *message);

	virtual	void				SetLabel(const char* string);
			const char*			Label() const;

	virtual	void				SetValue(int32 value);
			int32				Value() const;

	virtual	void				SetEnabled(bool enabled);
			bool				IsEnabled() const;

	virtual	void				GetPreferredSize(float* _width,
									float* _height);
	virtual	void				ResizeToPreferred();

	virtual	status_t			Invoke(BMessage* message = NULL);
	virtual	BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 what, const char* property);
	virtual	status_t			GetSupportedSuites(BMessage* message);

	virtual	status_t			Perform(perform_code d, void* arg);

	virtual	status_t			SetIcon(const BBitmap* bitmap,
									uint32 flags = 0);
			status_t			SetIconBitmap(const BBitmap* bitmap,
									uint32 which, uint32 flags = 0);
			const BBitmap*		IconBitmap(uint32 which) const;

protected:
			bool				IsFocusChanging() const;
			bool				IsTracking() const;
			void				SetTracking(bool state);

			void				SetValueNoUpdate(int32 value);

private:
			struct IconData;

private:
	static	BBitmap*			_ConvertToRGB32(const BBitmap* bitmap,
									bool noAppServerLink = false);
	static	status_t			_TrimBitmap(const BBitmap* bitmap,
									bool keepAspect, BBitmap*& _trimmedBitmap);
			status_t			_MakeBitmaps(const BBitmap* bitmap,
									uint32 flags);

	virtual	void				_ReservedControl2();
	virtual	void				_ReservedControl3();
	virtual	void				_ReservedControl4();

			BControl&			operator=(const BControl&);

			void				InitData(BMessage* data = NULL);

private:
			char*				fLabel;
			int32				fValue;
			bool				fEnabled;
			bool				fFocusChanging;
			bool				fTracking;
			bool				fWantsNav;
			IconData*			fIconData;

#ifdef B_HAIKU_64_BIT
			uint32				_reserved[2];
#else
			uint32				_reserved[3];
#endif
};

#endif // _CONTROL_H
