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

namespace BPrivate {
	class BIcon;
};


class BControl : public BView, public BInvoker {
public:
								BControl(BRect frame, const char* name,
									const char* label, BMessage* message,
									uint32 resizingMode, uint32 flags);
								BControl(const char* name, const char* label,
									BMessage* message, uint32 flags);
	virtual						~BControl();

								BControl(BMessage* data);
	static	BArchivable*		Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data, bool deep = true) const;

	virtual	void				WindowActivated(bool active);

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				AllAttached();
	virtual	void				AllDetached();

	virtual	void				MessageReceived(BMessage* message);
	virtual	void				MakeFocus(bool focus = true);

	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 code,
									const BMessage* dragMessage);

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
			BPrivate::BIcon*	fIcon;

#ifdef B_HAIKU_64_BIT
			uint32				_reserved[2];
#else
			uint32				_reserved[3];
#endif
};

#endif // _CONTROL_H
