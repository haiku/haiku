/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DRAGGER_H
#define _DRAGGER_H


#include <Locker.h>
#include <List.h>
#include <View.h>

class BBitmap;
class BMessage;
class BPopUpMenu;
class BShelf;

namespace BPrivate {
	struct replicant_data;
	class ShelfContainerViewFilter;
};


class BDragger : public BView {
public:
								BDragger(BRect frame, BView* target,
									uint32 resizingMode = B_FOLLOW_NONE,
									uint32 flags = B_WILL_DRAW);
								BDragger(BView* target,
									uint32 flags = B_WILL_DRAW);
								BDragger(BMessage* data);
	virtual						~BDragger();

	static	BArchivable*		 Instantiate(BMessage* data);
	virtual	status_t			Archive(BMessage* data,
									bool deep = true) const;

	virtual void				AttachedToWindow();
	virtual void				DetachedFromWindow();

	virtual void				Draw(BRect updateRect);
	virtual void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual void				MessageReceived(BMessage* message);
	virtual	void				FrameMoved(BPoint newPosition);
	virtual	void				FrameResized(float newWidth, float newHeight);

	static	status_t			ShowAllDraggers();
	static	status_t			HideAllDraggers();
	static	bool				AreDraggersDrawn();

	virtual BHandler*			ResolveSpecifier(BMessage* message,
									int32 index, BMessage* specifier,
									int32 form, const char* property);
	virtual status_t			GetSupportedSuites(BMessage* data);
	virtual status_t			Perform(perform_code code, void* data);

	virtual void				ResizeToPreferred();
	virtual void				GetPreferredSize(float* _width,
									float* _height);
	virtual void				MakeFocus(bool focus = true);
	virtual void				AllAttached();
	virtual void				AllDetached();

			status_t			SetPopUp(BPopUpMenu* contextMenu);
			BPopUpMenu*			PopUp() const;

			bool				InShelf() const;
			BView*				Target() const;

	virtual	BBitmap*			DragBitmap(BPoint* offset, drawing_mode* mode);

	class Private;

protected:
			bool				IsVisibilityChanging() const;

private:
	friend class BPrivate::ShelfContainerViewFilter;
	friend struct BPrivate::replicant_data;
	friend class Private;
	friend class BShelf;

	virtual	void				_ReservedDragger2();
	virtual	void				_ReservedDragger3();
	virtual	void				_ReservedDragger4();

	static	void				_UpdateShowAllDraggers(bool visible);

			BDragger&			operator=(const BDragger& other);

			void				_InitData();
			void				_AddToList();
			void				_RemoveFromList();
			status_t			_DetermineRelationship();
			status_t			_SetViewToDrag(BView* target);
			void				_SetShelf(BShelf* shelf);
			void				_SetZombied(bool state);
			void				_BuildDefaultPopUp();
			void				_ShowPopUp(BView* target, BPoint where);

			enum relation {
				TARGET_UNKNOWN,
				TARGET_IS_CHILD,
				TARGET_IS_PARENT,
				TARGET_IS_SIBLING
			};

			BView*				fTarget;
			relation			fRelation;
			BShelf*				fShelf;
			bool				fTransition;
			bool				fIsZombie;
			char				fErrCount;
			bool				fPopUpIsCustom;
			BBitmap*			fBitmap;
			BPopUpMenu*			fPopUp;
			uint32				_reserved[3];
};

#endif /* _DRAGGER_H */
