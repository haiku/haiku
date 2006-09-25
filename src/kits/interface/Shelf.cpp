/*
 * Copyright 2001-2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Marc Flerackers (mflerackers@androme.be)
 *		Axel Dörfler, axeld@pinc-software.de
 *		Jérôme Duval
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
#include <PropertyInfo.h>
#include <Rect.h>
#include <Shelf.h>
#include <View.h>

#include <ViewPrivate.h>
#include <ZombieReplicantView.h>

#include <stdio.h>
#include <string.h>


static property_info sShelfPropertyList[] = {
	{
		"Replicant",
		{ B_COUNT_PROPERTIES, B_CREATE_PROPERTY },
		{ B_DIRECT_SPECIFIER }, 
		NULL, 0, 
	},

	{
		"Replicant",
		{ B_DELETE_PROPERTY, B_GET_PROPERTY },
		{ B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, B_NAME_SPECIFIER, B_ID_SPECIFIER }, 
		NULL, 0, 
	},

	{
		"Replicant",
		{}, 
		{ B_INDEX_SPECIFIER, B_REVERSE_INDEX_SPECIFIER, B_NAME_SPECIFIER, B_ID_SPECIFIER },
		"... of Replicant {index | name | id} of ...", 0,
	},
	
	{ 0, { 0 }, { 0 }, 0, 0 }
};

static property_info sReplicantPropertyList[] = {
	{
		"ID",
		{ B_GET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0, { B_INT32_TYPE }
	},
	
	{
		"Name",
		{ B_GET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0, { B_STRING_TYPE }
	},

	{
		"Signature",
		{ B_GET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0, { B_STRING_TYPE }
	},

	{
		"Suites",
		{ B_GET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		NULL, 0, { B_PROPERTY_INFO_TYPE }
	},

	{
		"View",
		{ },
		{ B_DIRECT_SPECIFIER },
		NULL, 0,
	},
	
	{ 0, { 0 }, { 0 }, 0, 0 }
};


extern "C" void  _ReservedShelf1__6BShelfFv(BShelf *const, int32,
	const BMessage*, const BView*);


namespace BPrivate {

struct replicant_data {
	replicant_data(BMessage *message, BView *view, BDragger *dragger,
		BDragger::relation relation, unsigned long id, image_id image);
	replicant_data();

	static replicant_data* Find(BList const *list, BMessage const *msg);
	static replicant_data* Find(BList const *list, BView const *view, bool allowZombie);
	static replicant_data* Find(BList const *list, unsigned long id);

	static int32 IndexOf(BList const *list, BMessage const *msg);
	static int32 IndexOf(BList const *list, BView const *view, bool allowZombie);
	static int32 IndexOf(BList const *list, unsigned long id);

	BMessage*			message;
	BView*				view;
	BDragger*			dragger;
	BDragger::relation	relation;
	unsigned long		id;
	image_id			image;
	status_t			error;
	BView*				zombie_view;
};

class ShelfContainerViewFilter : public BMessageFilter {
	public:
		ShelfContainerViewFilter(BShelf *shelf, BView *view);

		filter_result	Filter(BMessage *msg, BHandler **handler);

	private:
		filter_result	_ObjectDropFilter(BMessage *msg, BHandler **handler);

		BShelf	*fShelf;
		BView	*fView;
};

class ReplicantViewFilter : public BMessageFilter {
	public:
		ReplicantViewFilter(BShelf *shelf, BView *view);

		filter_result Filter(BMessage *message, BHandler **handler);

	private:
		BShelf	*fShelf;
		BView	*fView;
};

}	// namespace BPrivate


using BPrivate::replicant_data;
using BPrivate::ReplicantViewFilter;
using BPrivate::ShelfContainerViewFilter;


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


replicant_data::replicant_data(BMessage *_message, BView *_view, BDragger *_dragger,
	BDragger::relation _relation, unsigned long _id, image_id _image)
	:
	message(_message),
	view(_view),
	dragger(NULL),
	relation(_relation),
	id(_id),
	image(_image),
	error(B_OK),
	zombie_view(NULL)
{
}


replicant_data::replicant_data()
	:
	message(NULL),
	view(NULL),
	dragger(NULL),
	relation(BDragger::TARGET_UNKNOWN),
	id(0),
	image(-1),
	error(B_ERROR),
	zombie_view(NULL)
{
}


//static
replicant_data *
replicant_data::Find(BList const *list, BMessage const *msg)
{
	int32 i = 0;
	replicant_data *item;
	while ((item = (replicant_data*)list->ItemAt(i++)) != NULL) {
		if (item->message == msg)
			return item;
	}

	return NULL;
}


//static
replicant_data *
replicant_data::Find(BList const *list, BView const *view, bool allowZombie)
{
	int32 i = 0;
	replicant_data *item;
	while ((item = (replicant_data*)list->ItemAt(i++)) != NULL) {
		if (item->view == view)
			return item;

		if (allowZombie && item->zombie_view == view)
			return item;
	}

	return NULL;
}


//static
replicant_data *
replicant_data::Find(BList const *list, unsigned long id)
{
	int32 i = 0;
	replicant_data *item;
	while ((item = (replicant_data*)list->ItemAt(i++)) != NULL) {
		if (item->id == id)
			return item;
	}

	return NULL;
}


//static
int32
replicant_data::IndexOf(BList const *list, BMessage const *msg)
{
	int32 i = 0;
	replicant_data *item;
	while ((item = (replicant_data*)list->ItemAt(i)) != NULL) {
		if (item->message == msg)
			return i;
		i++;
	}

	return -1;
}


//static
int32
replicant_data::IndexOf(BList const *list, BView const *view, bool allowZombie)
{
	int32 i = 0;
	replicant_data *item;
	while ((item = (replicant_data*)list->ItemAt(i)) != NULL) {
		if (item->view == view)
			return i;

		if (allowZombie && item->zombie_view == view)
			return i;
		i++;
	}

	return -1;
}


//static
int32
replicant_data::IndexOf(BList const *list, unsigned long id)
{
	int32 i = 0;
	replicant_data *item;
	while ((item = (replicant_data*)list->ItemAt(i)) != NULL) {
		if (item->id == id)
			return i;
		i++;
	}

	return -1;
}


//	#pragma mark -


ShelfContainerViewFilter::ShelfContainerViewFilter(BShelf *shelf, BView *view)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
	fShelf(shelf),
	fView(view)	
{
}


filter_result
ShelfContainerViewFilter::Filter(BMessage *msg, BHandler **handler)
{
	filter_result filter = B_DISPATCH_MESSAGE;

	if (msg->what == B_ARCHIVED_OBJECT
		|| msg->what == B_ABOUT_REQUESTED)
		return _ObjectDropFilter(msg, handler);

	return filter;
}


filter_result
ShelfContainerViewFilter::_ObjectDropFilter(BMessage *msg, BHandler **_handler)
{
	BView *mouseView = NULL;
	if (*_handler)
		mouseView = dynamic_cast<BView*>(*_handler);

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

	BLooper *looper = NULL;
	BHandler *handler = msg->ReturnAddress().Target(&looper);

	if (Looper() == looper) {
		BDragger *dragger = NULL;
		if (handler)
			dragger = dynamic_cast<BDragger*>(handler);

		BRect rect;
		if (dragger->fRelation == BDragger::TARGET_IS_CHILD)
			rect = dragger->Frame();
		else
			rect = dragger->fTarget->Frame();
		rect.OffsetTo(point);
		point = rect.LeftTop() + fShelf->AdjustReplicantBy(rect, msg);
		
		if (dragger->fRelation == BDragger::TARGET_IS_PARENT)
			dragger->fTarget->MoveTo(point);
		else if (dragger->fRelation == BDragger::TARGET_IS_CHILD)
			dragger->MoveTo(point);
		else {
			//TODO: TARGET_UNKNOWN/TARGET_SIBLING
		}
	
	} else {
		if (fShelf->_AddReplicant(msg, &point, fShelf->fGenCount++) == B_OK)
			Looper()->DetachCurrentMessage();
	}

	return B_SKIP_MESSAGE;
}


//	#pragma mark -


ReplicantViewFilter::ReplicantViewFilter(BShelf *shelf, BView *view)
	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
	fShelf(shelf),
	fView(view)
{		
}


filter_result
ReplicantViewFilter::Filter(BMessage *message, BHandler **handler)
{
	if (message->what == kDeleteReplicant) {
		if (handler != NULL)
			*handler = fShelf;
		message->AddPointer("_target", fView);
	}
	return B_DISPATCH_MESSAGE;
}


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
	if (msg->what == kDeleteReplicant) {
		BView *replicant = NULL;
		if (msg->FindPointer("_target", (void **)&replicant) == B_OK && replicant != NULL)
			DeleteReplicant(replicant);
		
		return;
	}

	BMessage replyMsg(B_REPLY);
	status_t err = B_BAD_SCRIPT_SYNTAX;

	BMessage specifier;
	int32 what;
	const char *prop;
	int32 index;
	if (msg->GetCurrentSpecifier(&index, &specifier, &what, &prop) != B_OK)
		return BHandler::MessageReceived(msg);

	switch (msg->what) {
		case B_DELETE_PROPERTY:
		case B_GET_PROPERTY:
		case B_GET_SUPPORTED_SUITES:
			if (strcmp(prop, "Replicant") == 0) {
				BMessage reply;
				int32 i;
				uint32 ID;
				BView *replicant = NULL;
				BMessage *repMessage = NULL;
				err = _GetProperty(&specifier, &reply);
				if (err == B_OK)
					err = reply.FindInt32("index", &i);
				bool popped = (msg->PopSpecifier()==B_OK);
				if (err == B_OK && !popped && msg->what == B_DELETE_PROPERTY) { // Delete Replicant
					err = DeleteReplicant(i);
					break;
				}
				if (err == B_OK && popped 
					&& msg->what == B_GET_SUPPORTED_SUITES) {
					err = replyMsg.AddString("suites", "suite/vnd.Be-replicant");
					if (err == B_OK) {
						BPropertyInfo propInfo(sReplicantPropertyList);
						err = replyMsg.AddFlat("messages", &propInfo);
					}
					break;
				}
				if (err == B_OK)
					repMessage = ReplicantAt(i, &replicant, &ID, &err);
				if (err == B_OK && replicant) {
					if (!popped) { // Get Replicant
						BMessage archive;
						err = replicant->Archive(&archive);
						if (err == B_OK)
							err = replyMsg.AddMessage("result", &archive);
						break;
					}
					// now handles the replicant suite
					err = B_BAD_SCRIPT_SYNTAX;
					if (msg->what != B_GET_PROPERTY)
						break;
					if (msg->GetCurrentSpecifier(&index, &specifier, &what, &prop) != B_OK)
						break;
					if (strcmp(prop, "ID") == 0) {
						err = replyMsg.AddInt32("result", ID);
					} else if (strcmp(prop, "Name") == 0) {
						err = replyMsg.AddString("result", replicant->Name());
					} else if (strcmp(prop, "Signature") == 0) {
						const char *add_on = NULL;
						err = repMessage->FindString("add_on", &add_on);
						if (err == B_OK)
							err = replyMsg.AddString("result", add_on);
					} else if (strcmp(prop, "Suites") == 0) {
						err = replyMsg.AddString("suites", "suite/vnd.Be-replicant");
						if (err == B_OK) {
							BPropertyInfo propInfo(sReplicantPropertyList);
							err = replyMsg.AddFlat("messages", &propInfo);
						}
					}
				}
				break;
			}
			return BHandler::MessageReceived(msg);

		case B_COUNT_PROPERTIES:
			if (strcmp(prop, "Replicant") == 0) {
				err = replyMsg.AddInt32("result", CountReplicants());
				break;
			}
			return BHandler::MessageReceived(msg);
	}

	if (err < B_OK) {
		replyMsg.what = B_MESSAGE_NOT_UNDERSTOOD;

		if (err == B_BAD_SCRIPT_SYNTAX)
			replyMsg.AddString("message", "Didn't understand the specifier(s)");
		else
			replyMsg.AddString("message", strerror(err));			
	}

	replyMsg.AddInt32("error", err);
	msg->SendReply(&replyMsg);
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
BShelf::ResolveSpecifier(BMessage *msg, int32 index, BMessage *specifier,
						int32 form, const char *property)
{
	BPropertyInfo shelfPropInfo(sShelfPropertyList);
	BHandler *target = NULL;
	BView *replicant = NULL;
	switch (shelfPropInfo.FindMatch(msg, 0, specifier, form, property)) {
		case 0:
		case 1:
			if (msg->PopSpecifier() != B_OK ) {
				target = this;
				break;
			}
			msg->SetCurrentSpecifier(index);
			// fall through
		case 2: {
			BMessage reply;
			status_t err = _GetProperty(specifier, &reply);
			int32 i;
			uint32 ID;
			if (err == B_OK)
				err = reply.FindInt32("index", &i);
			if (err == B_OK)
				ReplicantAt(i, &replicant, &ID, &err);

			if (err == B_OK && replicant != NULL) {
				if (index <= 0 && msg->what == B_GET_SUPPORTED_SUITES)
					return this;
				msg->PopSpecifier();
			} else {
				BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
				replyMsg.AddInt32("error", B_BAD_INDEX);
				replyMsg.AddString("message", "Cannot find replicant at/with specified index/name.");
				msg->SendReply(&replyMsg);
			}
			}
			break;
	}

	if (!replicant) {
		if (target)
			return target;
		return BHandler::ResolveSpecifier(msg, index, specifier, form,
			property);
	}

	int32 repIndex;
	status_t err = msg->GetCurrentSpecifier(&repIndex, specifier, &form, &property);
	if (err) {
		BMessage reply(B_MESSAGE_NOT_UNDERSTOOD);
		reply.AddInt32("error", err);
		msg->SendReply(&reply);
		return NULL;
	}

	BPropertyInfo replicantPropInfo(sReplicantPropertyList);
	switch (replicantPropInfo.FindMatch(msg, 0, specifier, form, property)) {
		case 0:
		case 1:
		case 2:
		case 3:
			msg->SetCurrentSpecifier(index);
			target = this;
			break;
		case 4:
			target = replicant;
			msg->PopSpecifier();
			break;
		default:
			break;
	}
	if (!target) {
		BMessage replyMsg(B_MESSAGE_NOT_UNDERSTOOD);
		replyMsg.AddInt32("error", B_BAD_SCRIPT_SYNTAX);
		replyMsg.AddString("message", "Didn't understand the specifier(s)");
		msg->SendReply(&replyMsg);
	}
	return target;
}


status_t
BShelf::GetSupportedSuites(BMessage *message)
{
	status_t err;
	err = message->AddString("suites", "suite/vnd.Be-shelf");
	if (err == B_OK) {
		BPropertyInfo propInfo(sShelfPropertyList);
		err = message->AddFlat("messages", &propInfo);
	}
	if (err == B_OK)
		return BHandler::GetSupportedSuites(message);
	return err;
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
	return _AddReplicant(data, &location, fGenCount++);
}


status_t
BShelf::DeleteReplicant(BView *replicant)
{
	int32 index = replicant_data::IndexOf(&fReplicants, replicant, true);

	replicant_data *item = (replicant_data*)fReplicants.ItemAt(index);
	if (item == NULL)
		return B_BAD_VALUE;

	return _DeleteReplicant(item);
}


status_t
BShelf::DeleteReplicant(BMessage *data)
{
	int32 index = replicant_data::IndexOf(&fReplicants, data);

	replicant_data *item = (replicant_data*)fReplicants.ItemAt(index);
	if (!item)
		return B_BAD_VALUE;

	return _DeleteReplicant(item);
}


status_t
BShelf::DeleteReplicant(int32 index)
{
	replicant_data *item = (replicant_data*)fReplicants.ItemAt(index);
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
	replicant_data *item = (replicant_data*)fReplicants.ItemAt(index);
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
		*_view = item->view;
	if (_uniqueID)
		*_uniqueID = item->id;
	if (_error)
		*_error = item->error;

	return item->message;
}


int32
BShelf::IndexOf(const BView* replicantView) const
{
	return replicant_data::IndexOf(&fReplicants, replicantView, false);
}


int32
BShelf::IndexOf(const BMessage *archive) const
{
	return replicant_data::IndexOf(&fReplicants, archive);
}


int32
BShelf::IndexOf(uint32 id) const
{
	return replicant_data::IndexOf(&fReplicants, id);
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
_ReservedShelf1__6BShelfFv(BShelf *const, int32, const BMessage*, const BView*)
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

	fFilter = new ShelfContainerViewFilter(this, fContainerView);

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
BShelf::_DeleteReplicant(replicant_data* item)
{
	bool loadedImage = item->message->FindBool("");

	BView *view = item->view;
	if (view == NULL)
		view = item->zombie_view;

	if (view)
		view->RemoveSelf();

	if (item->dragger)
		item->dragger->RemoveSelf();

	int32 index = replicant_data::IndexOf(&fReplicants, item->message);

	// TODO: Test if it's ok here
	ReplicantDeleted(index, item->message, view);

	fReplicants.RemoveItem(item);

	if (loadedImage && item->image >= 0)
		unload_add_on(item->image);

	delete item;

	return B_OK;
}


status_t
BShelf::_AddReplicant(BMessage *data, BPoint *location, uint32 uniqueID)
{
	// Check shelf types if needed
	if (fTypeEnforced) {
		const char *shelfType = NULL;
		if (data->FindString("shelf_type", &shelfType) == B_OK && shelfType != NULL) {
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
		const char *className = NULL;
		const char *addOn = NULL;

		if (data->FindString("class", &className) == B_OK
			&& data->FindString("add_on", &addOn) == B_OK) {
			int32 i = 0;
			replicant_data *item;
			const char *replicantClassName = NULL;
			const char *replicantAddOn = NULL;

			while ((item = (replicant_data*)fReplicants.ItemAt(i++)) != NULL) {
				if (item->message->FindString("class", &replicantClassName) == B_OK
					&& item->message->FindString("add_on", &replicantAddOn) == B_OK
					&& !strcmp(className, replicantClassName)
					&& addOn != NULL && replicantAddOn != NULL
					&& !strcmp(addOn, replicantAddOn)) {
					printf("Replicant was rejected. Unique replicant already exists. class=%s, signature=%s",
						replicantClassName, replicantAddOn);
					return send_reply(data, B_ERROR, uniqueID);
				}
			}
		}
	}

	BDragger* dragger = NULL;
	BView* replicant = NULL;
	BDragger::relation relation = BDragger::TARGET_UNKNOWN;
	_BZombieReplicantView_* zombie = NULL;
	
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
		BMessage widget;
		if (data->FindMessage("__widget", &widget) == B_OK) {
			image_id draggerImage = B_ERROR;
			replicant = view;
			dragger = dynamic_cast<BDragger*>(instantiate_object(&widget, &draggerImage));
			if (dragger != NULL) {
				// Replicant is either a sibling or unknown
				dragger->_SetViewToDrag(replicant);
				relation = BDragger::TARGET_IS_SIBLING;
			}
		} else {
			// Replicant is child of the dragger
			if ((dragger = dynamic_cast<BDragger*>(view)) != NULL) {
				replicant = dragger->ChildAt(0);
				dragger->_SetViewToDrag(replicant);
				relation = BDragger::TARGET_IS_CHILD;
			} else {
				// Replicant is parent of the dragger
				replicant = view;
				dragger = dynamic_cast<BDragger*>(replicant->FindView("_dragger_"));

				if (dragger)
					dragger->_SetViewToDrag(replicant);

				relation = BDragger::TARGET_IS_PARENT;
			}
		}

		if (dragger != NULL)
			dragger->_SetShelf(this);

		BRect frame;
		if (relation != BDragger::TARGET_IS_CHILD) {
			frame = replicant->Frame().OffsetToCopy(point);
			fContainerView->AddChild(replicant);
		} else
			frame = dragger->Frame().OffsetToCopy(point);

		if (!CanAcceptReplicantView(frame, replicant, data)) {
			// the view has not been accepted

			if (relation == BDragger::TARGET_IS_PARENT
				|| relation == BDragger::TARGET_IS_SIBLING)
				delete replicant;
			if (relation == BDragger::TARGET_IS_CHILD
				|| relation == BDragger::TARGET_IS_SIBLING)
				delete dragger;

			return send_reply(data, B_ERROR, uniqueID);
		}

		BPoint adjust = AdjustReplicantBy(frame, data);
		
		// TODO: that's probably not correct for all relations (or any?)
		view->MoveTo(point + adjust);	

		// if it's a sibling or a child, we need to add the dragger
		if (relation == BDragger::TARGET_IS_SIBLING || relation == BDragger::TARGET_IS_CHILD)
			fContainerView->AddChild(dragger);

		replicant->AddFilter(new ReplicantViewFilter(this, replicant));
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

			dragger = new BDragger(frame, zombie);
			dragger->_SetShelf(this);
			dragger->_SetZombied(true);

			zombie->AddChild(dragger);
			zombie->SetArchive(data);
			zombie->AddFilter(new ReplicantViewFilter(this, zombie));

			fContainerView->AddChild(zombie);
		}
	}

	data->RemoveName("_drop_point_");
	data->RemoveName("_drop_offset_");

	replicant_data *item = new replicant_data(data, replicant, dragger, relation,
		uniqueID, image);

	item->error = B_OK;
	item->zombie_view = zombie;

	fReplicants.AddItem(item);

	return send_reply(data, B_OK, uniqueID);
}


status_t
BShelf::_GetProperty(BMessage *msg, BMessage *reply)
{
	uint32 ID;
	status_t err = B_ERROR;
	BView *replicant = NULL;
	switch (msg->what) {
		case B_INDEX_SPECIFIER:	{
			int32 index = -1;
			if (msg->FindInt32("index", &index)!=B_OK) 
				break;
			ReplicantAt(index, &replicant, &ID, &err);
			break;
		}
		case B_REVERSE_INDEX_SPECIFIER:	{
			int32 rindex;
			if (msg->FindInt32("index", &rindex) != B_OK)
				break;
			ReplicantAt(CountReplicants() - rindex, &replicant, &ID, &err);
			break;
		}
		case B_NAME_SPECIFIER: {
			const char *name;
			if (msg->FindString("name", &name) != B_OK)
				break;
			for (int32 i=0; i<CountReplicants(); i++) {
				BView *view = NULL;
				ReplicantAt(i, &view, &ID, &err);
				if (err == B_OK) {
					if (view->Name() != NULL && 
						strlen(view->Name()) == strlen(name) && !strcmp(view->Name(), name)) {
						replicant = view;
						break;
					}
					err = B_NAME_NOT_FOUND;
				}
			}
			break;
		}
		case B_ID_SPECIFIER: {
			uint32 id;
			if (msg->FindInt32("id", (int32 *)&id) != B_OK)
				break;
			for (int32 i=0; i<CountReplicants(); i++) {
				BView *view = NULL;
				ReplicantAt(i, &view, &ID, &err);
				if (err == B_OK) {
					if (ID == id) {
						replicant = view;
						break;
					}
					err = B_NAME_NOT_FOUND;
				}
			}
			break;
		}
		default:
			break;
	}

	if (replicant) {
		reply->AddInt32("index", IndexOf(replicant));
		reply->AddInt32("ID", ID);
	}

	return err;
}
