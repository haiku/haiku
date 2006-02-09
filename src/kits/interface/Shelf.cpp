/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*!	BShelf stores replicant views that are dropped onto it */


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
#include <string.h>


extern "C" void  _ReservedShelf1__6BShelfFv(BShelf *const, int32,
	const BMessage*, const BView*);


class _rep_data_ {
	private:
		friend class BShelf;

		_rep_data_(BMessage *message, BView *view, BDragger *dragger,
			BDragger::relation relation, unsigned long id, image_id image);
		_rep_data_();

		static _rep_data_* find(BList const *list, BMessage const *msg);
		static _rep_data_* find(BList const *list, BView const *view, bool allowZombie);
		static _rep_data_* find(BList const *list, unsigned long id);

		static int32 index_of(BList const *list, BMessage const *msg);
		static int32 index_of(BList const *list, BView const *view, bool allowZombie);
		static int32 index_of(BList const *list, unsigned long id);

		BMessage			*fMessage;
		BView				*fView;
		BDragger			*fDragger;
		BDragger::relation	fRelation;
		unsigned long		fId;
		image_id			fImage;
		status_t			fError;
		BView				*fZombieView;
};


class _TContainerViewFilter_ : public BMessageFilter {
	public:
		_TContainerViewFilter_(BShelf *shelf, BView *view);
		virtual ~_TContainerViewFilter_();

		filter_result	Filter(BMessage *msg, BHandler **handler);
		filter_result	ObjectDropFilter(BMessage *msg, BHandler **handler);

	protected:
		BShelf	*fShelf;
		BView	*fView;
};


//	#pragma mark -


/*!	\brief Helper function for BShelf::_AddReplicant()
*/
static status_t
send_reply(BMessage* message, status_t status, uint32 uniqueID)
{
	if (message->IsSourceWaiting()) {
		BMessage reply(B_REPLY);
		reply.AddInt32("id", uniqueID);
		reply.AddInt32("error", status);
		message->SendReply(&reply);
	}

	return status;
}


//	#pragma mark -


_rep_data_::_rep_data_(BMessage *message, BView *view, BDragger *dragger,
	BDragger::relation relation, unsigned long id, image_id image)
	:
	fMessage(message),
	fView(view),
	fDragger(NULL),
	fRelation(relation),
	fId(id),
	fImage(image),
	fError(B_OK),
	fZombieView(NULL)
{
}


_rep_data_::_rep_data_()
	:
	fMessage(NULL),
	fView(NULL),
	fDragger(NULL),
	fRelation(BDragger::TARGET_UNKNOWN),
	fId(0),
	fImage(-1),
	fError(B_ERROR),
	fZombieView(NULL)
{
}


//static
_rep_data_ *
_rep_data_::find(BList const *list, BMessage const *msg)
{
	int32 i = 0;
	_rep_data_ *item;
	while ((item = (_rep_data_*)list->ItemAt(i++)) != NULL) {
		if (item->fMessage == msg)
			return item;
	}

	return NULL;
}


//static
_rep_data_ *
_rep_data_::find(BList const *list, BView const *view, bool allowZombie)
{
	int32 i = 0;
	_rep_data_ *item;
	while ((item = (_rep_data_*)list->ItemAt(i++)) != NULL) {
		if (item->fView == view)
			return item;

		if (allowZombie && item->fZombieView == view)
			return item;
	}

	return NULL;
}


//static
_rep_data_ *
_rep_data_::find(BList const *list, unsigned long id)
{
	int32 i = 0;
	_rep_data_ *item;
	while ((item = (_rep_data_*)list->ItemAt(i++)) != NULL) {
		if (item->fId == id)
			return item;
	}

	return NULL;
}


//static
int32
_rep_data_::index_of(BList const *list, BMessage const *msg)
{
	int32 i = 0;
	_rep_data_ *item;
	while ((item = (_rep_data_*)list->ItemAt(i)) != NULL) {
		if (item->fMessage == msg)
			return i;
		i++;
	}

	return -1;
}


//static
int32
_rep_data_::index_of(BList const *list, BView const *view, bool allowZombie)
{
	int32 i = 0;
	_rep_data_ *item;
	while ((item = (_rep_data_*)list->ItemAt(i)) != NULL) {
		if (item->fView == view)
			return i;

		if (allowZombie && item->fZombieView == view)
			return i;
		i++;
	}

	return -1;
}


