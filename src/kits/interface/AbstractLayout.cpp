/*
 * Copyright 2010, Haiku, Inc.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include <AbstractLayout.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <View.h>
#include <ViewPrivate.h>


namespace {
	const char* const kSizesField = "BAbstractLayout:sizes";
		// kSizesField == {min, max, preferred}
	const char* const kAlignmentField = "BAbstractLayout:alignment";
	const char* const kFrameField = "BAbstractLayout:frame";
	const char* const kVisibleField = "BAbstractLayout:visible";

	enum proxy_type { VIEW_PROXY_TYPE, DATA_PROXY_TYPE }; 
}


struct BAbstractLayout::Proxy {

	Proxy(proxy_type type)
		:
		type(type)
	{
	}

	virtual	BSize		MinSize() const = 0;
	virtual void		SetMinSize(const BSize&) = 0;

	virtual	BSize		MaxSize() const = 0;
	virtual void		SetMaxSize(const BSize&) = 0;

	virtual	BSize		PreferredSize() const = 0;
	virtual void		SetPreferredSize(const BSize&) = 0;

	virtual	BAlignment	Alignment() const = 0;
	virtual	void		SetAlignment(const BAlignment&) = 0;

	virtual	BRect		Frame() const = 0;
	virtual	void		SetFrame(const BRect& frame) = 0;

	virtual bool		IsVisible(bool ancestorHidden) const = 0;
	virtual	void		SetVisible(bool visible) = 0;
	
	virtual	status_t	AddDataToArchive(BMessage* archive,
							bool ancestorHidden) = 0;
	virtual	status_t	RestoreDataFromArchive(const BMessage* archive) = 0;

			proxy_type	type;
};


struct BAbstractLayout::DataProxy : Proxy {

	DataProxy()
		:
		Proxy(DATA_PROXY_TYPE),
		minSize(),
		maxSize(),
		preferredSize(),
		alignment(),
		frame(-1, -1, 0, 0),
		visible(true)
	{
	}

	BSize MinSize() const
	{
		return minSize;
	}

	void SetMinSize(const BSize& min)
	{
		minSize = min;
	}

	BSize MaxSize() const
	{
		return maxSize;
	}

	void SetMaxSize(const BSize& max)
	{
		maxSize = max;
	}

	BSize PreferredSize() const
	{
		return preferredSize;
	}

	void SetPreferredSize(const BSize& preferred)
	{
		preferredSize = preferred;
	}

	BAlignment Alignment() const
	{
		return this->alignment;
	}

	void SetAlignment(const BAlignment& align)
	{
		this->alignment = align;
	}

	BRect Frame() const
	{
		return frame;
	}

	void SetFrame(const BRect& frame)
	{
		if (frame == this->frame)
			return;
		this->frame = frame;
	}

	bool IsVisible(bool) const
	{
		return visible;
	}

	void SetVisible(bool visible)
	{
		this->visible = visible;
	}

	status_t AddDataToArchive(BMessage* archive, bool ancestorHidden)
	{
		status_t err = archive->AddSize(kSizesField, minSize);
		if (err == B_OK)
			err = archive->AddSize(kSizesField, maxSize);
		if (err == B_OK)
			err = archive->AddSize(kSizesField, preferredSize);
		if (err == B_OK)
			err = archive->AddAlignment(kAlignmentField, alignment);
		if (err == B_OK)
			err = archive->AddRect(kFrameField, frame);
		if (err == B_OK)
			err = archive->AddBool(kVisibleField, visible);

		return err;
	}

	status_t RestoreDataFromArchive(const BMessage* archive)
	{
		status_t err = archive->FindSize(kSizesField, 0, &minSize);
		if (err == B_OK)
			err = archive->FindSize(kSizesField, 1, &maxSize);
		if (err == B_OK)
			err = archive->FindSize(kSizesField, 2, &preferredSize);
		if (err == B_OK)
			err = archive->FindAlignment(kAlignmentField, &alignment);
		if (err == B_OK)
			err = archive->FindRect(kFrameField, &frame);
		if (err == B_OK)
			err = archive->FindBool(kVisibleField, &visible);

		return err;
	}

	BSize		minSize;
	BSize		maxSize;
	BSize		preferredSize;
	BAlignment	alignment;
	BRect		frame;
	bool		visible;
};


struct BAbstractLayout::ViewProxy : Proxy {
	ViewProxy(BView* target)
		:
		Proxy(VIEW_PROXY_TYPE),
		view(target)
	{
	}

	BSize MinSize() const
	{
		return view->ExplicitMinSize();
	}

	void SetMinSize(const BSize& min)
	{
		view->SetExplicitMinSize(min);
	}

	BSize MaxSize() const
	{
		return view->ExplicitMaxSize();
	}

	void SetMaxSize(const BSize& min)
	{
		view->SetExplicitMaxSize(min);
	}

	BSize PreferredSize() const
	{
		return view->ExplicitPreferredSize();
	}

	void SetPreferredSize(const BSize& preferred)
	{
		view->SetExplicitPreferredSize(preferred);
	}

	BAlignment Alignment() const
	{
		return view->ExplicitAlignment();
	}

	void SetAlignment(const BAlignment& alignment)
	{
		view->SetExplicitAlignment(alignment);
	}

	BRect Frame() const
	{
		return view->Frame();
	}

	void SetFrame(const BRect& frame)
	{
		view->MoveTo(frame.LeftTop());
		view->ResizeTo(frame.Width(), frame.Height());
	}

	bool IsVisible(bool ancestorsVisible) const
	{
		int16 showLevel = BView::Private(view).ShowLevel();
		return (showLevel - (ancestorsVisible) ? 0 : 1) <= 0;
	}

	void SetVisible(bool visible)
	{
		// No need to check that we are not re-hiding, that is done
		// for us.
		if (visible)
			view->Show();
		else
			view->Hide();
	}

	status_t AddDataToArchive(BMessage* archive, bool ancestorHidden)
	{
		return B_OK;
	}

	status_t RestoreDataFromArchive(const BMessage* archive)
	{
		return B_OK;
	}

	BView*	view;
};


BAbstractLayout::BAbstractLayout()
	:
	fExplicitData(new BAbstractLayout::DataProxy())
{
}


BAbstractLayout::BAbstractLayout(BMessage* from)
	:
	BLayout(BUnarchiver::PrepareArchive(from)),
	fExplicitData(new DataProxy())
{
	BUnarchiver(from).Finish();
}


BAbstractLayout::~BAbstractLayout()
{
	delete fExplicitData;
}


BSize
BAbstractLayout::MinSize()
{
	return BLayoutUtils::ComposeSize(fExplicitData->MinSize(), BaseMinSize());
}


BSize
BAbstractLayout::MaxSize()
{
	return BLayoutUtils::ComposeSize(fExplicitData->MaxSize(), BaseMaxSize());
}


BSize
BAbstractLayout::PreferredSize()
{
	return BLayoutUtils::ComposeSize(fExplicitData->PreferredSize(),
		BasePreferredSize());
}


BAlignment
BAbstractLayout::Alignment()
{
	return BLayoutUtils::ComposeAlignment(fExplicitData->Alignment(),
		BaseAlignment());
}


void
BAbstractLayout::SetExplicitMinSize(BSize size)
{
	fExplicitData->SetMinSize(size);
}


void
BAbstractLayout::SetExplicitMaxSize(BSize size)
{
	fExplicitData->SetMaxSize(size);
}


void
BAbstractLayout::SetExplicitPreferredSize(BSize size)
{
	fExplicitData->SetPreferredSize(size);
}


void
BAbstractLayout::SetExplicitAlignment(BAlignment alignment)
{
	fExplicitData->SetAlignment(alignment);
}


BSize
BAbstractLayout::BaseMinSize()
{
	return BSize(0, 0);
}


BSize
BAbstractLayout::BaseMaxSize()
{
	return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
}


BSize
BAbstractLayout::BasePreferredSize()
{
	return BSize(0, 0);
}


BAlignment
BAbstractLayout::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}


BRect
BAbstractLayout::Frame()
{
	return fExplicitData->Frame();
}


void
BAbstractLayout::SetFrame(BRect frame)
{
	if (frame != fExplicitData->Frame()) {
		fExplicitData->SetFrame(frame);
		if (!Owner())
			Relayout();
	}
}


bool
BAbstractLayout::IsVisible()
{
	return fExplicitData->IsVisible(AncestorsVisible());
}


void
BAbstractLayout::SetVisible(bool visible)
{
	if (visible != fExplicitData->IsVisible(AncestorsVisible())) {
		fExplicitData->SetVisible(visible);
		if (Layout())
			Layout()->InvalidateLayout(false);
		VisibilityChanged(visible);
	}
}


status_t
BAbstractLayout::Archive(BMessage* into, bool deep) const
{
	BArchiver archiver(into);
	status_t err = BLayout::Archive(into, deep);

	return archiver.Finish(err);
}


status_t
BAbstractLayout::AllUnarchived(const BMessage* from)
{
	status_t err = fExplicitData->RestoreDataFromArchive(from);
	if (err != B_OK)
		return err;
		
	return BLayout::AllUnarchived(from);
}


void
BAbstractLayout::OwnerChanged(BView* was)
{
	if (was) {
		static_cast<ViewProxy*>(fExplicitData)->view = Owner();
		return;
	}

	delete fExplicitData;
	fExplicitData = new ViewProxy(Owner());
}


void
BAbstractLayout::AncestorVisibilityChanged(bool shown)
{
	if (AncestorsVisible() == shown)
		return;

	if (BView* owner = Owner()) {
		if (shown)
			owner->Show();
		else
			owner->Hide();
	}
	BLayout::AncestorVisibilityChanged(shown);
}


status_t
BAbstractLayout::Perform(perform_code code, void* _data)
{
	return BLayout::Perform(code, _data);
}

