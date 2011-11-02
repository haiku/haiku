/*
 * Copyright 2006-2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_LAYOUT_H
#define	_LAYOUT_H


#include <Alignment.h>
#include <Archivable.h>
#include <LayoutItem.h>
#include <List.h>
#include <Size.h>


class BLayoutContext;
class BLayoutItem;
class BView;


class BLayout : public BLayoutItem {
public:
								BLayout();
								BLayout(BMessage* archive);
	virtual						~BLayout();

			BView*				Owner() const;
			BView*				TargetView() const;
	virtual	BView*				View(); // from BLayoutItem

	// methods dealing with items
	virtual	BLayoutItem*		AddView(BView* child);
	virtual	BLayoutItem*		AddView(int32 index, BView* child);

	virtual	bool				AddItem(BLayoutItem* item);
	virtual	bool				AddItem(int32 index, BLayoutItem* item);

	virtual	bool				RemoveView(BView* child);
	virtual	bool				RemoveItem(BLayoutItem* item);
	virtual	BLayoutItem*		RemoveItem(int32 index);

			BLayoutItem*		ItemAt(int32 index) const;
			int32				CountItems() const;
			int32				IndexOfItem(const BLayoutItem* item) const;
			int32				IndexOfView(BView* child) const;

			bool				AncestorsVisible() const;

	// Layouting related methods

	virtual	void				InvalidateLayout(bool children = false);
	virtual	void				Relayout(bool immediate = false);
									// from BLayoutItem
			void				RequireLayout();
			bool				IsValid();
			void				EnableLayoutInvalidation();
			void				DisableLayoutInvalidation();

			void				LayoutItems(bool force = false);
			BRect				LayoutArea();
			BLayoutContext*		LayoutContext() const;

	// Archiving methods

	virtual status_t			Archive(BMessage* into, bool deep = true) const;
	virtual	status_t			AllUnarchived(const BMessage* from);

	virtual status_t			ItemArchived(BMessage* into, BLayoutItem* item,
									int32 index) const;
	virtual	status_t			ItemUnarchived(const BMessage* from,
									BLayoutItem* item, int32 index);

protected:
	// BLayout hook methods
	virtual	bool				ItemAdded(BLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(BLayoutItem* item, int32 fromIndex);
	virtual	void				LayoutInvalidated(bool children);
	virtual	void				DoLayout() = 0;
	virtual	void				OwnerChanged(BView* was);

	// BLayoutItem hook methods
	virtual	void				AttachedToLayout();
	virtual void				DetachedFromLayout(BLayout* layout);
	virtual	void				AncestorVisibilityChanged(bool shown);

	// To be called by sub-classes in SetVisible().
			void				VisibilityChanged(bool show);
	// To be called when layout data is known to be good
			void				ResetLayoutInvalidation();

	virtual status_t			Perform(perform_code d, void* arg);

private:

	// FBC padding
	virtual	void				_ReservedLayout1();
	virtual	void				_ReservedLayout2();
	virtual	void				_ReservedLayout3();
	virtual	void				_ReservedLayout4();
	virtual	void				_ReservedLayout5();
	virtual	void				_ReservedLayout6();
	virtual	void				_ReservedLayout7();
	virtual	void				_ReservedLayout8();
	virtual	void				_ReservedLayout9();
	virtual	void				_ReservedLayout10();

			friend class BView;

			void				SetOwner(BView* owner);
			void				SetTarget(BView* target);

			void				_LayoutWithinContext(bool force,
									BLayoutContext* context);

			uint32				fState;
			bool				fAncestorsVisible;
			int32				fInvalidationDisabled;
			BLayoutContext*		fContext;
			BView*				fOwner;
			BView*				fTarget;
			BList				fItems;
			BList				fNestedLayouts;

			uint32				_reserved[10];
};


#endif	//	_LAYOUT_H
