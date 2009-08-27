/*
 * Copyright 2007-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _POP_UP_MENU_H
#define _POP_UP_MENU_H


#include <Menu.h>


class BPopUpMenu : public BMenu {
public:
								BPopUpMenu(const char* title,
									bool radioMode = true,
									bool autoRename = true,
									menu_layout layout = B_ITEMS_IN_COLUMN);
								BPopUpMenu(BMessage* data);
	virtual						~BPopUpMenu();

	virtual	status_t			Archive(BMessage* archive,
									bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

			BMenuItem*			Go(BPoint where, bool autoInvoke = false,
									bool keepOpen = false, bool async = false);
			BMenuItem*			Go(BPoint where, bool autoInvoke,
									bool keepOpen, BRect openRect,
									bool async = false);

	virtual void				MessageReceived(BMessage* message);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);

	virtual BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual status_t			GetSupportedSuites(BMessage* data);

	virtual status_t			Perform(perform_code code, void* data);

	virtual void				ResizeToPreferred();
	virtual void				GetPreferredSize(float* _width,
									float* _height);
	virtual void				MakeFocus(bool state = true);
	virtual void				AllAttached();
	virtual void				AllDetached();

			void				SetAsyncAutoDestruct(bool state);
			bool				AsyncAutoDestruct() const;

protected:
	virtual	BPoint				ScreenLocation();

	virtual	void				_ReservedPopUpMenu1();
	virtual	void				_ReservedPopUpMenu2();
	virtual	void				_ReservedPopUpMenu3();

			BPopUpMenu&			operator=(const BPopUpMenu& other);

private:
			BMenuItem*			_Go(BPoint where, bool autoInvoke,
									bool startOpened, BRect* specialRect,
									bool async);
			BMenuItem*			_StartTrack(BPoint where, bool autoInvoke,
									bool startOpened, BRect* specialRect);
			BMenuItem*			_WaitMenu(void* data);

	static	int32				_thread_entry(void* data);

private:
			BPoint				fWhere;
			bool				fUseWhere;
			bool				fAutoDestruct;

			bool				_fUnusedBool1;
			bool				_fUnusedBool2;

			thread_id			fTrackThread;

			uint32				_reserved[3];
};

#endif // _POP_UP_MENU_H
