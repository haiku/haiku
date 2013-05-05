/*
 * Copyright 2010, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef	_ABSTRACT_LAYOUT_H
#define	_ABSTRACT_LAYOUT_H

#include <Alignment.h>
#include <Layout.h>
#include <Size.h>

class BAbstractLayout : public BLayout {
public:
								BAbstractLayout();
								BAbstractLayout(BMessage* from);
	virtual						~BAbstractLayout();

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
	virtual	BAlignment			Alignment();

	virtual	void				SetExplicitMinSize(BSize size);
	virtual	void				SetExplicitMaxSize(BSize size);
	virtual	void				SetExplicitPreferredSize(BSize size);
	virtual	void				SetExplicitAlignment(BAlignment alignment);

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

	virtual BRect				Frame();
	virtual	void				SetFrame(BRect frame);	

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual status_t			Archive(BMessage* into, bool deep = true) const;

	virtual	status_t			Perform(perform_code d, void* arg);

protected:
	// Archiving hook methods
	virtual	status_t 			AllArchived(BMessage* archive) const;
	virtual	status_t			AllUnarchived(const BMessage* from);

	virtual status_t			ItemArchived(BMessage* into, BLayoutItem* item,
									int32 index) const;
	virtual	status_t			ItemUnarchived(const BMessage* from,
									BLayoutItem* item, int32 index);

	virtual	bool				ItemAdded(BLayoutItem* item, int32 atIndex);
	virtual	void				ItemRemoved(BLayoutItem* item, int32 fromIndex);
	virtual	void				LayoutInvalidated(bool children);
	virtual	void				OwnerChanged(BView* was);

	// BLayoutItem hook methods
	virtual	void				AttachedToLayout();
	virtual void				DetachedFromLayout(BLayout* layout);
	virtual	void				AncestorVisibilityChanged(bool shown);

private:
	virtual	void				_ReservedAbstractLayout1();
	virtual	void				_ReservedAbstractLayout2();
	virtual	void				_ReservedAbstractLayout3();
	virtual	void				_ReservedAbstractLayout4();
	virtual	void				_ReservedAbstractLayout5();
	virtual	void				_ReservedAbstractLayout6();
	virtual	void				_ReservedAbstractLayout7();
	virtual	void				_ReservedAbstractLayout8();
	virtual	void				_ReservedAbstractLayout9();
	virtual	void				_ReservedAbstractLayout10();

	// forbidden methods
								BAbstractLayout(const BAbstractLayout&);
			void				operator =(const BAbstractLayout&);

			struct	Proxy;
			struct	ViewProxy;
			struct	DataProxy;

			Proxy*				fExplicitData;
			uint32				_reserved[4];
};

#endif	//	_ABSTRACT_LAYOUT_ITEM_H
