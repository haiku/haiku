/*
 * Copyright 2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <ToolTip.h>

#include <new>

#include <Message.h>
#include <TextView.h>
#include <ToolTipManager.h>


BToolTip::BToolTip()
{
	_InitData();
}


BToolTip::BToolTip(BMessage* archive)
{
	_InitData();

	bool sticky;
	if (archive->FindBool("sticky", &sticky) == B_OK)
		fIsSticky = sticky;

	// TODO!
}


BToolTip::~BToolTip()
{
}


status_t
BToolTip::Archive(BMessage* archive, bool deep) const
{
	status_t status = BArchivable::Archive(archive, deep);

	if (fIsSticky)
		status = archive->AddBool("sticky", fIsSticky);

	// TODO!
	return status;
}


void
BToolTip::SetSticky(bool enable)
{
	fIsSticky = enable;
}


bool
BToolTip::IsSticky() const
{
	return fIsSticky;
}


void
BToolTip::SetMouseRelativeLocation(BPoint location)
{
	fRelativeLocation = location;
}


BPoint
BToolTip::MouseRelativeLocation() const
{
	return fRelativeLocation;
}


void
BToolTip::SetAlignment(BAlignment alignment)
{
	fAlignment = alignment;
}


BAlignment
BToolTip::Alignment() const
{
	return fAlignment;
}


void
BToolTip::AttachedToWindow()
{
}


void
BToolTip::DetachedFromWindow()
{
}


bool
BToolTip::Lock()
{
	bool lockedLooper;
	while (true) {
		lockedLooper = View()->LockLooper();
		if (!lockedLooper) {
			BToolTipManager* manager = BToolTipManager::Manager();
			manager->Lock();

			if (View()->Window() != NULL) {
				manager->Unlock();
				continue;
			}
		}
		break;
	}

	fLockedLooper = lockedLooper;
	return true;
}


void
BToolTip::Unlock()
{
	if (fLockedLooper)
		View()->UnlockLooper();
	else
		BToolTipManager::Manager()->Unlock();
}


void
BToolTip::_InitData()
{
	fIsSticky = false;
	fRelativeLocation = BPoint(20, 20);
	fAlignment = BAlignment(B_ALIGN_RIGHT, B_ALIGN_BOTTOM);
}


//	#pragma mark -


BTextToolTip::BTextToolTip(const char* text)
{
	_InitData(text);
}


BTextToolTip::BTextToolTip(BMessage* archive)
{
	// TODO!
}


BTextToolTip::~BTextToolTip()
{
	delete fTextView;
}


/*static*/ BTextToolTip*
BTextToolTip::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "BTextToolTip"))
		return NULL;

	return new(std::nothrow) BTextToolTip(archive);
}


status_t
BTextToolTip::Archive(BMessage* archive, bool deep) const
{
	status_t status = BToolTip::Archive(archive, deep);
	// TODO!

	return status;
}


BView*
BTextToolTip::View() const
{
	return fTextView;
}


const char*
BTextToolTip::Text() const
{
	return fTextView->Text();
}


void
BTextToolTip::SetText(const char* text)
{
	if (!Lock())
		return;

	fTextView->SetText(text);
	fTextView->InvalidateLayout();

	Unlock();
}


void
BTextToolTip::_InitData(const char* text)
{
	fTextView = new BTextView("tool tip text");
	fTextView->SetText(text);
	fTextView->MakeEditable(false);
	fTextView->SetViewColor(ui_color(B_TOOL_TIP_BACKGROUND_COLOR));
	rgb_color color = ui_color(B_TOOL_TIP_TEXT_COLOR);
	fTextView->SetFontAndColor(NULL, 0, &color);
	fTextView->SetWordWrap(false);
}

