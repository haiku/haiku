/*
 * Copyright 2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ICON_VIEW_H
#define ICON_VIEW_H


#include <Entry.h>
#include <Messenger.h>
#include <Mime.h>
#include <String.h>
#include <View.h>


class Icon {
	public:
		Icon();
		Icon(const Icon& source);
		~Icon();

		void SetTo(BAppFileInfo& info, const char* type = NULL);
		void SetTo(entry_ref& ref, const char* type = NULL);
		status_t CopyTo(BAppFileInfo& info, const char* type = NULL, bool force = false);
		status_t CopyTo(entry_ref& ref, const char* type = NULL, bool force = false);
		status_t CopyTo(BMessage& message);

		void SetData(const uint8* data, size_t size);
		void SetLarge(const BBitmap* large);
		void SetMini(const BBitmap* large);
		void Unset();

		bool HasData() const;
		status_t GetData(icon_size which, BBitmap** _bitmap) const;
		status_t GetData(uint8** _data, size_t* _size) const;

		status_t GetIcon(BBitmap* bitmap) const;

		Icon& operator=(const Icon& source);

		void AdoptLarge(BBitmap *large);
		void AdoptMini(BBitmap *mini);
		void AdoptData(uint8* data, size_t size);

		static BBitmap* AllocateBitmap(int32 size, int32 space = -1);

	private:
		BBitmap*	fLarge;
		BBitmap*	fMini;
		uint8*		fData;
		size_t		fSize;
};

class IconView : public BView {
	public:
		IconView(BRect rect, const char* name,
			uint32 resizeMode = B_FOLLOW_LEFT | B_FOLLOW_TOP,
			uint32 flags = B_NAVIGABLE);
		virtual ~IconView();

		virtual void AttachedToWindow();
		virtual void DetachedFromWindow();
		virtual void MessageReceived(BMessage* message);
		virtual void Draw(BRect updateRect);
		virtual void GetPreferredSize(float* _width, float* _height);

		virtual void MouseDown(BPoint where);
		virtual void MouseUp(BPoint where);
		virtual void MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage);
		virtual void KeyDown(const char* bytes, int32 numBytes);

		virtual void MakeFocus(bool focus = true);

		void SetTo(entry_ref& file, const char* fileType = NULL);
		void SetTo(::Icon* icon);
		void SetIconSize(int32 size);
		void ShowIconHeap(bool show);
		void SetEnabled(bool enabled);
		void SetTarget(const BMessenger& target);
		void Update();
		void Invoke();

		::Icon* Icon();

	private:
		bool _AcceptsDrag(const BMessage* message);
		BRect _BitmapRect() const;
		void _AddOrEditIcon();
		void _SetIcon(BBitmap* large, BBitmap* mini, const uint8* data, size_t size,
			bool force = false);
		void _SetIcon(entry_ref* ref);
		void _RemoveIcon();
		void _DeleteIcons();
		void _StartWatching();
		void _StopWatching();

		BMessenger	fTarget;
		int32		fIconSize;
		BBitmap*	fIcon;
		BBitmap*	fHeapIcon;

		::Icon*		fIconData;

		bool		fHasRef;
		bool		fIsLive;
		entry_ref	fRef;
		BString		fFileType;
		BPoint		fDragPoint;
		bool		fTracking;
		bool		fDragging;
		bool		fDropTarget;
		bool		fEnabled;
};

static const uint32 kMsgIconInvoked = 'iciv';
static const uint32 kMsgRemoveIcon = 'icrm';
static const uint32 kMsgAddIcon = 'icad';
static const uint32 kMsgEditIcon = 'iced';

enum icon_source {
	kNoIcon = 0,
	kOwnIcon,
	kApplicationIcon,
	kSupertypeIcon
};

extern status_t icon_for_type(BMimeType& type, BBitmap& bitmap,
	icon_size size, icon_source* _source = NULL);

#endif	// ICON_VIEW_H
