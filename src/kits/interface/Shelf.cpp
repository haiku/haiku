//------------------------------------------------------------------------------
//	Copyright (c) 2001-2004, Haiku
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Shelf.cpp
//	Author:			Marc Flerackers (mflerackers@androme.be)
//	Description:	BShelf stores replicant views that are dropped onto it
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Beep.h>
#include <Dragger.h>
#include <Entry.h>
#include <File.h>
#include <Looper.h>
#include <Message.h>
#include <MessageFilter.h>
#include <Messenger.h>
#include <Point.h>
#include <Rect.h>
#include <Shelf.h>
#include <View.h>

#include <ZombieReplicantView.h>

#include <stdio.h>

class _rep_data_ {
		_rep_data_(BMessage *message, BView *view, BDragger *dragger,
			BDragger::relation relation, unsigned long id, image_id image)
			:
			fMessage(message),
			fView(view),
			fDragger(NULL),
			fRelation(relation),
			fId(id),
			fImage(image),
			fError(B_OK),
			fView2(NULL)
		
		{	
		}

		_rep_data_()
			:
			fMessage(NULL),
			fView(NULL),
			fDragger(NULL),
			fRelation(BDragger::TARGET_UNKNOWN),
			fId(0),
			fError(B_ERROR),
			fView2(NULL)
		{	
		}

static	_rep_data_	*find(BList const *list, BMessage const *msg)
		{
			int32 i = 0;
			_rep_data_ *item;
			while ((item = (_rep_data_*)list->ItemAt(i++)) != NULL) {
				if (item->fMessage == msg)
					return item;
			}

			return NULL;
		}

static	_rep_data_	*find(BList const *list, BView const *view, bool aBool)
		{
			int32 i = 0;
			_rep_data_ *item;
			while ((item = (_rep_data_*)list->ItemAt(i++)) != NULL) {
				if (item->fView == view)
					return item;

				if (aBool && item->fView2 == view)
					return item;
			}

			return NULL;
		}

static	_rep_data_	*find(BList const *list, unsigned long id)
		{
			int32 i = 0;
			_rep_data_ *item;
			while ((item = (_rep_data_*)list->ItemAt(i++)) != NULL) {
				if (item->fId == id)
					return item;
			}

			return NULL;
		}

static	int32		index_of(BList const *list, BMessage const *msg)
		{
			int32 i = 0;
			_rep_data_ *item;
			while ((item = (_rep_data_*)list->ItemAt(i++)) != NULL) {
				if (item->fMessage == msg)
					return i;
			}

			return -1;
		}

static	int32		index_of(BList const *list, BView const *view, bool aBool)
		{
			int32 i = 0;
			_rep_data_ *item;
			while ((item = (_rep_data_*)list->ItemAt(i++)) != NULL) {
				if (item->fView == view)
					return i;

				if (aBool && item->fView2 == view)
					return i;
			}

			return -1;
		}

static	int32		index_of(BList const *list, unsigned long id)
		{
			int32 i = 0;
			_rep_data_ *item;
			while ((item = (_rep_data_*)list->ItemAt(i++)) != NULL) {
				if (item->fId == id)
					return i;
			}

			return -1;
		}

friend class BShelf;

		BMessage			*fMessage;
		BView				*fView;
		BDragger			*fDragger;
		BDragger::relation	fRelation;
		unsigned long		fId;
		image_id			fImage;
		status_t			fError;
		BView				*fView2;
};


class _TContainerViewFilter_ : public BMessageFilter {
public:
						_TContainerViewFilter_(BShelf *shelf, BView *view);
virtual					~_TContainerViewFilter_();
			
		filter_result	Filter(BMessage *msg, BHandler **handler);
		filter_result	ObjectDropFilter(BMessage *msg, BHandler **handler);

		BShelf	*fShelf;
		BView	*fView;
};


_TContainerViewFilter_::_TContainerViewFilter_(BShelf *shelf, BView *view)
	:	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE)
{
	fShelf = shelf;
	fView = view;
}


_TContainerViewFilter_::~_TContainerViewFilter_()
{
}