//static
int32
_rep_data_::index_of(BList const *list, unsigned long id)
{
	int32 i = 0;
	_rep_data_ *item;
	while ((item = (_rep_data_*)list->ItemAt(i)) != NULL) {
		if (item->fId == id)
			return i;
		i++;
	}

	return -1;
}


//	#pragma mark -


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

	BLooper *looper;
	BHandler *handler = msg->ReturnAddress().Target(&looper);

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
		if (fShelf->_AddReplicant(msg, &point, 0) == B_OK)
			Looper()->DetachCurrentMessage();
	}

	return B_SKIP_MESSAGE;
}


//	#pragma mark -


class _TReplicantViewFilter_ : public BMessageFilter {
	public:
		_TReplicantViewFilter_(BShelf *shelf, BView *view)
			: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE)
		{
			fShelf = shelf;
			fView = view;
		}

		virtual	~_TReplicantViewFilter_()
		{
		}
	
		filter_result Filter(BMessage *, BHandler **)
		{
			return B_DISPATCH_MESSAGE;
		}

	protected:
		BShelf	*fShelf;
		BView	*fView;
};


//	#pragma mark -


BShelf::BShelf(BView *view, bool allowDrags, const char *shelfType)
	: BHandler(shelfType)
{
	_InitData(NULL, NULL, view, allowDrags);
}


BShelf::BShelf(const entry_ref *ref, BView *view, bool allowDrags,
	const char *shelfType)
	: BHandler(shelfType)
{
	_InitData(new BEntry(ref), NULL, view, allowDrags);
}


BShelf::BShelf(BDataIO *stream, BView *view, bool allowDrags,
	const char *shelfType)
	: BHandler(shelfType)
{
	_InitData(NULL, stream, view, allowDrags);
}


BShelf::BShelf(BMessage *data)
	: BHandler(data)
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
	//TODO: Implement
}


status_t
BShelf::Save()
{
	status_t status = B_ERROR;
	if (fEntry != NULL) {
		BFile *file = new BFile(fEntry, B_READ_WRITE);
		status = file->InitCheck();
		if (status < B_OK) {
			delete file;
			return status;
		}
		fStream = file;
	}

	if (fStream != NULL) {
		BMessage message;
		status = _Archive(&message);
		if (status == B_OK)
			status = message.Flatten(fStream);
	}

	return status;
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
	}
	if (fEntry) {
		fEntry->GetRef(&entry);

		if (ref)
			*ref = entry;

		return NULL;
	}

	*ref = entry;
	return NULL;
}


status_t
BShelf::AddReplicant(BMessage *data, BPoint location)
{
	return _AddReplicant(data, &location, 0);
}


status_t
BShelf::DeleteReplicant(BView *replicant)
{
	int32 index = _rep_data_::index_of(&fReplicants, replicant, true);

	_rep_data_ *item = (_rep_data_*)fReplicants.ItemAt(index);
	if (item == NULL)
		return B_BAD_VALUE;

	return _DeleteReplicant(item);
}


status_t
BShelf::DeleteReplicant(BMessage *data)
{
	int32 index = _rep_data_::index_of(&fReplicants, data);

	_rep_data_ *item = (_rep_data_*)fReplicants.ItemAt(index);
	if (!item)
		return B_BAD_VALUE;

	return _DeleteReplicant(item);
}


status_t
BShelf::DeleteReplicant(int32 index)
{
	_rep_data_ *item = (_rep_data_*)fReplicants.ItemAt(index);
	if (!item)
		return B_BAD_INDEX;

	return _DeleteReplicant(item);
}


int32
BShelf::CountReplicants() const
{
	return fReplicants.CountItems();
}


