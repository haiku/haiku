/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ICON_VIEW_H
#define ICON_VIEW_H


#include <Control.h>
#include <Entry.h>
#include <Messenger.h>
#include <Mime.h>
#include <String.h>


enum icon_source {
	kNoIcon = 0,
	kOwnIcon,
	kApplicationIcon,
	kSupertypeIcon
};


class Icon {
public:
								Icon();
								Icon(const Icon& source);
								~Icon();

			void				SetTo(const BAppFileInfo& info,
									const char* type = NULL);
			void				SetTo(const entry_ref& ref,
									const char* type = NULL);
			void				SetTo(const BMimeType& type,
									icon_source* _source = NULL);
			status_t			CopyTo(BAppFileInfo& info,
									const char* type = NULL,
									bool force = false) const;
			status_t			CopyTo(const entry_ref& ref,
									const char* type = NULL,
									bool force = false) const;
			status_t			CopyTo(BMimeType& type,
									bool force = false) const;
			status_t			CopyTo(BMessage& message) const;

			void				SetData(const uint8* data, size_t size);
			void				SetLarge(const BBitmap* large);
			void				SetMini(const BBitmap* large);
			void				Unset();

			bool				HasData() const;
			status_t			GetData(icon_size which,
									BBitmap** _bitmap) const;
			status_t			GetData(uint8** _data, size_t* _size) const;

			status_t			GetIcon(BBitmap* bitmap) const;

			Icon&				operator=(const Icon& source);

			void				AdoptLarge(BBitmap* large);
			void				AdoptMini(BBitmap* mini);
			void				AdoptData(uint8* data, size_t size);

	static	BBitmap*			AllocateBitmap(int32 size, int32 space = -1);

private:
			BBitmap*			fLarge;
			BBitmap*			fMini;
			uint8*				fData;
			size_t				fSize;
};


class BSize;


class IconView : public BControl {
public:
								IconView(const char* name,
									uint32 flags = B_NAVIGABLE);
	virtual						~IconView();

	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				Draw(BRect updateRect);
	virtual	void				GetPreferredSize(float* _width, float* _height);

	virtual	BSize				MaxSize();
	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual	void				MakeFocus(bool focus = true);

			void				SetTo(const entry_ref& file,
									const char* fileType = NULL);
			void				SetTo(const BMimeType& type);
			void				SetTo(::Icon* icon);
			void				Unset();
			void				Update();

			void				SetIconSize(int32 size);
			void				ShowIconHeap(bool show);
			void				ShowEmptyFrame(bool show);
			status_t			SetTarget(const BMessenger& target);
			void				SetModificationMessage(BMessage* message);
			status_t			Invoke(BMessage* message = NULL);

			::Icon*				Icon();
			int32				IconSize() const { return fIconSize; }
			icon_source			IconSource() const { return fSource; }
			status_t			GetRef(entry_ref& ref) const;
			status_t			GetMimeType(BMimeType& type) const;

#if __GNUC__ == 2
	virtual	status_t			SetTarget(BMessenger target);
	virtual	status_t			SetTarget(const BHandler* handler,
									const BLooper* looper = NULL);
#else
			using BControl::SetTarget;
#endif


protected:
	virtual	bool				AcceptsDrag(const BMessage* message);
	virtual	BRect				BitmapRect() const;

private:
			void				_AddOrEditIcon();
			void				_SetIcon(BBitmap* large, BBitmap* mini,
									const uint8* data, size_t size,
									bool force = false);
			void				_SetIcon(entry_ref* ref);
			void				_RemoveIcon();
			void				_DeleteIcons();
			void				_StartWatching();
			void				_StopWatching();

			BMessenger			fTarget;
			BMessage*			fModificationMessage;
			int32				fIconSize;
			BBitmap*			fIcon;
			BBitmap*			fHeapIcon;

			bool				fHasRef;
			bool				fHasType;
			entry_ref			fRef;
			BMimeType			fType;
			icon_source			fSource;
			::Icon*				fIconData;

			BPoint				fDragPoint;
			bool				fTracking;
			bool				fDragging;
			bool				fDropTarget;
			bool				fShowEmptyFrame;
};


static const uint32 kMsgIconInvoked	= 'iciv';
static const uint32 kMsgRemoveIcon	= 'icrm';
static const uint32 kMsgAddIcon		= 'icad';
static const uint32 kMsgEditIcon	= 'iced';


extern status_t icon_for_type(const BMimeType& type, uint8** _data,
	size_t* _size, icon_source* _source = NULL);
extern status_t icon_for_type(const BMimeType& type, BBitmap& bitmap,
	icon_size size, icon_source* _source = NULL);


#endif	// ICON_VIEW_H