filter_result
_TContainerViewFilter_::Filter(BMessage *msg, BHandler **handler)
{
	filter_result filter = B_DISPATCH_MESSAGE;

	if (msg->what == B_ARCHIVED_OBJECT ||
		msg->what == B_ABOUT_REQUESTED)
		return ObjectDropFilter(msg, handler);

	return filter;
}


filter_result
_TContainerViewFilter_::ObjectDropFilter(BMessage *msg, BHandler **_handler)
{
	BView *mouseView;

	if (*_handler)
		mouseView = dynamic_cast<BView*>(*_handler);
	else
		mouseView = NULL;

	if (msg->WasDropped()) {
		if (!fShelf->fAllowDragging) {
			printf("Dragging replicants isn't allowed to this shelf.");
			beep();
			return B_SKIP_MESSAGE;
		}
	}

	BPoint point;
	BPoint offset;

	if (msg->WasDropped()) {
		point = msg->DropPoint(&offset);

		point = mouseView->ConvertFromScreen(point - offset);
	}

	BHandler *handler;
	BLooper *looper;
	handler = msg->ReturnAddress().Target(&looper);

	BDragger *dragger;

	if (Looper() == looper) {
		if (handler)
			dragger = dynamic_cast<BDragger*>(handler);
		else
			dragger = NULL;

		if (dragger->fRelation == BDragger::TARGET_IS_CHILD) {
			BRect rect = dragger->Frame();
			rect.OffsetTo(point);
			point = fShelf->AdjustReplicantBy(rect, msg);
		
		} else {
			BRect rect = dragger->fTarget->Frame();
			rect.OffsetTo(point);
			point = fShelf->AdjustReplicantBy(rect, msg);
		}

		if (dragger->fRelation == BDragger::TARGET_IS_PARENT)
			dragger->fTarget->MoveTo(point);
		else if (dragger->fRelation == BDragger::TARGET_IS_CHILD)
			dragger->MoveTo(point);
		else {
			//TODO: TARGET_UNKNOWN/TARGET_SIBLING
		}
	
	} else {
		if (fShelf->RealAddReplicant(msg, &point, 0) == B_OK)
			Looper()->DetachCurrentMessage();
	}

	return B_SKIP_MESSAGE;
}


class _TReplicantViewFilter_ : public BMessageFilter {
public:
						_TReplicantViewFilter_(BShelf *shelf, BView *view)
							:	BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE)
						{
							fShelf = shelf;
							fView = view;
						}
virtual					~_TReplicantViewFilter_()
						{
						}
			
		filter_result	Filter(BMessage *, BHandler **)
						{
							return B_DISPATCH_MESSAGE;
						}

		BShelf	*fShelf;
		BView	*fView;
};


// **** BShelf ****
BShelf::BShelf(BView *view, bool allow_drags, const char *shelf_type)
	:	BHandler(shelf_type)
{
	InitData(NULL, NULL, view, allow_drags);
}


BShelf::BShelf(const entry_ref *ref, BView *view, bool allow_drags,
			   const char *shelf_type)
	:	BHandler(shelf_type)
{
	InitData(new BEntry(ref), NULL, view, allow_drags);
}


BShelf::BShelf(BDataIO *stream, BView *view, bool allow_drags,
			   const char *shelf_type)
	:	BHandler(shelf_type)
{
	InitData(NULL, stream, view, allow_drags);
}


BShelf::BShelf(BMessage *data)
	:	BHandler(data)
{
	// TODO: Implement ?
}


BShelf::~BShelf()
{
	Save();

	delete fEntry;
}


status_t
BShelf::Archive(BMessage *data, bool deep) const
{
	return B_ERROR;
}


BArchivable *
BShelf::Instantiate(BMessage *data)
{
	return NULL;
}


void
BShelf::MessageReceived(BMessage *msg)
{
}


status_t
BShelf::Save()
{
	//TODO
	return B_ERROR;
}


void
BShelf::SetDirty(bool state)
{
	fDirty = state;
}


bool
BShelf::IsDirty() const
{
	return fDirty;
}


BHandler *
BShelf::ResolveSpecifier(BMessage *msg, int32 index,
								   BMessage *specifier, int32 form,
								   const char *property)
{
	//TODO
	return NULL;
}


status_t
BShelf::GetSupportedSuites(BMessage *data)
{
	//TODO
	return B_ERROR;
}


