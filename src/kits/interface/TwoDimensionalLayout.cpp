/*
 * Copyright 2006-2010, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <TwoDimensionalLayout.h>

#include <stdio.h>

#include <ControlLook.h>
#include <LayoutContext.h>
#include <LayoutItem.h>
#include <LayoutUtils.h>
#include <List.h>
#include <Message.h>
#include <View.h>

#include <Referenceable.h>

#include "CollapsingLayouter.h"




// Some words of explanation:
//
// This class is the base class for BLayouts that organize their items
// on a grid, with each item covering one or more grid cells (always a
// rectangular area). The derived classes only need to implement the
// hooks reporting the constraints for the items and additional constraints
// for the rows and columns. This class does all the layouting.
//
// The basic idea of the layout process is simple. The horizontal and the
// vertical dimensions are laid out independently and the items are set to the
// resulting locations and sizes. The "height for width" feature makes the
// height depend on the width, which makes things a bit more complicated.
// The horizontal dimension must be laid out first and and the results are
// fed into the vertical layout process.
//
// The AlignLayoutWith() feature, which allows to align layouts for different
// views with each other, causes the need for the three inner *Layouter classes.
// For each set of layouts aligned with each other with respect to one
// dimension that dimension must be laid out together. The class responsible
// is CompoundLayouter; one instance exists per such set. The derived class
// VerticalCompoundLayouter is a specialization for the vertical dimension
// which additionally takes care of the "height for width" feature. Per layout
// a single LocalLayouter exists, which comprises the required glue layout
// code and serves as a proxy for the layout, providing service methods
// needed by the CompoundLayouter.

// TODO: Check for memory leaks!

//#define DEBUG_LAYOUT

// CompoundLayouter
class BTwoDimensionalLayout::CompoundLayouter : public BReferenceable {
public:
								CompoundLayouter(enum orientation orientation);
	virtual						~CompoundLayouter();

			orientation			Orientation();

	virtual	Layouter*			GetLayouter(bool minMax);

			LayoutInfo*			GetLayoutInfo();

			void				AddLocalLayouter(LocalLayouter* localLayouter);
			void				RemoveLocalLayouter(
									LocalLayouter* localLayouter);

			status_t			AddAlignedLayoutsToArchive(BArchiver* archiver,
									LocalLayouter* requestedBy);

			void				AbsorbCompoundLayouter(CompoundLayouter* other);

	virtual	void				InvalidateLayout();
			bool				IsMinMaxValid();
			void				ValidateMinMax();
			void				Layout(float size, LocalLayouter* localLayouter,
									BLayoutContext* context);

protected:
	virtual	void				DoLayout(float size,
									LocalLayouter* localLayouter,
									BLayoutContext* context);

			Layouter*			fLayouter;
			LayoutInfo*			fLayoutInfo;
			orientation			fOrientation;
			BList				fLocalLayouters;
			BLayoutContext*		fLayoutContext;
			float				fLastLayoutSize;

			void				_PrepareItems();

			int32				_CountElements();
			bool				_HasMultiElementItems();

			void				_AddConstraints(Layouter* layouter);

			float				_Spacing();
};

// VerticalCompoundLayouter
class BTwoDimensionalLayout::VerticalCompoundLayouter
	: public CompoundLayouter, private BLayoutContextListener {
public:
								VerticalCompoundLayouter();

	virtual	Layouter*			GetLayouter(bool minMax);

	virtual	void				InvalidateLayout();

			void				InvalidateHeightForWidth();

			void				InternalGetHeightForWidth(
									LocalLayouter* localLayouter,
									BLayoutContext* context,
									bool realLayout, float* minHeight,
									float* maxHeight, float* preferredHeight);

protected:
	virtual	void				DoLayout(float size,
									LocalLayouter* localLayouter,
									BLayoutContext* context);

private:
			Layouter*			fHeightForWidthLayouter;
			float				fCachedMinHeightForWidth;
			float				fCachedMaxHeightForWidth;
			float				fCachedPreferredHeightForWidth;
			BLayoutContext*		fHeightForWidthLayoutContext;

			bool				_HasHeightForWidth();

			bool				_SetHeightForWidthLayoutContext(
									BLayoutContext* context);

	// BLayoutContextListener
	virtual	void				LayoutContextLeft(BLayoutContext* context);
};

// LocalLayouter
class BTwoDimensionalLayout::LocalLayouter : private BLayoutContextListener {
public:
								LocalLayouter(BTwoDimensionalLayout* layout);
								~LocalLayouter();

	// interface for the BTwoDimensionalLayout class

			BSize				MinSize();
			BSize				MaxSize();
			BSize				PreferredSize();

			void				InvalidateLayout();
			void				Layout(BSize size);

			BRect				ItemFrame(Dimensions itemDimensions);

			void				ValidateMinMax();

			void				DoHorizontalLayout(float width);

			void				InternalGetHeightForWidth(float width,
									float* minHeight, float* maxHeight,
									float* preferredHeight);

			void				AlignWith(LocalLayouter* other,
									enum orientation orientation);

	// Archiving stuff
			status_t			AddAlignedLayoutsToArchive(BArchiver* archiver);
			status_t			AddOwnerToArchive(BArchiver* archiver,
									CompoundLayouter* requestedBy,
									bool& _wasAvailable);
			status_t			AlignLayoutsFromArchive(BUnarchiver* unarchiver,
									orientation posture);


	// interface for the compound layout context

			void				PrepareItems(
									CompoundLayouter* compoundLayouter);
			int32				CountElements(
									CompoundLayouter* compoundLayouter);
			bool				HasMultiElementItems(
									CompoundLayouter* compoundLayouter);

			void				AddConstraints(
									CompoundLayouter* compoundLayouter,
									Layouter* layouter);

			float				Spacing(CompoundLayouter* compoundLayouter);

			bool				HasHeightForWidth();

			bool				AddHeightForWidthConstraints(
									VerticalCompoundLayouter* compoundLayouter,
									Layouter* layouter,
									BLayoutContext* context);
			void				SetHeightForWidthConstraintsAdded(bool added);

			void				SetCompoundLayouter(
									CompoundLayouter* compoundLayouter,
									enum orientation orientation);

			void				InternalInvalidateLayout(
									CompoundLayouter* compoundLayouter);

	// implementation private
private:
			BTwoDimensionalLayout* fLayout;
			CompoundLayouter*	fHLayouter;
			VerticalCompoundLayouter* fVLayouter;
			BList				fHeightForWidthItems;

	// active layout context when doing last horizontal layout
			BLayoutContext*		fHorizontalLayoutContext;
			float				fHorizontalLayoutWidth;
			bool				fHeightForWidthConstraintsAdded;

			void				_SetHorizontalLayoutContext(
									BLayoutContext* context, float width);

	// BLayoutContextListener
	virtual	void				LayoutContextLeft(BLayoutContext* context);
};


// #pragma mark -

// archiving constants
namespace {
	const char* const kHAlignedLayoutField = "BTwoDimensionalLayout:"
		"halignedlayout";
	const char* const kVAlignedLayoutField = "BTwoDimensionalLayout:"
		"valignedlayout";
	const char* const kInsetsField = "BTwoDimensionalLayout:insets";
	const char* const kSpacingField = "BTwoDimensionalLayout:spacing";
		// kSpacingField = {fHSpacing, fVSpacing}
}


BTwoDimensionalLayout::BTwoDimensionalLayout()
	:
	fLeftInset(0),
	fRightInset(0),
	fTopInset(0),
	fBottomInset(0),
	fHSpacing(0),
	fVSpacing(0),
	fLocalLayouter(new LocalLayouter(this))
{
}


BTwoDimensionalLayout::BTwoDimensionalLayout(BMessage* from)
	:
	BAbstractLayout(from),
	fLeftInset(0),
	fRightInset(0),
	fTopInset(0),
	fBottomInset(0),
	fHSpacing(0),
	fVSpacing(0),
	fLocalLayouter(new LocalLayouter(this))
{
	BRect insets;
	from->FindRect(kInsetsField, &insets);
	SetInsets(insets.left, insets.top, insets.right, insets.bottom);

	from->FindFloat(kSpacingField, 0, &fHSpacing);
	from->FindFloat(kSpacingField, 1, &fVSpacing);
}


BTwoDimensionalLayout::~BTwoDimensionalLayout()
{
	delete fLocalLayouter;
}


void
BTwoDimensionalLayout::SetInsets(float left, float top, float right,
	float bottom)
{
	fLeftInset = BControlLook::ComposeSpacing(left);
	fTopInset = BControlLook::ComposeSpacing(top);
	fRightInset = BControlLook::ComposeSpacing(right);
	fBottomInset = BControlLook::ComposeSpacing(bottom);

	InvalidateLayout();
}


void
BTwoDimensionalLayout::GetInsets(float* left, float* top, float* right,
	float* bottom) const
{
	if (left)
		*left = fLeftInset;
	if (top)
		*top = fTopInset;
	if (right)
		*right = fRightInset;
	if (bottom)
		*bottom = fBottomInset;
}


void
BTwoDimensionalLayout::AlignLayoutWith(BTwoDimensionalLayout* other,
	enum orientation orientation)
{
	if (!other || other == this)
		return;

	fLocalLayouter->AlignWith(other->fLocalLayouter, orientation);

	InvalidateLayout();
}


BSize
BTwoDimensionalLayout::BaseMinSize()
{
	_ValidateMinMax();
	return AddInsets(fLocalLayouter->MinSize());
}


BSize
BTwoDimensionalLayout::BaseMaxSize()
{
	_ValidateMinMax();
	return AddInsets(fLocalLayouter->MaxSize());
}


BSize
BTwoDimensionalLayout::BasePreferredSize()
{
	_ValidateMinMax();
	return AddInsets(fLocalLayouter->PreferredSize());
}


BAlignment
BTwoDimensionalLayout::BaseAlignment()
{
	return BAbstractLayout::BaseAlignment();
}


bool
BTwoDimensionalLayout::HasHeightForWidth()
{
	_ValidateMinMax();
	return fLocalLayouter->HasHeightForWidth();
}


void
BTwoDimensionalLayout::GetHeightForWidth(float width, float* min, float* max,
	float* preferred)
{
	if (!HasHeightForWidth())
		return;

	float outerSpacing = fLeftInset + fRightInset - 1;
	fLocalLayouter->InternalGetHeightForWidth(BLayoutUtils::SubtractDistances(
		width, outerSpacing), min, max, preferred);
	AddInsets(min, max, preferred);
}


void
BTwoDimensionalLayout::SetFrame(BRect frame)
{
	BAbstractLayout::SetFrame(frame);
}


void
BTwoDimensionalLayout::InvalidateLayout(bool children)
{
	BLayout::InvalidateLayout(children);
	fLocalLayouter->InvalidateLayout();
}


status_t
BTwoDimensionalLayout::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BAbstractLayout::Archive(into, deep);

	if (err == B_OK) {
		BRect insets(fLeftInset, fTopInset, fRightInset, fBottomInset);
		err = into->AddRect(kInsetsField, insets);
	}

	if (err == B_OK)
		err = into->AddFloat(kSpacingField, fHSpacing);

	if (err == B_OK)
		err = into->AddFloat(kSpacingField, fVSpacing);

	return archiver.Finish(err);
}


status_t
BTwoDimensionalLayout::AllArchived(BMessage* into) const
{
	BArchiver archiver(into);

	status_t err = BLayout::AllArchived(into);
	if (err == B_OK)
		err = fLocalLayouter->AddAlignedLayoutsToArchive(&archiver);
	return err;
}


status_t
BTwoDimensionalLayout::AllUnarchived(const BMessage* from)
{
	status_t err = BLayout::AllUnarchived(from);
	if (err != B_OK)
		return err;

	BUnarchiver unarchiver(from);
	err = fLocalLayouter->AlignLayoutsFromArchive(&unarchiver, B_HORIZONTAL);
	if (err == B_OK)
		err = fLocalLayouter->AlignLayoutsFromArchive(&unarchiver, B_VERTICAL);

	return err;
}


void
BTwoDimensionalLayout::DerivedLayoutItems()
{
	_ValidateMinMax();

	// layout the horizontal/vertical elements
	BSize size(SubtractInsets(LayoutArea().Size()));

#ifdef DEBUG_LAYOUT
printf("BTwoDimensionalLayout::DerivedLayoutItems(): view: %p"
	" size: (%.1f, %.1f)\n", View(), size.Width(), size.Height());
#endif

	fLocalLayouter->Layout(size);

	// layout the items
	BPoint itemOffset(LayoutArea().LeftTop());
	int itemCount = CountItems();
	for (int i = 0; i < itemCount; i++) {
		BLayoutItem* item = ItemAt(i);
		if (item->IsVisible()) {
			Dimensions itemDimensions;
			GetItemDimensions(item, &itemDimensions);
			BRect frame = fLocalLayouter->ItemFrame(itemDimensions);
			frame.left += fLeftInset;
			frame.top += fTopInset;
			frame.right += fLeftInset;
			frame.bottom += fTopInset;
			frame.OffsetBy(itemOffset);
{
#ifdef DEBUG_LAYOUT
printf("  frame for item %2d (view: %p): ", i, item->View());
frame.PrintToStream();
#endif
//BSize min(item->MinSize());
//BSize max(item->MaxSize());
//printf("    min: (%.1f, %.1f), max: (%.1f, %.1f)\n", min.width, min.height,
//	max.width, max.height);
//if (item->HasHeightForWidth()) {
//float minHeight, maxHeight, preferredHeight;
//item->GetHeightForWidth(frame.Width(), &minHeight, &maxHeight,
//	&preferredHeight);
//printf("    hfw: min: %.1f, max: %.1f, pref: %.1f\n", minHeight, maxHeight,
//	preferredHeight);
//}
}

			item->AlignInFrame(frame);
		}
//else
//printf("  item %2d not visible", i);
	}
}


BSize
BTwoDimensionalLayout::AddInsets(BSize size)
{
	size.width = BLayoutUtils::AddDistances(size.width,
		fLeftInset + fRightInset - 1);
	size.height = BLayoutUtils::AddDistances(size.height,
		fTopInset + fBottomInset - 1);
	return size;
}


void
BTwoDimensionalLayout::AddInsets(float* minHeight, float* maxHeight,
	float* preferredHeight)
{
	float insets = fTopInset + fBottomInset - 1;
	if (minHeight)
		*minHeight = BLayoutUtils::AddDistances(*minHeight, insets);
	if (maxHeight)
		*maxHeight = BLayoutUtils::AddDistances(*maxHeight, insets);
	if (preferredHeight)
		*preferredHeight = BLayoutUtils::AddDistances(*preferredHeight, insets);
}


BSize
BTwoDimensionalLayout::SubtractInsets(BSize size)
{
	size.width = BLayoutUtils::SubtractDistances(size.width,
		fLeftInset + fRightInset - 1);
	size.height = BLayoutUtils::SubtractDistances(size.height,
		fTopInset + fBottomInset - 1);
	return size;
}


void
BTwoDimensionalLayout::PrepareItems(enum orientation orientation)
{
}


bool
BTwoDimensionalLayout::HasMultiColumnItems()
{
	return false;
}


bool
BTwoDimensionalLayout::HasMultiRowItems()
{
	return false;
}


void
BTwoDimensionalLayout::_ValidateMinMax()
{
	fLocalLayouter->ValidateMinMax();
}


// #pragma mark - CompoundLayouter


BTwoDimensionalLayout::CompoundLayouter::CompoundLayouter(
	enum orientation orientation)
	:
	fLayouter(NULL),
	fLayoutInfo(NULL),
	fOrientation(orientation),
	fLocalLayouters(10),
	fLayoutContext(NULL),
	fLastLayoutSize(-1)
{
}


BTwoDimensionalLayout::CompoundLayouter::~CompoundLayouter()
{
}


orientation
BTwoDimensionalLayout::CompoundLayouter::Orientation()
{
	return fOrientation;
}


Layouter*
BTwoDimensionalLayout::CompoundLayouter::GetLayouter(bool minMax)
{
	return fLayouter;
}


LayoutInfo*
BTwoDimensionalLayout::CompoundLayouter::GetLayoutInfo()
{
	return fLayoutInfo;
}


void
BTwoDimensionalLayout::CompoundLayouter::AddLocalLayouter(
	LocalLayouter* localLayouter)
{
	if (localLayouter) {
		if (!fLocalLayouters.HasItem(localLayouter)) {
			fLocalLayouters.AddItem(localLayouter);
			InvalidateLayout();
		}
	}
}


void
BTwoDimensionalLayout::CompoundLayouter::RemoveLocalLayouter(
	LocalLayouter* localLayouter)
{
	if (fLocalLayouters.RemoveItem(localLayouter))
		InvalidateLayout();
}


status_t
BTwoDimensionalLayout::CompoundLayouter::AddAlignedLayoutsToArchive(
	BArchiver* archiver, LocalLayouter* requestedBy)
{
	// The LocalLayouter* that really owns us is at index 0, layouts
	// at other indices are aligned to this one.
	if (requestedBy != fLocalLayouters.ItemAt(0))
		return B_OK;

	status_t err;
	for (int32 i = fLocalLayouters.CountItems() - 1; i > 0; i--) {
		LocalLayouter* layouter = (LocalLayouter*)fLocalLayouters.ItemAt(i);

		bool wasAvailable;
		err = layouter->AddOwnerToArchive(archiver, this, wasAvailable);
		if (err != B_OK && wasAvailable)
			return err;
	}
	return B_OK;
}


void
BTwoDimensionalLayout::CompoundLayouter::AbsorbCompoundLayouter(
	CompoundLayouter* other)
{
	if (other == this)
		return;

	int32 count = other->fLocalLayouters.CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		LocalLayouter* layouter
			= (LocalLayouter*)other->fLocalLayouters.ItemAt(i);
		AddLocalLayouter(layouter);
		layouter->SetCompoundLayouter(this, fOrientation);
	}

	InvalidateLayout();
}


void
BTwoDimensionalLayout::CompoundLayouter::InvalidateLayout()
{
	if (!fLayouter)
		return;

	delete fLayouter;
	delete fLayoutInfo;

	fLayouter = NULL;
	fLayoutInfo = NULL;
	fLayoutContext = NULL;

	// notify all local layouters to invalidate the respective views
	int32 count = fLocalLayouters.CountItems();
	for (int32 i = 0; i < count; i++) {
		LocalLayouter* layouter = (LocalLayouter*)fLocalLayouters.ItemAt(i);
		layouter->InternalInvalidateLayout(this);
	}
}


bool
BTwoDimensionalLayout::CompoundLayouter::IsMinMaxValid()
{
	return (fLayouter != NULL);
}


void
BTwoDimensionalLayout::CompoundLayouter::ValidateMinMax()
{
	if (IsMinMaxValid())
		return;

	fLastLayoutSize = -1;

	// create the layouter
	_PrepareItems();

	int elementCount = _CountElements();

	fLayouter = new CollapsingLayouter(elementCount, _Spacing());

	// tell the layouter about our constraints
	// TODO: We should probably ignore local layouters whose view is hidden.
	// It's a bit tricky to find out, whether the view is hidden, though, since
	// this doesn't necessarily mean only hidden relative to the parent, but
	// hidden relative to a common parent.
	_AddConstraints(fLayouter);

	fLayoutInfo = fLayouter->CreateLayoutInfo();
}


void
BTwoDimensionalLayout::CompoundLayouter::Layout(float size,
	LocalLayouter* localLayouter, BLayoutContext* context)
{
	ValidateMinMax();

	if (context != fLayoutContext || fLastLayoutSize != size) {
		DoLayout(size, localLayouter, context);
		fLayoutContext = context;
		fLastLayoutSize = size;
	}
}


void
BTwoDimensionalLayout::CompoundLayouter::DoLayout(float size,
	LocalLayouter* localLayouter, BLayoutContext* context)
{
	fLayouter->Layout(fLayoutInfo, size);
}


void
BTwoDimensionalLayout::CompoundLayouter::_PrepareItems()
{
	int32 count = fLocalLayouters.CountItems();
	for (int32 i = 0; i < count; i++) {
		LocalLayouter* layouter = (LocalLayouter*)fLocalLayouters.ItemAt(i);
		layouter->PrepareItems(this);
	}
}


int32
BTwoDimensionalLayout::CompoundLayouter::_CountElements()
{
	int32 elementCount = 0;
	int32 count = fLocalLayouters.CountItems();
	for (int32 i = 0; i < count; i++) {
		LocalLayouter* layouter = (LocalLayouter*)fLocalLayouters.ItemAt(i);
		int32 layouterCount = layouter->CountElements(this);
		elementCount = max_c(elementCount, layouterCount);
	}

	return elementCount;
}


bool
BTwoDimensionalLayout::CompoundLayouter::_HasMultiElementItems()
{
	int32 count = fLocalLayouters.CountItems();
	for (int32 i = 0; i < count; i++) {
		LocalLayouter* layouter = (LocalLayouter*)fLocalLayouters.ItemAt(i);
		if (layouter->HasMultiElementItems(this))
			return true;
	}

	return false;
}


void
BTwoDimensionalLayout::CompoundLayouter::_AddConstraints(Layouter* layouter)
{
	int32 count = fLocalLayouters.CountItems();
	for (int32 i = 0; i < count; i++) {
		LocalLayouter* localLayouter = (LocalLayouter*)fLocalLayouters.ItemAt(i);
		localLayouter->AddConstraints(this, layouter);
	}
}


float
BTwoDimensionalLayout::CompoundLayouter::_Spacing()
{
	if (!fLocalLayouters.IsEmpty())
		return ((LocalLayouter*)fLocalLayouters.ItemAt(0))->Spacing(this);
	return 0;
}


// #pragma mark - VerticalCompoundLayouter


BTwoDimensionalLayout::VerticalCompoundLayouter::VerticalCompoundLayouter()
	:
	CompoundLayouter(B_VERTICAL),
	fHeightForWidthLayouter(NULL),
	fCachedMinHeightForWidth(0),
	fCachedMaxHeightForWidth(0),
	fCachedPreferredHeightForWidth(0),
	fHeightForWidthLayoutContext(NULL)
{
}


Layouter*
BTwoDimensionalLayout::VerticalCompoundLayouter::GetLayouter(bool minMax)
{
	return (minMax || !_HasHeightForWidth()
		? fLayouter : fHeightForWidthLayouter);
}


void
BTwoDimensionalLayout::VerticalCompoundLayouter::InvalidateLayout()
{
	CompoundLayouter::InvalidateLayout();

	InvalidateHeightForWidth();
}


void
BTwoDimensionalLayout::VerticalCompoundLayouter::InvalidateHeightForWidth()
{
	if (fHeightForWidthLayouter != NULL) {
		delete fHeightForWidthLayouter;
		fHeightForWidthLayouter = NULL;

		// also make sure we're not reusing the old layout info
		fLastLayoutSize = -1;

		int32 count = fLocalLayouters.CountItems();
		for (int32 i = 0; i < count; i++) {
			LocalLayouter* layouter = (LocalLayouter*)fLocalLayouters.ItemAt(i);
			layouter->SetHeightForWidthConstraintsAdded(false);
		}
	}
}


void
BTwoDimensionalLayout::VerticalCompoundLayouter::InternalGetHeightForWidth(
	LocalLayouter* localLayouter, BLayoutContext* context, bool realLayout,
	float* minHeight, float* maxHeight, float* preferredHeight)
{
	bool updateCachedInfo = false;

	if (_SetHeightForWidthLayoutContext(context)
		|| fHeightForWidthLayouter == NULL) {
		// Either the layout context changed or we haven't initialized the
		// height for width layouter yet. We create it and init it now.

		// clone the vertical layouter
		delete fHeightForWidthLayouter;
		delete fLayoutInfo;
		fHeightForWidthLayouter = fLayouter->CloneLayouter();
		fLayoutInfo = fHeightForWidthLayouter->CreateLayoutInfo();

		// add the children's height for width constraints
		int32 count = fLocalLayouters.CountItems();
		for (int32 i = 0; i < count; i++) {
			LocalLayouter* layouter = (LocalLayouter*)fLocalLayouters.ItemAt(i);
			if (layouter->HasHeightForWidth()) {
				layouter->AddHeightForWidthConstraints(this,
					fHeightForWidthLayouter, context);
			}
		}

		updateCachedInfo = true;

		// get the height for width info
		fCachedMinHeightForWidth = fHeightForWidthLayouter->MinSize();
		fCachedMaxHeightForWidth = fHeightForWidthLayouter->MaxSize();
		fCachedPreferredHeightForWidth
			= fHeightForWidthLayouter->PreferredSize();

	} else if (localLayouter->HasHeightForWidth()) {
		// There is a height for width layouter and it has been initialized
		// in the current layout context. So we just add the height for width
		// constraints of the calling local layouter, if they haven't been
		// added yet.
		updateCachedInfo = localLayouter->AddHeightForWidthConstraints(this,
			fHeightForWidthLayouter, context);
	}

	// update cached height for width info, if something changed
	if (updateCachedInfo) {
		// get the height for width info
		fCachedMinHeightForWidth = fHeightForWidthLayouter->MinSize();
		fCachedMaxHeightForWidth = fHeightForWidthLayouter->MaxSize();
		fCachedPreferredHeightForWidth
			= fHeightForWidthLayouter->PreferredSize();
	}

	if (minHeight)
		*minHeight = fCachedMinHeightForWidth;
	if (maxHeight)
		*maxHeight = fCachedMaxHeightForWidth;
	if (preferredHeight)
		*preferredHeight = fCachedPreferredHeightForWidth;
}


void
BTwoDimensionalLayout::VerticalCompoundLayouter::DoLayout(float size,
	LocalLayouter* localLayouter, BLayoutContext* context)
{
	Layouter* layouter;
	if (_HasHeightForWidth()) {
		float minHeight, maxHeight, preferredHeight;
		InternalGetHeightForWidth(localLayouter, context, true, &minHeight,
			&maxHeight, &preferredHeight);
		size = max_c(size, minHeight);
		layouter = fHeightForWidthLayouter;
	} else
		layouter = fLayouter;

	layouter->Layout(fLayoutInfo, size);
}


bool
BTwoDimensionalLayout::VerticalCompoundLayouter::_HasHeightForWidth()
{
	int32 count = fLocalLayouters.CountItems();
	for (int32 i = 0; i < count; i++) {
		LocalLayouter* layouter = (LocalLayouter*)fLocalLayouters.ItemAt(i);
		if (layouter->HasHeightForWidth())
			return true;
	}

	return false;
}


bool
BTwoDimensionalLayout::VerticalCompoundLayouter
	::_SetHeightForWidthLayoutContext(BLayoutContext* context)
{
	if (context == fHeightForWidthLayoutContext)
		return false;

	if (fHeightForWidthLayoutContext != NULL) {
		fHeightForWidthLayoutContext->RemoveListener(this);
		fHeightForWidthLayoutContext = NULL;
	}

	// We can ignore the whole context business, if we have no more than one
	// local layouter. We use the layout context only to recognize when calls
	// of different local layouters belong to the same context.
	if (fLocalLayouters.CountItems() <= 1)
		return false;

	fHeightForWidthLayoutContext = context;

	if (fHeightForWidthLayoutContext != NULL)
		fHeightForWidthLayoutContext->AddListener(this);

	InvalidateHeightForWidth();

	return true;
}


void
BTwoDimensionalLayout::VerticalCompoundLayouter::LayoutContextLeft(
	BLayoutContext* context)
{
	fHeightForWidthLayoutContext = NULL;
}


// #pragma mark - LocalLayouter


BTwoDimensionalLayout::LocalLayouter::LocalLayouter(
		BTwoDimensionalLayout* layout)
	:
	fLayout(layout),
	fHLayouter(new CompoundLayouter(B_HORIZONTAL)),
	fVLayouter(new VerticalCompoundLayouter),
	fHeightForWidthItems(),
	fHorizontalLayoutContext(NULL),
	fHorizontalLayoutWidth(0),
	fHeightForWidthConstraintsAdded(false)
{
	fHLayouter->AddLocalLayouter(this);
	fVLayouter->AddLocalLayouter(this);
}


BTwoDimensionalLayout::LocalLayouter::~LocalLayouter()
{
	if (fHLayouter != NULL) {
		fHLayouter->RemoveLocalLayouter(this);
		fHLayouter->ReleaseReference();
	}

	if (fVLayouter != NULL) {
		fVLayouter->RemoveLocalLayouter(this);
		fVLayouter->ReleaseReference();
	}
}


BSize
BTwoDimensionalLayout::LocalLayouter::MinSize()
{
	return BSize(fHLayouter->GetLayouter(true)->MinSize(),
		fVLayouter->GetLayouter(true)->MinSize());
}


BSize
BTwoDimensionalLayout::LocalLayouter::MaxSize()
{
	return BSize(fHLayouter->GetLayouter(true)->MaxSize(),
		fVLayouter->GetLayouter(true)->MaxSize());
}


BSize
BTwoDimensionalLayout::LocalLayouter::PreferredSize()
{
	return BSize(fHLayouter->GetLayouter(true)->PreferredSize(),
		fVLayouter->GetLayouter(true)->PreferredSize());
}


void
BTwoDimensionalLayout::LocalLayouter::InvalidateLayout()
{
	fHLayouter->InvalidateLayout();
	fVLayouter->InvalidateLayout();
}


void
BTwoDimensionalLayout::LocalLayouter::Layout(BSize size)
{
	DoHorizontalLayout(size.width);
	fVLayouter->Layout(size.height, this, fLayout->LayoutContext());
}


BRect
BTwoDimensionalLayout::LocalLayouter::ItemFrame(Dimensions itemDimensions)
{
	LayoutInfo* hLayoutInfo = fHLayouter->GetLayoutInfo();
	LayoutInfo* vLayoutInfo = fVLayouter->GetLayoutInfo();
	float x = hLayoutInfo->ElementLocation(itemDimensions.x);
	float y = vLayoutInfo->ElementLocation(itemDimensions.y);
	float width = hLayoutInfo->ElementRangeSize(itemDimensions.x,
		itemDimensions.width);
	float height = vLayoutInfo->ElementRangeSize(itemDimensions.y,
		itemDimensions.height);
	return BRect(x, y, x + width, y + height);
}


void
BTwoDimensionalLayout::LocalLayouter::ValidateMinMax()
{
	if (fHLayouter->IsMinMaxValid() && fVLayouter->IsMinMaxValid())
		return;

	if (!fHLayouter->IsMinMaxValid())
		fHeightForWidthItems.MakeEmpty();

	_SetHorizontalLayoutContext(NULL, -1);

	fHLayouter->ValidateMinMax();
	fVLayouter->ValidateMinMax();
	fLayout->ResetLayoutInvalidation();
}


void
BTwoDimensionalLayout::LocalLayouter::DoHorizontalLayout(float width)
{
	BLayoutContext* context = fLayout->LayoutContext();
	if (fHorizontalLayoutContext != context
			|| width != fHorizontalLayoutWidth) {
		_SetHorizontalLayoutContext(context, width);
		fHLayouter->Layout(width, this, context);
		fVLayouter->InvalidateHeightForWidth();
	}
}


void
BTwoDimensionalLayout::LocalLayouter::InternalGetHeightForWidth(float width,
	float* minHeight, float* maxHeight, float* preferredHeight)
{
	DoHorizontalLayout(width);
	fVLayouter->InternalGetHeightForWidth(this, fHorizontalLayoutContext, false,
		minHeight, maxHeight, preferredHeight);
}


void
BTwoDimensionalLayout::LocalLayouter::AlignWith(LocalLayouter* other,
	enum orientation orientation)
{
	if (orientation == B_HORIZONTAL)
		other->fHLayouter->AbsorbCompoundLayouter(fHLayouter);
	else
		other->fVLayouter->AbsorbCompoundLayouter(fVLayouter);
}


status_t
BTwoDimensionalLayout::LocalLayouter::AddAlignedLayoutsToArchive(
	BArchiver* archiver)
{
	status_t err = fHLayouter->AddAlignedLayoutsToArchive(archiver, this);

	if (err == B_OK)
		err = fVLayouter->AddAlignedLayoutsToArchive(archiver, this);

	return err;
}


status_t
BTwoDimensionalLayout::LocalLayouter::AddOwnerToArchive(BArchiver* archiver,
	CompoundLayouter* requestedBy, bool& _wasAvailable)
{
	const char* field = kHAlignedLayoutField;
	if (requestedBy == fVLayouter)
		field = kVAlignedLayoutField;

	if ((_wasAvailable = archiver->IsArchived(fLayout)))
		return archiver->AddArchivable(field, fLayout);

	return B_NAME_NOT_FOUND;
}


status_t
BTwoDimensionalLayout::LocalLayouter::AlignLayoutsFromArchive(
	BUnarchiver* unarchiver, orientation posture)
{
	const char* field = kHAlignedLayoutField;
	if (posture == B_VERTICAL)
		field = kVAlignedLayoutField;

	int32 count;
	status_t err = unarchiver->ArchiveMessage()->GetInfo(field, NULL, &count);
	if (err == B_NAME_NOT_FOUND)
		return B_OK;

	BTwoDimensionalLayout* retriever;
	for (int32 i = 0; i < count && err == B_OK; i++) {
		err = unarchiver->FindObject(field, i,
			BUnarchiver::B_DONT_ASSUME_OWNERSHIP, retriever);

		if (err == B_OK)
			retriever->AlignLayoutWith(fLayout, posture);
	}

	return err;
}


void
BTwoDimensionalLayout::LocalLayouter::PrepareItems(
	CompoundLayouter* compoundLayouter)
{
	fLayout->PrepareItems(compoundLayouter->Orientation());
}


int32
BTwoDimensionalLayout::LocalLayouter::CountElements(
	CompoundLayouter* compoundLayouter)
{
	if (compoundLayouter->Orientation() == B_HORIZONTAL)
		return fLayout->InternalCountColumns();
	else
		return fLayout->InternalCountRows();
}


bool
BTwoDimensionalLayout::LocalLayouter::HasMultiElementItems(
	CompoundLayouter* compoundLayouter)
{
	if (compoundLayouter->Orientation() == B_HORIZONTAL)
		return fLayout->HasMultiColumnItems();
	else
		return fLayout->HasMultiRowItems();
}


void
BTwoDimensionalLayout::LocalLayouter::AddConstraints(
	CompoundLayouter* compoundLayouter, Layouter* layouter)
{
	enum orientation orientation = compoundLayouter->Orientation();
	int itemCount = fLayout->CountItems();
	if (itemCount > 0) {
		for (int i = 0; i < itemCount; i++) {
			BLayoutItem* item = fLayout->ItemAt(i);
			if (item->IsVisible()) {
				Dimensions itemDimensions;
				fLayout->GetItemDimensions(item, &itemDimensions);

				BSize min = item->MinSize();
				BSize max = item->MaxSize();
				BSize preferred = item->PreferredSize();

				if (orientation == B_HORIZONTAL) {
					layouter->AddConstraints(
						itemDimensions.x,
						itemDimensions.width,
						min.width,
						max.width,
						preferred.width);

					if (item->HasHeightForWidth())
						fHeightForWidthItems.AddItem(item);

				} else {
					layouter->AddConstraints(
						itemDimensions.y,
						itemDimensions.height,
						min.height,
						max.height,
						preferred.height);
				}
			}
		}

		// add column/row constraints
		ColumnRowConstraints constraints;
		int elementCount = CountElements(compoundLayouter);
		for (int element = 0; element < elementCount; element++) {
			fLayout->GetColumnRowConstraints(orientation, element,
				&constraints);
			layouter->SetWeight(element, constraints.weight);
			layouter->AddConstraints(element, 1, constraints.min,
				constraints.max, constraints.min);
		}
	}
}


float
BTwoDimensionalLayout::LocalLayouter::Spacing(
	CompoundLayouter* compoundLayouter)
{
	return (compoundLayouter->Orientation() == B_HORIZONTAL
		? fLayout->fHSpacing : fLayout->fVSpacing);
}


bool
BTwoDimensionalLayout::LocalLayouter::HasHeightForWidth()
{
	return !fHeightForWidthItems.IsEmpty();
}


bool
BTwoDimensionalLayout::LocalLayouter::AddHeightForWidthConstraints(
	VerticalCompoundLayouter* compoundLayouter, Layouter* layouter,
	BLayoutContext* context)
{
	if (context != fHorizontalLayoutContext)
		return false;

	if (fHeightForWidthConstraintsAdded)
		return false;

	LayoutInfo* hLayoutInfo = fHLayouter->GetLayoutInfo();

	// add the children's height for width constraints
	int32 itemCount = fHeightForWidthItems.CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		BLayoutItem* item = (BLayoutItem*)fHeightForWidthItems.ItemAt(i);
		Dimensions itemDimensions;
		fLayout->GetItemDimensions(item, &itemDimensions);

		float minHeight, maxHeight, preferredHeight;
		item->GetHeightForWidth(
			hLayoutInfo->ElementRangeSize(itemDimensions.x,
				itemDimensions.width),
			&minHeight, &maxHeight, &preferredHeight);
		layouter->AddConstraints(
			itemDimensions.y,
			itemDimensions.height,
			minHeight,
			maxHeight,
			preferredHeight);
	}

	SetHeightForWidthConstraintsAdded(true);

	return true;
}


void
BTwoDimensionalLayout::LocalLayouter::SetHeightForWidthConstraintsAdded(
	bool added)
{
	fHeightForWidthConstraintsAdded = added;
}


void
BTwoDimensionalLayout::LocalLayouter::SetCompoundLayouter(
	CompoundLayouter* compoundLayouter, enum orientation orientation)
{
	CompoundLayouter* oldCompoundLayouter;
	if (orientation == B_HORIZONTAL) {
		oldCompoundLayouter = fHLayouter;
		fHLayouter = compoundLayouter;
	} else {
		oldCompoundLayouter = fVLayouter;
		fVLayouter = static_cast<VerticalCompoundLayouter*>(compoundLayouter);
	}

	if (compoundLayouter == oldCompoundLayouter)
		return;

	if (oldCompoundLayouter != NULL) {
		oldCompoundLayouter->RemoveLocalLayouter(this);
		oldCompoundLayouter->ReleaseReference();
	}

	if (compoundLayouter != NULL)
		compoundLayouter->AcquireReference();

	InternalInvalidateLayout(compoundLayouter);
}


void
BTwoDimensionalLayout::LocalLayouter::InternalInvalidateLayout(
	CompoundLayouter* compoundLayouter)
{
	_SetHorizontalLayoutContext(NULL, -1);

	fLayout->BLayout::InvalidateLayout();
}


void
BTwoDimensionalLayout::LocalLayouter::_SetHorizontalLayoutContext(
	BLayoutContext* context, float width)
{
	if (context != fHorizontalLayoutContext) {
		if (fHorizontalLayoutContext != NULL)
			fHorizontalLayoutContext->RemoveListener(this);

		fHorizontalLayoutContext = context;

		if (fHorizontalLayoutContext != NULL)
			fHorizontalLayoutContext->AddListener(this);
	}

	fHorizontalLayoutWidth = width;
}


void
BTwoDimensionalLayout::LocalLayouter::LayoutContextLeft(BLayoutContext* context)
{
	fHorizontalLayoutContext = NULL;
	fHorizontalLayoutWidth = -1;
}