BMessage *
BShelf::ReplicantAt(int32 index, BView **_view, uint32 *_uniqueID,
	status_t *_error) const
{
	_rep_data_ *item = (_rep_data_*)fReplicants.ItemAt(index);
	if (item == NULL) {
		// no replicant found
		if (_view)
			*_view = NULL;
		if (_uniqueID)
			*_uniqueID = ~0UL;
		if (_error)
			*_error = B_BAD_INDEX;

		return NULL;
	}

	if (_view)
		*_view = item->fView;
	if (_uniqueID)
		*_uniqueID = item->fId;
	if (_error)
		*_error = item->fError;

	return item->fMessage;
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


void
_ReservedShelf1__6BShelfFv(BShelf *const, int32, const BMessage*, 
								const BView*)
{
}


void BShelf::_ReservedShelf2() {}
void BShelf::_ReservedShelf3() {}
void BShelf::_ReservedShelf4() {}
void BShelf::_ReservedShelf5() {}
void BShelf::_ReservedShelf6() {}
void BShelf::_ReservedShelf7() {}
void BShelf::_ReservedShelf8() {}


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
BShelf::_InitData(BEntry *entry, BDataIO *stream, BView *view,
	bool allowDrags)
{
	fContainerView = view;
	fStream = NULL;
	fEntry = entry;
	fFilter = NULL;
	fGenCount = 1;
	fAllowDragging = allowDrags;
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
	fContainerView->_SetShelf(this);

	if (fStream) {
		BMessage archive;

		if (archive.Unflatten(fStream) == B_OK) {
			bool allowZombies;
			if (archive.FindBool("_zom_dsp", &allowZombies) != B_OK)
				allowZombies = false;

			SetDisplaysZombies(allowZombies);

			if (archive.FindBool("_zom_alw", &allowZombies) != B_OK)
				allowZombies = true;

			SetAllowsZombies(allowZombies);

			int32 genCount;
			if (!archive.FindInt32("_sg_cnt", &genCount))
				genCount = 1;

			// TODO find archived replicants
		}
	}
}


status_t
BShelf::_DeleteReplicant(_rep_data_* item)
{
	bool loadedImage = item->fMessage->FindBool("");

	BView *view = item->fView;
	if (view == NULL)
		view = item->fZombieView;

	if (view)
		view->RemoveSelf();

	if (item->fDragger)
		item->fDragger->RemoveSelf();

	int32 index = _rep_data_::index_of(&fReplicants, item->fMessage);
	
	// TODO: Test if it's ok here
	ReplicantDeleted(index, item->fMessage, view);
	
	fReplicants.RemoveItem(item);

	if (loadedImage && item->fImage >= 0)
		unload_add_on(item->fImage);

	delete item;

	return B_OK;
}


status_t
BShelf::_AddReplicant(BMessage *data, BPoint *location, uint32 uniqueID)
{
	const char *shelfType = NULL;
	data->FindString("shelf_type", &shelfType);

	// Check shelf types if needed
	if (fTypeEnforced) {
		if (shelfType) {
			if (Name() && strcmp(shelfType, Name()) != 0) {
				printf("Replicant was rejected by BShelf: The BShelf's type and the Replicant's type don't match.");
				return send_reply(data, B_ERROR, uniqueID);
			} else {
				printf("Replicant was rejected by BShelf: Replicant indicated a <type> (%s), but the shelf does not.", shelfType);
				return send_reply(data, B_ERROR, uniqueID);
			}
		} else {
			printf("Replicant was rejected by BShelf: Replicant did not have a <type>");
			return send_reply(data, B_ERROR, uniqueID);
		}
	}

	// Check if we can accept this message
	if (!CanAcceptReplicantMessage(data)) {
		printf("Replicant was rejected by BShelf: CanAcceptReplicant() returned false");
		return send_reply(data, B_ERROR, uniqueID);
	}

	// Check if we can create multiple instances
	if (data->FindBool("be:load_each_time")) {
		const char *_class = NULL;
		const char *add_on = NULL;

		if (data->FindString("class", &_class) == B_OK
			&& data->FindString("add_on", &add_on) == B_OK) {
			int32 i = 0;
			_rep_data_ *item;
			const char *rep_class = NULL;
			const char *rep_add_on = NULL;
			
			while ((item = (_rep_data_*)fReplicants.ItemAt(i++)) != NULL) {
				if (item->fMessage->FindString("class", &rep_class) == B_OK
					&& item->fMessage->FindString("add_on", &rep_add_on) == B_OK
					&& !strcmp(_class, rep_class) && add_on && rep_add_on
					&& !strcmp(add_on, rep_add_on)) {
					printf("Replicant was rejected. Unique replicant already exists. class=%s, signature=%s",
						rep_class, rep_add_on);
					return send_reply(data, B_ERROR, uniqueID);
				}
			}
		}
	}

	BDragger* dragger = NULL;
	BView* replicant = NULL;
	BDragger::relation relation = BDragger::TARGET_UNKNOWN;
	BMessage widget;
	_BZombieReplicantView_* zombie = NULL;
	//bool wasDropped = data->WasDropped();

	// Instantiate the object, if this fails we have a zombie
	image_id image;
	BArchivable *archivable = instantiate_object(data, &image);
	if (archivable) {
		BView *view = dynamic_cast<BView*>(archivable);
		BPoint point;

		if (location)
			point = *location;
		else
			point = view->Frame().LeftTop();

		// TODO: test me -- there seems to be lots of bugs parked here!

		// Check if we have a dragger archived as "__widget" inside the message
		if (data->FindMessage("__widget", &widget) == B_OK) {
			image_id draggerImage = B_ERROR;
			BArchivable *archivable = instantiate_object(&widget, &draggerImage);

			replicant = view;

			if (archivable) {
				if ((dragger = dynamic_cast<BDragger*>(archivable)) != NULL) {
					// Replicant is either a sibling or unknown
					dragger->SetViewToDrag(replicant);
					relation = BDragger::TARGET_IS_SIBLING;
				}
			}
		} else {
			// Replicant is child of the dragger
			if ((dragger = dynamic_cast<BDragger*>(view)) != NULL) {
				replicant = dragger->ChildAt(0);
				dragger->SetViewToDrag(replicant);
				relation = BDragger::TARGET_IS_CHILD;
			} else {
				// Replicant is parent of the dragger
				replicant = view;
				dragger = dynamic_cast<BDragger*>(replicant->FindView("_dragger_"));

				if (dragger)
					dragger->SetViewToDrag(replicant);

				relation = BDragger::TARGET_IS_PARENT;
			}
		}

		if (dragger != NULL)
			dragger->SetShelf(this);

		BRect frame;
		if (relation == BDragger::TARGET_IS_CHILD)
			frame = dragger->Frame().OffsetToCopy(point);
		else
			frame = replicant->Frame().OffsetToCopy(point);

		BPoint adjust = AdjustReplicantBy(frame, data);

		AddFilter(new _TReplicantViewFilter_(this, replicant));

		// TODO: that's probably not correct for all relations (or any?)
		view->MoveTo(point + adjust);

		if (relation != BDragger::TARGET_IS_CHILD)
			fContainerView->AddChild(replicant);

		// if it's a sibling or a child, we need to add the dragger
		if (relation == BDragger::TARGET_IS_SIBLING || relation == BDragger::TARGET_IS_CHILD)
			fContainerView->AddChild(dragger);
	} else if (fDisplayZombies && fAllowZombies) {
		// TODO: the zombies must be adjusted and moved as well!
		BRect frame;
		if (data->FindRect("_frame", &frame) != B_OK)
			frame = BRect();

		if (data->WasDropped()) {
			BPoint dropPoint, offset;
			dropPoint = data->DropPoint(&offset);

			frame.OffsetTo(B_ORIGIN);
			frame.OffsetTo(fContainerView->ConvertFromScreen(dropPoint) - offset);

			zombie = new _BZombieReplicantView_(frame, B_ERROR);

			frame.OffsetTo(B_ORIGIN);

			BDragger *dragger = new BDragger(frame, zombie);

			dragger->SetShelf(this);
			dragger->SetZombied(true);

			zombie->AddChild(dragger);

			AddFilter(new _TReplicantViewFilter_(this, zombie));

			fContainerView->AddChild(zombie);
		}
	}

	data->RemoveName("_drop_point_");
	data->RemoveName("_drop_offset_");

	_rep_data_ *item = new _rep_data_(data, replicant, dragger, relation,
		uniqueID, image);

	item->fError = B_OK;
	item->fZombieView = zombie;

	if (zombie)
		zombie->SetArchive(data);

	fReplicants.AddItem(item);

	return send_reply(data, B_OK, uniqueID);
}


status_t
BShelf::_GetProperty(BMessage *msg, BMessage *reply)
{
	//TODO: Implement
	return B_ERROR;
}