status_t
BShelf::Perform(perform_code d, void *arg)
{
	return BHandler::Perform(d, arg);
}


bool
BShelf::AllowsDragging() const
{
	return fAllowDragging;
}


void
BShelf::SetAllowsDragging(bool state)
{
	fAllowDragging = state;
}


bool
BShelf::AllowsZombies() const
{
	return fAllowZombies;
}


void
BShelf::SetAllowsZombies(bool state)
{
	fAllowZombies = state;
}


bool
BShelf::DisplaysZombies() const
{
	return fDisplayZombies;
}


void
BShelf::SetDisplaysZombies(bool state)
{
	fDisplayZombies = state;
}


bool
BShelf::IsTypeEnforced() const
{
	return fTypeEnforced;
}


void
BShelf::SetTypeEnforced(bool state)
{
	fTypeEnforced = state;
}


status_t
BShelf::SetSaveLocation(BDataIO *data_io)
{
	fDirty = true;

	if (fEntry) {
		delete fEntry;
		fEntry = NULL;
	}

	fStream = data_io;

	return B_OK;
}


status_t
BShelf::SetSaveLocation(const entry_ref *ref)
{
	fDirty = true;

	if (fEntry)
		delete fEntry;
	else
		fStream = NULL;

	fEntry = new BEntry(ref);

	return B_OK;
}


BDataIO *
BShelf::SaveLocation(entry_ref *ref) const
{
	entry_ref entry;

	if (fStream && ref) {
		*ref = entry;
		return fStream;
	
	} else if (fEntry) {
		fEntry->GetRef(&entry);

		if (ref)
			*ref = entry;

		return NULL;
	
	} else {
		*ref = entry;
		return NULL;
	}
}


status_t
BShelf::AddReplicant(BMessage *data, BPoint location)
{
	return RealAddReplicant(data, &location, 0);
}


status_t
BShelf::DeleteReplicant(BView *replicant)
{
	int32 index = _rep_data_::index_of(&fReplicants, replicant, true);

	_rep_data_ *item = (_rep_data_*)fReplicants.ItemAt(index);

	if (!item)
		return B_ERROR;

	bool aBool;

	item->fMessage->FindBool("", &aBool);

	BView *view = item->fView;

	if (!view)
		view = item->fView2;

	if (view)
		view->RemoveSelf();

	if (item->fDragger)
		item->fDragger->RemoveSelf();

	fReplicants.RemoveItem(item);

	if (aBool && item->fImage >= 0)
		unload_add_on(item->fImage);

	delete item;

	return B_OK;
}


status_t
BShelf::DeleteReplicant(BMessage *data)
{
	int32 index = _rep_data_::index_of(&fReplicants, data);

	_rep_data_ *item = (_rep_data_*)fReplicants.ItemAt(index);

	if (!item)
		return B_ERROR;

	bool aBool;

	item->fMessage->FindBool("", &aBool);

	BView *view = item->fView;

	if (!view)
		view = item->fView2;

	if (view)
		view->RemoveSelf();

	if (item->fDragger)
		item->fDragger->RemoveSelf();

	fReplicants.RemoveItem(item);

	if (aBool && item->fImage >= 0)
		unload_add_on(item->fImage);

	delete item;

	return B_OK;
}


status_t
BShelf::DeleteReplicant(int32 index)
{
	_rep_data_ *item = (_rep_data_*)fReplicants.ItemAt(index);

	if (!item)
		return B_ERROR;

	bool aBool;

	item->fMessage->FindBool("", &aBool);

	BView *view = item->fView;

	if (!view)
		view = item->fView2;

	if (view)
		view->RemoveSelf();

	if (item->fDragger)
		item->fDragger->RemoveSelf();

	ReplicantDeleted(index, item->fMessage, item->fView);

	fReplicants.RemoveItem(item);

	if (aBool && item->fImage >= 0)
		unload_add_on(item->fImage);

	delete item;

	return B_OK;
}


int32
BShelf::CountReplicants() const
{
	return fReplicants.CountItems();
}


BMessage *
BShelf::ReplicantAt(int32 index, BView **view, uint32 *uid,
							  status_t *perr) const
{
	status_t err = B_ERROR;
	BMessage *message = NULL;

	_rep_data_ *item = (_rep_data_*)fReplicants.ItemAt(index);

	if (item) {
		message = item->fMessage;
		*view = item->fView;
		*uid = item->fId;
		*perr = item->fError;
	
	} else {
		*view = NULL;
		*uid = 0;
		*perr = err;
	}

	return message;
}


int32
BShelf::IndexOf(const BView *replicant_view) const
{
	return _rep_data_::index_of(&fReplicants, replicant_view, false);
}


int32
BShelf::IndexOf(const BMessage *archive) const
{
	return _rep_data_::index_of(&fReplicants, archive);
}


int32
BShelf::IndexOf(uint32 id) const
{
	return _rep_data_::index_of(&fReplicants, id);
}


bool
BShelf::CanAcceptReplicantMessage(BMessage*) const
{
	return true;
}


bool
BShelf::CanAcceptReplicantView(BRect, BView*, BMessage*) const
{
	return true;
}


BPoint
BShelf::AdjustReplicantBy(BRect, BMessage*) const
{
	return B_ORIGIN;
}


void
BShelf::ReplicantDeleted(int32 index, const BMessage *archive,
							  const BView *replicant)
{
}


void BShelf::_ReservedShelf2() {}
void BShelf::_ReservedShelf3() {}
void BShelf::_ReservedShelf4() {}
void BShelf::_ReservedShelf5() {}
#if !_PR3_COMPATIBLE_
void BShelf::_ReservedShelf6() {}
void BShelf::_ReservedShelf7() {}
void BShelf::_ReservedShelf8() {}
#endif


BShelf::BShelf(const BShelf&)
{
}


BShelf &
BShelf::operator=(const BShelf &)
{
	return *this;
}


status_t
BShelf::_Archive(BMessage *data) const
{
	BHandler::Archive(data);

	data->AddBool("_zom_dsp", DisplaysZombies());
	data->AddBool("_zom_alw", AllowsZombies());
	data->AddInt32("_sg_cnt", fGenCount);

	BMessage archive('ARCV');

	// TODO archive replicants

	return B_ERROR;
}


void
BShelf::InitData(BEntry *entry, BDataIO *stream, BView *view,
					  bool allow_drags)
{
	fContainerView = view;
	fStream = NULL;
	fEntry = entry;
	fFilter = NULL;
	fGenCount = 1;
	fAllowDragging = allow_drags;
	fDirty = true;
	fDisplayZombies = false;
	fAllowZombies = true;
	fTypeEnforced = false;

	if (entry)
		fStream = new BFile(entry, B_READ_ONLY);
	else
		fStream = stream;

	fFilter = new _TContainerViewFilter_(this, fContainerView);

	fContainerView->AddFilter(fFilter);
	fContainerView->set_shelf(this);

	if (fStream) {
		BMessage archive;

		if (archive.Unflatten(fStream) == B_OK) {
			bool aBool;

			if (!archive.FindBool("_zom_dsp", &aBool))
				aBool = false;

			SetDisplaysZombies(aBool);

			if (!archive.FindBool("_zom_alw", &aBool))
				aBool = true;

			SetAllowsZombies(aBool);

			int32 genCount;

			if (!archive.FindInt32("_sg_cnt", &genCount))
				genCount = 1;

			// TODO find archived replicants
		}
	}
}


status_t
BShelf::RealAddReplicant(BMessage *data, BPoint *loc, uint32 uid)
{
	BView *replicant = NULL;
	BDragger *dragger = NULL;
	BDragger::relation relation = BDragger::TARGET_UNKNOWN;
	BMessage widget;
	BMessage reply;
	image_id image = B_ERROR;
	image_id image2 = B_ERROR;
	_BZombieReplicantView_ *zombie = NULL;
	//bool wasDropped = data->WasDropped();
	const char *shelf_type = NULL;

	data->FindString("shelf_type", &shelf_type);

	// Check shelf types if needed
	if (fTypeEnforced) {
		if (shelf_type) {
			if (Name() && strcmp(shelf_type, Name()) != 0) {
				printf("Replicant was rejected by BShelf: The BShelf's type and the Replicant's type don't match.");
				return B_ERROR;
		
			} else {
				printf("Replicant was rejected by BShelf: Replicant indicated a <type> (%s), but the shelf does not.", shelf_type);
				return B_ERROR;
			}
		
		} else {
			printf("Replicant was rejected by BShelf: Replicant did not have a <type>");
			return B_ERROR;
		}
	}

	// Check if we can accept this message
	if (!CanAcceptReplicantMessage(data))
		return B_ERROR;

	// Check if we can create multiple instances
	if (data->FindBool("be:load_each_time")) {
		const char *_class = NULL;
		const char *add_on = NULL;

		if (data->FindString("class", &_class)) {
			if (data->FindString("add_on", &add_on)) {
				int32 i = 0;
				_rep_data_ *item;
				const char *rep_class = NULL;
				const char *rep_add_on = NULL;

				while ((item = (_rep_data_*)fReplicants.ItemAt(i++)) !=NULL) {
					item->fMessage->FindString("class", &rep_class);
					item->fMessage->FindString("add_on", &rep_add_on);

					if (strcmp(_class, rep_class) == 0) {
						if (add_on && rep_add_on && strcmp(_class, rep_class) == 0)
							printf("Replicant was rejected. Unique replicant already exists. class=%s, signature=%s",
								rep_class, rep_add_on);
					}
				}
			}
		}
	}

	// Instantiate the object, if this fails we have a zombie
	BArchivable *archivable = instantiate_object(data, &image);

	if (archivable) {
		BView *view = dynamic_cast<BView*>(archivable);
		BPoint point;

		if (loc)
			point = *loc;
		else
			point = view->Frame().LeftTop();

		// Check if we have a dragger archived as "__widget" inside the message
		if (data->FindMessage("__widget", &widget) == B_OK) {
			BArchivable *archivable2 = instantiate_object(&widget, &image2);

			replicant = view;

			if (archivable2) {
				if ((dragger = dynamic_cast<BDragger*>(archivable2)) != NULL) {
					// Replicant is either a sibling or unknown
					dragger->SetViewToDrag(replicant);
				}
			}
		} else {
			// Replicant is child of the dragger
			if ((dragger = dynamic_cast<BDragger*>(view)) != NULL)
				dragger->SetViewToDrag(replicant = dragger->ChildAt(0));
			else {
				// Replicant is parent of the dragger
				replicant = view;
				dragger = dynamic_cast<BDragger*>(replicant->FindView("_dragger_"));

				if (dragger)
					dragger->SetViewToDrag(replicant);
			}
		}

		dragger->SetShelf(this);

		AddFilter(new _TReplicantViewFilter_(this, replicant));

		fContainerView->AddChild(view);
	
	} else if (fDisplayZombies && fAllowZombies) {
		BRect _frame;

		if (data->FindRect("_frame", &_frame) != B_OK)
			_frame = BRect();

		if (data->WasDropped()) {
			BPoint dropPoint, offset;
			dropPoint = data->DropPoint(&offset);

			_frame.OffsetTo(B_ORIGIN);
			_frame.OffsetTo(fContainerView->ConvertFromScreen(dropPoint)-offset);

			zombie = new _BZombieReplicantView_(_frame, B_ERROR);

			_frame.OffsetTo(B_ORIGIN);

			BDragger *dragger = new BDragger(_frame, zombie);

			dragger->SetShelf(this);
			dragger->SetZombied(true);

			zombie->AddChild(dragger);

			AddFilter(new _TReplicantViewFilter_(this, zombie));

			fContainerView->AddChild(zombie);
		}
	}
	
	data->RemoveName("_drop_point_");
	data->RemoveName("_drop_offset_");

	_rep_data_ *item = new _rep_data_(data, replicant, dragger, relation, uid,
		image);

	item->fError = B_OK;
	item->fView2 = zombie;

	if (zombie)
		zombie->SetArchive(data);

	fReplicants.AddItem(item);

	if (data->IsSourceWaiting()) {
		reply.AddInt32("id", uid);
		reply.AddInt32("error", B_OK);
		data->SendReply(&reply);
	}

	//TODO:

	return B_ERROR;
}


status_t
BShelf::GetProperty(BMessage *msg, BMessage *reply)
{
	//TODO: Implement
	return B_ERROR;
}
