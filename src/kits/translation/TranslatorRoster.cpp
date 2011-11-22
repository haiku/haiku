/*
 * Copyright 2002-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */

/*!
	This class is the guts of the translation kit, it makes the
	whole thing happen. It bridges the applications using this
	object with the translators that the apps need to access.
*/

#include <TranslatorRoster.h>

#include <new>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <Application.h>
#include <Autolock.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>

#include <driver_settings.h>
#include <image.h>
#include <safemode_defs.h>
#include <syscalls.h>

#include "FuncTranslator.h"
#include "TranslatorRosterPrivate.h"


namespace BPrivate {

class QuarantineTranslatorImage {
public:
								QuarantineTranslatorImage(
									BTranslatorRoster::Private& privateRoster);
								~QuarantineTranslatorImage();

			void				Put(const entry_ref& ref);
			void				Remove();

private:
			BTranslatorRoster::Private& fRoster;
			entry_ref			fRef;
			bool				fRemove;
};

}	// namespace BPrivate

// Extensions used in the extension BMessage, defined in TranslatorFormats.h
char B_TRANSLATOR_EXT_HEADER_ONLY[]			= "/headerOnly";
char B_TRANSLATOR_EXT_DATA_ONLY[]			= "/dataOnly";
char B_TRANSLATOR_EXT_COMMENT[]				= "/comment";
char B_TRANSLATOR_EXT_TIME[]				= "/time";
char B_TRANSLATOR_EXT_FRAME[]				= "/frame";
char B_TRANSLATOR_EXT_BITMAP_RECT[]			= "bits/Rect";
char B_TRANSLATOR_EXT_BITMAP_COLOR_SPACE[]	= "bits/space";
char B_TRANSLATOR_EXT_BITMAP_PALETTE[]		= "bits/palette";
char B_TRANSLATOR_EXT_SOUND_CHANNEL[]		= "nois/channel";
char B_TRANSLATOR_EXT_SOUND_MONO[]			= "nois/mono";
char B_TRANSLATOR_EXT_SOUND_MARKER[]		= "nois/marker";
char B_TRANSLATOR_EXT_SOUND_LOOP[]			= "nois/loop";

BTranslatorRoster* BTranslatorRoster::sDefaultRoster = NULL;


namespace BPrivate {

/*!
	The purpose of this class is to put a translator entry_ref into - and remove
	it from the list of translators on destruction (if Remove() was called
	before).

	This is used in Private::CreateTranslators() in case a translator hides a
	previous one (ie. if you install a translator in the user's translators
	directory that has the same name as one in the system's directory, it will
	hide this entry).
*/
QuarantineTranslatorImage::QuarantineTranslatorImage(
	BTranslatorRoster::Private& privateRoster)
	:
	fRoster(privateRoster),
	fRemove(false)
{
}


QuarantineTranslatorImage::~QuarantineTranslatorImage()
{
	if (fRef.device == -1 || !fRemove)
		return;

	fRoster.RemoveTranslators(fRef);
}


void
QuarantineTranslatorImage::Put(const entry_ref& ref)
{
	fRef = ref;
}


void
QuarantineTranslatorImage::Remove()
{
	fRemove = true;
}

}	// namespace BPrivate


//	#pragma mark -


BTranslatorRoster::Private::Private()
	:
	BHandler("translator roster"),
	BLocker("translator list"),
	fABISubDirectory(NULL),
	fNextID(1),
	fLazyScanning(true),
	fSafeMode(false)
{
	char parameter[32];
	size_t parameterLength = sizeof(parameter);

	if (_kern_get_safemode_option(B_SAFEMODE_SAFE_MODE, parameter,
			&parameterLength) == B_OK) {
		if (!strcasecmp(parameter, "enabled") || !strcasecmp(parameter, "on")
			|| !strcasecmp(parameter, "true") || !strcasecmp(parameter, "yes")
			|| !strcasecmp(parameter, "enable") || !strcmp(parameter, "1"))
			fSafeMode = true;
	}

	if (_kern_get_safemode_option(B_SAFEMODE_DISABLE_USER_ADD_ONS, parameter,
			&parameterLength) == B_OK) {
		if (!strcasecmp(parameter, "enabled") || !strcasecmp(parameter, "on")
			|| !strcasecmp(parameter, "true") || !strcasecmp(parameter, "yes")
			|| !strcasecmp(parameter, "enable") || !strcmp(parameter, "1"))
			fSafeMode = true;
	}

	// We might run in compatibility mode on a system with a different ABI. The
	// translators matching our ABI can usually be found in respective
	// subdirectories of the translator directories.
	system_info info;
	if (get_system_info(&info) == B_OK
		&& (info.abi & B_HAIKU_ABI_MAJOR)
			!= (B_HAIKU_ABI & B_HAIKU_ABI_MAJOR)) {
			switch (B_HAIKU_ABI & B_HAIKU_ABI_MAJOR) {
				case B_HAIKU_ABI_GCC_2:
					fABISubDirectory = "gcc2";
					break;
				case B_HAIKU_ABI_GCC_4:
					fABISubDirectory = "gcc4";
					break;
			}
	}

	// we're sneaking us into the BApplication
	if (be_app != NULL && be_app->Lock()) {
		be_app->AddHandler(this);
		be_app->Unlock();
	}
}


BTranslatorRoster::Private::~Private()
{
	stop_watching(this);

	if (Looper() && LockLooper()) {
		BLooper* looper = Looper();
		Looper()->RemoveHandler(this);
		looper->Unlock();
	}

	// Release all translators, so that they can delete themselves

	TranslatorMap::iterator iterator = fTranslators.begin();
	std::set<image_id> images;

	while (iterator != fTranslators.end()) {
		BTranslator* translator = iterator->second.translator;

		translator->fOwningRoster = NULL;
			// we don't want to be notified about this anymore

		images.insert(iterator->second.image);
		translator->Release();

		iterator++;
	}

	// Unload all images

	std::set<image_id>::const_iterator imageIterator = images.begin();

	while (imageIterator != images.end()) {
		unload_add_on(*imageIterator);
		imageIterator++;
	}
}


void
BTranslatorRoster::Private::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_NODE_MONITOR:
		{
			BAutolock locker(this);

			printf("translator roster node monitor: ");
			message->PrintToStream();

			int32 opcode;
			if (message->FindInt32("opcode", &opcode) != B_OK)
				return;

			switch (opcode) {
				case B_ENTRY_CREATED:
				{
					const char* name;
					node_ref nodeRef;
					if (message->FindInt32("device", &nodeRef.device) != B_OK
						|| message->FindInt64("directory", &nodeRef.node)
							!= B_OK
						|| message->FindString("name", &name) != B_OK)
						break;

					// TODO: make this better (possible under Haiku)
					snooze(100000);
						// let the font be written completely before trying to
						// open it

					_EntryAdded(nodeRef, name);
					break;
				}

				case B_ENTRY_MOVED:
				{
					// has the entry been moved into a monitored directory or
					// has it been removed from one?
					const char* name;
					node_ref toNodeRef;
					node_ref fromNodeRef;
					node_ref nodeRef;

					if (message->FindInt32("device", &nodeRef.device) != B_OK
						|| message->FindInt64("to directory", &toNodeRef.node)
							!= B_OK
						|| message->FindInt64("from directory",
							(int64*)&fromNodeRef.node) != B_OK
						|| message->FindInt64("node", (int64*)&nodeRef.node)
							!= B_OK
						|| message->FindString("name", &name) != B_OK)
						break;

					fromNodeRef.device = nodeRef.device;
					toNodeRef.device = nodeRef.device;

					// Do we know this one yet?
					translator_item* item = _FindTranslator(nodeRef);
					if (item == NULL) {
						// it's a new one!
						if (_IsKnownDirectory(toNodeRef))
							_EntryAdded(toNodeRef, name);
						break;
					}

					if (!_IsKnownDirectory(toNodeRef)) {
						// translator got removed
						_RemoveTranslators(&nodeRef);
						break;
					}

					// the name may have changed
					item->ref.set_name(name);
					item->ref.directory = toNodeRef.node;

					if (_IsKnownDirectory(fromNodeRef)
						&& _IsKnownDirectory(toNodeRef)) {
						// TODO: we should rescan for the name, there might be
						// name clashes with translators in other directories
						// (as well as old ones revealed)
						break;
					}
					break;
				}

				case B_ENTRY_REMOVED:
				{
					node_ref nodeRef;
					uint64 directoryNode;
					if (message->FindInt32("device", &nodeRef.device) != B_OK
						|| message->FindInt64("directory",
							(int64*)&directoryNode) != B_OK
						|| message->FindInt64("node", &nodeRef.node) != B_OK)
						break;

					translator_item* item = _FindTranslator(nodeRef);
					if (item != NULL)
						_RemoveTranslators(&nodeRef);
					break;
				}
			}
			break;
		}

		default:
			BHandler::MessageReceived(message);
			break;
	}
}


void
BTranslatorRoster::Private::AddDefaultPaths()
{
	// add user directories first, so that they can override system translators
	const directory_which paths[] = {
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY,
	};

	for (uint32 i = fSafeMode ? 4 : 0; i < sizeof(paths) / sizeof(paths[0]);
			i++) {
		BPath path;
		status_t status = find_directory(paths[i], &path, true);
		if (status == B_OK && path.Append("Translators") == B_OK) {
			mkdir(path.Path(), 0755);
				// make sure the directory exists before we add it
			AddPath(path.Path());
		}
	}
}


/*!
	Adds the colon separated list of directories to the roster.

	Note, the order in which these directories are added to actually matters,
	translators with the same name will be taken from the earlier directory
	first. See _CompareTranslatorDirectoryPriority().
*/
status_t
BTranslatorRoster::Private::AddPaths(const char* paths)
{
	if (paths == NULL)
		return B_BAD_VALUE;

	status_t status = B_OK;
	int32 added = 0;

	while (paths != NULL) {
		const char* end = strchr(paths, ':');
		BString path;

		if (end != NULL) {
			path.SetTo(paths, end - 1 - paths);
			paths = end + 1;
		} else {
			path.SetTo(paths);
			paths = NULL;
		}

		// Keep the last error that occured, and return it
		// but don't overwrite it, if the last path was
		// added successfully.
		int32 count;
		status_t error = AddPath(path.String(), &count);
		if (error != B_NO_ERROR)
			status = error;

		added += count;
	}

	if (added == 0)
		return status;

	return B_OK;
}


/*!
	Adds a new directory to the roster.

	Note, the order in which these directories are added to actually matters,
	see AddPaths().
*/
status_t
BTranslatorRoster::Private::AddPath(const char* path, int32* _added)
{
	BDirectory directory(path);
	status_t status = directory.InitCheck();
	if (status != B_OK)
		return status;

	// if a subdirectory for our ABI exists, use that instead
	if (fABISubDirectory != NULL) {
		BEntry entry(&directory, fABISubDirectory);
		if (entry.IsDirectory()) {
			status = directory.SetTo(&entry);
			if (status != B_OK)
				return status;
		}
	}

	node_ref nodeRef;
	status = directory.GetNodeRef(&nodeRef);
	if (status < B_OK)
		return status;

	// do we know this directory already?
	if (_IsKnownDirectory(nodeRef))
		return B_OK;

	if (Looper() != NULL) {
		// watch that directory
		watch_node(&nodeRef, B_WATCH_DIRECTORY, this);
		fDirectories.push_back(nodeRef);
	}

	int32 count = 0;
	int32 files = 0;

	entry_ref ref;
	while (directory.GetNextRef(&ref) == B_OK) {
		BEntry entry(&ref);
		if (entry.IsDirectory())
			continue;
		if (CreateTranslators(ref, count) == B_OK)
			count++;

		files++;
	}

	if (_added)
		*_added = count;

	if (files != 0 && count == 0)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
BTranslatorRoster::Private::AddTranslator(BTranslator* translator,
	image_id image, const entry_ref* ref, ino_t node)
{
	BAutolock locker(this);

	translator_item item;
	item.translator = translator;
	item.image = image;
	item.node = node;
	if (ref != NULL)
		item.ref = *ref;

	try {
		fTranslators[fNextID] = item;
	} catch (...) {
		return B_NO_MEMORY;
	}

	translator->fOwningRoster = this;
	translator->fID = fNextID++;
	return B_OK;
}


void
BTranslatorRoster::Private::RemoveTranslators(entry_ref& ref)
{
	_RemoveTranslators(NULL, &ref);
}


BTranslator*
BTranslatorRoster::Private::FindTranslator(translator_id id)
{
	if (!IsLocked()) {
		debugger("translator must be locked!");
		return NULL;
	}

	const translator_item* item = _FindTranslator(id);
	if (item != NULL)
		return item->translator;

	return NULL;
}


status_t
BTranslatorRoster::Private::GetTranslatorData(image_id image,
	translator_data& data)
{
	// If this is a translator add-on, it is in the C format
	memset(&data, 0, sizeof(translator_data));

	// find all the symbols

	int32* version;
	if (get_image_symbol(image, "translatorName", B_SYMBOL_TYPE_DATA,
			(void**)&data.name) < B_OK
		|| get_image_symbol(image, "translatorInfo", B_SYMBOL_TYPE_DATA,
			(void**)&data.info) < B_OK
		|| get_image_symbol(image, "translatorVersion", B_SYMBOL_TYPE_DATA,
			(void**)&version) < B_OK || version == NULL
		|| get_image_symbol(image, "inputFormats", B_SYMBOL_TYPE_DATA,
			(void**)&data.input_formats) < B_OK
		|| get_image_symbol(image, "outputFormats", B_SYMBOL_TYPE_DATA,
			(void**)&data.output_formats) < B_OK
		|| get_image_symbol(image, "Identify", B_SYMBOL_TYPE_TEXT,
			(void**)&data.identify_hook) < B_OK
		|| get_image_symbol(image, "Translate", B_SYMBOL_TYPE_TEXT,
			(void**)&data.translate_hook) < B_OK) {
		return B_BAD_TYPE;
	}

	data.version = *version;

	// those calls are optional
	get_image_symbol(image, "MakeConfig", B_SYMBOL_TYPE_TEXT,
		(void**)&data.make_config_hook);
	get_image_symbol(image, "GetConfigMessage", B_SYMBOL_TYPE_TEXT,
		(void**)&data.get_config_message_hook);

	return B_OK;
}


status_t
BTranslatorRoster::Private::CreateTranslators(const entry_ref& ref,
	int32& count, BMessage* update)
{
	BAutolock locker(this);

	BPrivate::QuarantineTranslatorImage quarantine(*this);

	const translator_item* item = _FindTranslator(ref.name);
	if (item != NULL) {
		// check if the known translator has a higher priority
		if (_CompareTranslatorDirectoryPriority(item->ref, ref) <= 0) {
			// keep the existing add-on
			return B_OK;
		}

		// replace existing translator(s) if the new translator succeeds
		quarantine.Put(item->ref);
	}

	BEntry entry(&ref);
	node_ref nodeRef;
	status_t status = entry.GetNodeRef(&nodeRef);
	if (status < B_OK)
		return status;

	BPath path(&ref);
	image_id image = load_add_on(path.Path());
	if (image < B_OK)
		return image;

	// Function pointer used to create post R4.5 style translators
	BTranslator *(*makeNthTranslator)(int32 n, image_id you, uint32 flags, ...);

	status = get_image_symbol(image, "make_nth_translator",
		B_SYMBOL_TYPE_TEXT, (void**)&makeNthTranslator);
	if (status == B_OK) {
		// If the translator add-on supports the post R4.5
		// translator creation mechanism, keep loading translators
		// until MakeNthTranslator stops returning them.
		BTranslator* translator = NULL;
		int32 created = 0;
		for (int32 n = 0; (translator = makeNthTranslator(n, image, 0)) != NULL;
				n++) {
			if (AddTranslator(translator, image, &ref, nodeRef.node) == B_OK) {
				if (update)
					update->AddInt32("translator_id", translator->fID);
				count++;
				created++;
			} else {
				translator->Release();
					// this will delete the translator
			}
		}

		if (created == 0)
			unload_add_on(image);

		quarantine.Remove();
		return B_OK;
	}

	// If this is a translator add-on, it is in the C format
	translator_data translatorData;
	status = GetTranslatorData(image, translatorData);

	// add this translator to the list
	BPrivate::BFuncTranslator* translator = NULL;
	if (status == B_OK) {
		translator = new (std::nothrow) BPrivate::BFuncTranslator(
			translatorData);
		if (translator == NULL)
			status = B_NO_MEMORY;
	}

	if (status == B_OK)
		status = AddTranslator(translator, image, &ref, nodeRef.node);

	if (status == B_OK) {
		if (update)
			update->AddInt32("translator_id", translator->fID);
		quarantine.Remove();
		count++;
	} else
		unload_add_on(image);

	return status;
}


status_t
BTranslatorRoster::Private::StartWatching(BMessenger target)
{
	try {
		fMessengers.push_back(target);
	} catch (...) {
		return B_NO_MEMORY;
	}

	if (fLazyScanning) {
		fLazyScanning = false;
			// Since we now have someone to report to, we cannot lazily
			// adopt changes to the translator any longer

		_RescanChanged();
	}

	return B_OK;
}


status_t
BTranslatorRoster::Private::StopWatching(BMessenger target)
{
	MessengerList::iterator iterator = fMessengers.begin();

	while (iterator != fMessengers.end()) {
		if (*iterator == target) {
			fMessengers.erase(iterator);
			if (fMessengers.empty())
				fLazyScanning = true;

			return B_OK;
		}

		iterator++;
	}

	return B_BAD_VALUE;
}


status_t
BTranslatorRoster::Private::StoreTranslators(BMessage& archive)
{
	BAutolock locker(this);

	TranslatorMap::const_iterator iterator = fTranslators.begin();

	while (iterator != fTranslators.end()) {
		const translator_item& item = iterator->second;
		BPath path(&item.ref);
		if (path.InitCheck() == B_OK)
			archive.AddString("be:translator_path", path.Path());

		iterator++;
	}

	return B_OK;
}


status_t
BTranslatorRoster::Private::Identify(BPositionIO* source,
	BMessage* ioExtension, uint32 hintType, const char* hintMIME,
	uint32 wantType, translator_info* _info)
{
	BAutolock locker(this);

	_RescanChanged();

	TranslatorMap::const_iterator iterator = fTranslators.begin();
	BMessage baseExtension;
	if (ioExtension != NULL)
		baseExtension = *ioExtension;

	float bestWeight = 0.0f;

	while (iterator != fTranslators.end()) {
		BTranslator& translator = *iterator->second.translator;

		off_t pos = source->Seek(0, SEEK_SET);
		if (pos != 0)
			return pos < 0 ? (status_t)pos : B_IO_ERROR;

		int32 formatsCount = 0;
		const translation_format* formats = translator.InputFormats(
			&formatsCount);
		const translation_format* format = _CheckHints(formats, formatsCount,
			hintType, hintMIME);

		BMessage extension(baseExtension);
		translator_info info;
		if (translator.Identify(source, format, &extension, &info, wantType)
				== B_OK) {
			float weight = info.quality * info.capability;
			if (weight > bestWeight) {
				if (ioExtension != NULL)
					*ioExtension = extension;
				bestWeight = weight;

				info.translator = iterator->first;
				memcpy(_info, &info, sizeof(translator_info));
			}
		}

		iterator++;
	}

	if (bestWeight > 0.0f)
		return B_OK;

	return B_NO_TRANSLATOR;
}


status_t
BTranslatorRoster::Private::GetTranslators(BPositionIO* source,
	BMessage* ioExtension, uint32 hintType, const char* hintMIME,
	uint32 wantType, translator_info** _info, int32* _numInfo)
{
	BAutolock locker(this);

	_RescanChanged();

	int32 arraySize = fTranslators.size();
	translator_info* array = new (std::nothrow) translator_info[arraySize];
	if (array == NULL)
		return B_NO_MEMORY;

	TranslatorMap::const_iterator iterator = fTranslators.begin();
	int32 count = 0;

	while (iterator != fTranslators.end()) {
		BTranslator& translator = *iterator->second.translator;

		off_t pos = source->Seek(0, SEEK_SET);
		if (pos != 0) {
			delete[] array;
			return pos < 0 ? status_t(pos) : B_IO_ERROR;
		}

		int32 formatsCount = 0;
		const translation_format* formats = translator.InputFormats(
			&formatsCount);
		const translation_format* format = _CheckHints(formats, formatsCount,
			hintType, hintMIME);

		translator_info info;
		if (translator.Identify(source, format, ioExtension, &info, wantType)
				== B_OK) {
			info.translator = iterator->first;
			array[count++] = info;
		}

		iterator++;
	}

	*_info = array;
	*_numInfo = count;
	qsort(array, count, sizeof(translator_info),
		BTranslatorRoster::Private::_CompareSupport);
		// translators are sorted by best support

	return B_OK;
}


status_t
BTranslatorRoster::Private::GetAllTranslators(translator_id** _ids,
	int32* _count)
{
	BAutolock locker(this);

	_RescanChanged();

	int32 arraySize = fTranslators.size();
	translator_id* array = new (std::nothrow) translator_id[arraySize];
	if (array == NULL)
		return B_NO_MEMORY;

	TranslatorMap::const_iterator iterator = fTranslators.begin();
	int32 count = 0;

	while (iterator != fTranslators.end()) {
		array[count++] = iterator->first;
		iterator++;
	}

	*_ids = array;
	*_count = count;
	return B_OK;
}


status_t
BTranslatorRoster::Private::GetRefFor(translator_id id, entry_ref& ref)
{
	BAutolock locker(this);

	const translator_item* item = _FindTranslator(id);
	if (item == NULL)
		return B_NO_TRANSLATOR;

	BEntry entry(&item->ref);
	if (entry.InitCheck() == B_OK && entry.Exists() && entry.IsFile()) {
		ref = item->ref;
		return B_OK;
	}

	return B_ERROR;
}


void
BTranslatorRoster::Private::TranslatorDeleted(translator_id id)
{
	BAutolock locker(this);

	TranslatorMap::iterator iterator = fTranslators.find(id);
	if (iterator == fTranslators.end())
		return;

	fTranslators.erase(iterator);
}


/*static*/ int
BTranslatorRoster::Private::_CompareSupport(const void* _a, const void* _b)
{
	const translator_info* infoA = (const translator_info*)_a;
	const translator_info* infoB = (const translator_info*)_b;

	float weightA = infoA->quality * infoA->capability;
	float weightB = infoB->quality * infoB->capability;

	if (weightA == weightB)
		return 0;
	if (weightA > weightB)
		return -1;

	return 1;
}


/*!
	In lazy mode, freshly installed translator are not scanned immediately
	when they become available. Instead, they are put into a set.

	When a method is called that may be interested in these new translators,
	they are scanned on the fly. Since lazy mode also means that this roster
	does not have any listeners, we don't need to notify anyone about those
	changes.
*/
void
BTranslatorRoster::Private::_RescanChanged()
{
	while (!fRescanEntries.empty()) {
		EntryRefSet::iterator iterator = fRescanEntries.begin();
		int32 count;
		CreateTranslators(*iterator, count);

		fRescanEntries.erase(iterator);
	}
}


/*!
	Tests if the hints provided for a source stream are compatible to
	the formats the translator exports.
*/
const translation_format*
BTranslatorRoster::Private::_CheckHints(const translation_format* formats,
	int32 formatsCount, uint32 hintType, const char* hintMIME)
{
	if (formats == NULL || formatsCount <= 0 || (!hintType && hintMIME == NULL))
		return NULL;

	// The provided MIME type hint may be a super type
	int32 super = 0;
	if (hintMIME && !strchr(hintMIME, '/'))
		super = strlen(hintMIME);

	// scan for suitable format
	for (int32 i = 0; i < formatsCount && formats[i].type; i++) {
		if (formats[i].type == hintType
			|| (hintMIME
				&& ((super && !strncmp(formats[i].MIME, hintMIME, super))
					|| !strcmp(formats[i].MIME, hintMIME))))
			return &formats[i];
	}

	return NULL;
}


const translator_item*
BTranslatorRoster::Private::_FindTranslator(translator_id id) const
{
	TranslatorMap::const_iterator iterator = fTranslators.find(id);
	if (iterator == fTranslators.end())
		return NULL;

	return &iterator->second;
}


const translator_item*
BTranslatorRoster::Private::_FindTranslator(const char* name) const
{
	if (name == NULL)
		return NULL;

	TranslatorMap::const_iterator iterator = fTranslators.begin();

	while (iterator != fTranslators.end()) {
		const translator_item& item = iterator->second;
		if (item.ref.name != NULL && !strcmp(item.ref.name, name))
			return &item;

		iterator++;
	}

	return NULL;
}


const translator_item*
BTranslatorRoster::Private::_FindTranslator(entry_ref& ref) const
{
	if (ref.name == NULL)
		return NULL;

	TranslatorMap::const_iterator iterator = fTranslators.begin();

	while (iterator != fTranslators.end()) {
		const translator_item& item = iterator->second;
		if (item.ref == ref)
			return &item;

		iterator++;
	}

	return NULL;
}


translator_item*
BTranslatorRoster::Private::_FindTranslator(node_ref& nodeRef)
{
	if (nodeRef.device < 0)
		return NULL;

	TranslatorMap::iterator iterator = fTranslators.begin();

	while (iterator != fTranslators.end()) {
		translator_item& item = iterator->second;
		if (item.ref.device == nodeRef.device
			&& item.node == nodeRef.node)
			return &item;

		iterator++;
	}

	return NULL;
}


/*!
	Directories added to the roster have a certain priority - the first entry
	to be added has the highest priority; if a translator with the same name
	is to be found in two directories, the one with the higher priority is
	chosen.
*/
int32
BTranslatorRoster::Private::_CompareTranslatorDirectoryPriority(
	const entry_ref& a, const entry_ref& b) const
{
	// priority is determined by the order in the list

	node_ref nodeRefA;
	nodeRefA.device = a.device;
	nodeRefA.node = a.directory;

	node_ref nodeRefB;
	nodeRefB.device = b.device;
	nodeRefB.node = b.directory;

	NodeRefList::const_iterator iterator = fDirectories.begin();

	while (iterator != fDirectories.end()) {
		if (*iterator == nodeRefA)
			return -1;
		if (*iterator == nodeRefB)
			return 1;

		iterator++;
	}

	return 0;
}


bool
BTranslatorRoster::Private::_IsKnownDirectory(const node_ref& nodeRef) const
{
	NodeRefList::const_iterator iterator = fDirectories.begin();

	while (iterator != fDirectories.end()) {
		if (*iterator == nodeRef)
			return true;

		iterator++;
	}

	return false;
}


void
BTranslatorRoster::Private::_RemoveTranslators(const node_ref* nodeRef,
	const entry_ref* ref)
{
	if (ref == NULL && nodeRef == NULL)
		return;

	TranslatorMap::iterator iterator = fTranslators.begin();
	BMessage update(B_TRANSLATOR_REMOVED);
	image_id image = -1;

	while (iterator != fTranslators.end()) {
		TranslatorMap::iterator next = iterator;
		next++;

		const translator_item& item = iterator->second;
		if ((ref != NULL && item.ref == *ref)
			|| (nodeRef != NULL && item.ref.device == nodeRef->device
				&& item.node == nodeRef->node)) {
			item.translator->fOwningRoster = NULL;
				// if the translator is busy, we don't want to be notified
				// about the removal later on
			item.translator->Release();
			image = item.image;
			update.AddInt32("translator_id", iterator->first);

			fTranslators.erase(iterator);
		}

		iterator = next;
	}

	// Unload image from the removed translator

	if (image >= B_OK)
		unload_add_on(image);

	_NotifyListeners(update);
}


void
BTranslatorRoster::Private::_EntryAdded(const node_ref& nodeRef,
	const char* name)
{
	entry_ref ref;
	ref.device = nodeRef.device;
	ref.directory = nodeRef.node;
	ref.set_name(name);

	_EntryAdded(ref);
}


/*!
	In lazy mode, the entry is marked to be rescanned on next use of any
	translation method (that could make use of it).
	In non-lazy mode, the translators for this entry are created directly
	and listeners notified.

	Called by the node monitor handling.
*/
void
BTranslatorRoster::Private::_EntryAdded(const entry_ref& ref)
{
	BEntry entry;
	if (entry.SetTo(&ref) != B_OK || !entry.IsFile())
		return;

	if (fLazyScanning) {
		fRescanEntries.insert(ref);
		return;
	}

	BMessage update(B_TRANSLATOR_ADDED);
	int32 count = 0;
	CreateTranslators(ref, count, &update);

	_NotifyListeners(update);
}


void
BTranslatorRoster::Private::_NotifyListeners(BMessage& update) const
{
	MessengerList::const_iterator iterator = fMessengers.begin();

	while (iterator != fMessengers.end()) {
		(*iterator).SendMessage(&update);
		iterator++;
	}
}


//	#pragma mark -


BTranslatorRoster::BTranslatorRoster()
{
	_Initialize();
}


BTranslatorRoster::BTranslatorRoster(BMessage* model)
{
	_Initialize();

	if (model) {
		const char* path;
		for (int32 i = 0;
			model->FindString("be:translator_path", i, &path) == B_OK; i++) {
			BEntry entry(path);
			entry_ref ref;
			if (entry.GetRef(&ref) == B_OK) {
				int32 count = 0;
				fPrivate->CreateTranslators(ref, count);
			}
		}
	}
}


BTranslatorRoster::~BTranslatorRoster()
{
	// If the default BTranslatorRoster is being
	// deleted, set the pointer to the default
	// BTranslatorRoster to NULL
	if (sDefaultRoster == this)
		sDefaultRoster = NULL;

	delete fPrivate;
}


void
BTranslatorRoster::_Initialize()
{
	fPrivate = new BTranslatorRoster::Private();
}


status_t
BTranslatorRoster::Archive(BMessage* into, bool deep) const
{
	status_t status = BArchivable::Archive(into, deep);
	if (status != B_OK)
		return status;

	return fPrivate->StoreTranslators(*into);
}


BArchivable*
BTranslatorRoster::Instantiate(BMessage* from)
{
	if (!from || !validate_instantiation(from, "BTranslatorRoster"))
		return NULL;

	return new BTranslatorRoster(from);
}


BTranslatorRoster*
BTranslatorRoster::Default()
{
	static int32 lock = 0;

	if (sDefaultRoster != NULL)
		return sDefaultRoster;

	if (atomic_add(&lock, 1) != 0) {
		// Just wait for the default translator to be instantiated
		while (sDefaultRoster == NULL)
			snooze(10000);

		atomic_add(&lock, -1);
		return sDefaultRoster;
	}

	// If the default translators have not been loaded,
	// create a new BTranslatorRoster for them, and load them.
	if (sDefaultRoster == NULL) {
		BTranslatorRoster* roster = new BTranslatorRoster();
		roster->AddTranslators(NULL);

		sDefaultRoster = roster;
			// this will unlock any other threads waiting for
			// the default roster to become available
	}

	atomic_add(&lock, -1);
	return sDefaultRoster;
}


/*!
	This function takes a string of colon delimited paths, and adds
	the translators from those paths to this BTranslatorRoster.

	If load_path is NULL, it parses the environment variable
	TRANSLATORS. If that does not exist, it uses the system paths:
		/boot/home/config/add-ons/Translators,
		/system/add-ons/Translators.
*/
status_t
BTranslatorRoster::AddTranslators(const char* path)
{
	if (path == NULL)
		path = getenv("TRANSLATORS");
	if (path == NULL) {
		fPrivate->AddDefaultPaths();
		return B_OK;
	}

	return fPrivate->AddPaths(path);
}


/*!
	Adds a BTranslator based object to the BTranslatorRoster.
	When you add a BTranslator roster, it is Acquire()'d by
	BTranslatorRoster; it is Release()'d when the
	BTranslatorRoster is deleted.

	\param translator the translator to be added to the
		BTranslatorRoster

	\return B_BAD_VALUE, if translator is NULL,
		B_OK if all went well
*/
status_t
BTranslatorRoster::AddTranslator(BTranslator* translator)
{
	if (!translator)
		return B_BAD_VALUE;

	return fPrivate->AddTranslator(translator);
}


bool
BTranslatorRoster::IsTranslator(entry_ref* ref)
{
	if (ref == NULL)
		return false;

	BPath path(ref);
	image_id image = load_add_on(path.Path());
	if (image < B_OK)
		return false;

	// Function pointer used to create post R4.5 style translators
	BTranslator* (*makeNthTranslator)(int32 n, image_id you, uint32 flags, ...);

	status_t status = get_image_symbol(image, "make_nth_translator",
		B_SYMBOL_TYPE_TEXT, (void**)&makeNthTranslator);
	if (status < B_OK) {
		// If this is a translator add-on, it is in the C format
		translator_data translatorData;
		status = fPrivate->GetTranslatorData(image, translatorData);
	}

	unload_add_on(image);
	return status == B_OK;
}


/*!
	This function determines which translator is best suited
	to convert the data from \a source.

	\param source the data to be identified
	\param ioExtension the configuration data for the translator
	\param _info the information about the chosen translator is put here
	\param hintType a hint about the type of data that is in \a source, set
		it to zero if the type is not known
	\param hintMIME a hint about the MIME type of \a source, set it to NULL
		if the type is not known.
	\param wantType the desired output type - if zero, any type is okay.

	\return B_OK, identification of \a source was successful,
		B_NO_TRANSLATOR, no appropriate translator found,
		and other errors from accessing the source stream
*/
status_t
BTranslatorRoster::Identify(BPositionIO* source, BMessage* ioExtension,
	translator_info* _info, uint32 hintType, const char* hintMIME,
	uint32 wantType)
{
	if (source == NULL || _info == NULL)
		return B_BAD_VALUE;

	return fPrivate->Identify(source, ioExtension, hintType, hintMIME, wantType,
		_info);
}


/*!
	Finds all translators capable of handling the data in \a source
	and puts them into the outInfo array (which you must delete
	yourself when you are done with it). Specifying a value for
	\a hintType, \a hintMIME and/or \a wantType causes only the
	translators that satisfy them to be included in the outInfo.

	\param source the data to be translated
	\param ioExtension the configuration data for the translator
	\param _info, the array of acceptable translators is stored here if
		the function succeeds. It's the caller's responsibility to free
		the array using delete[].
	\param _numInfo, number of entries in the \a _info array
	\param hintType a hint about the type of data that is in \a source, set
		it to zero if the type is not known
	\param hintMIME a hint about the MIME type of \a source, set it to NULL
		if the type is not known.
	\param wantType the desired output type - if zero, any type is okay.

	\return B_OK, successfully indentified the data in \a source
		B_NO_TRANSLATOR, no translator could handle \a source
		other errors, problems using \a source
*/
status_t
BTranslatorRoster::GetTranslators(BPositionIO* source, BMessage* ioExtension,
	translator_info** _info, int32* _numInfo, uint32 hintType,
	const char* hintMIME, uint32 wantType)
{
	if (source == NULL || _info == NULL || _numInfo == NULL)
		return B_BAD_VALUE;

	return fPrivate->GetTranslators(source, ioExtension, hintType, hintMIME,
		wantType, _info, _numInfo);
}


/*!
	Returns an array in \a _ids of all of the translators stored by this
	object.
	You must free the array using delete[] when you are done with it.

	\param _ids the array is stored there (you own the array).
	\param _count number of IDs in the array.
*/
status_t
BTranslatorRoster::GetAllTranslators(translator_id** _ids, int32* _count)
{
	if (_ids == NULL || _count == NULL)
		return B_BAD_VALUE;

	return fPrivate->GetAllTranslators(_ids, _count);
}


/*!
	Returns information about the translator with the specified
	translator \a id.
	You must not free any of the data you get back.

	\param id identifies which translator you want info for
	\param _name the translator name is put here
	\param _info the translator description is put here
	\param _version the translation version is put here

	\return B_OK if successful,
		B_BAD_VALUE, if all parameters are NULL
		B_NO_TRANSLATOR, \id didn't identify an existing translator
*/
status_t
BTranslatorRoster::GetTranslatorInfo(translator_id id, const char** _name,
	const char** _info, int32* _version)
{
	if (_name == NULL && _info == NULL && _version == NULL)
		return B_BAD_VALUE;

	BAutolock locker(fPrivate);

	BTranslator* translator = fPrivate->FindTranslator(id);
	if (translator == NULL)
		return B_NO_TRANSLATOR;

	if (_name)
		*_name = translator->TranslatorName();
	if (_info)
		*_info = translator->TranslatorInfo();
	if (_version)
		*_version = translator->TranslatorVersion();

	return B_OK;
}


/*!
	Returns all of the input formats for the translator specified
	by \a id.
	You must not free any of the data you get back.

	\param id identifies which translator you want the input formats for
	\param _formats array of input formats
	\param _numFormats number of formats in the array

	\return B_OK if successful,
		B_BAD_VALUE, if any parameter is NULL
		B_NO_TRANSLATOR, \id didn't identify an existing translator
*/
status_t
BTranslatorRoster::GetInputFormats(translator_id id,
	const translation_format** _formats, int32* _numFormats)
{
	if (_formats == NULL || _numFormats == NULL)
		return B_BAD_VALUE;

	BAutolock locker(fPrivate);

	BTranslator* translator = fPrivate->FindTranslator(id);
	if (translator == NULL)
		return B_NO_TRANSLATOR;

	*_formats = translator->InputFormats(_numFormats);
	return B_OK;
}


/*!
	Returns all of the output formats for the translator specified
	by \a id.
	You must not free any of the data you get back.

	\param id identifies which translator you want the output formats for
	\param _formats array of output formats
	\param _numFormats number of formats in the array

	\return B_OK if successful,
		B_BAD_VALUE, if any parameter is NULL
		B_NO_TRANSLATOR, \id didn't identify an existing translator
*/
status_t
BTranslatorRoster::GetOutputFormats(translator_id id,
	const translation_format** _formats, int32* _numFormats)
{
	if (_formats == NULL || _numFormats == NULL)
		return B_BAD_VALUE;

	BAutolock locker(fPrivate);

	BTranslator* translator = fPrivate->FindTranslator(id);
	if (translator == NULL)
		return B_NO_TRANSLATOR;

	*_formats = translator->OutputFormats(_numFormats);
	return B_OK;
}


/*!
	This function is the whole point of the Translation Kit.
	This is for translating the data in \a source to \a destination
	using the format \a wantOutType.

	\param source the data to be translated
	\param ioExtension the configuration data for the translator
	\param info information about translator to use (can be NULL, in which
		case the \a source is identified first)
	\param destination where \a source is translated to
	\param hintType a hint about the type of data that is in \a source, set
		it to zero if the type is not known
	\param hintMIME a hint about the MIME type of \a source, set it to NULL
		if the type is not known.
	\param wantType the desired output type - if zero, any type is okay.

	\return B_OK, translation of \a source was successful,
		B_NO_TRANSLATOR, no appropriate translator found,
		and other errors from accessing the source and destination streams
*/
status_t
BTranslatorRoster::Translate(BPositionIO* source, const translator_info* info,
	BMessage* ioExtension, BPositionIO* destination, uint32 wantOutType,
	uint32 hintType, const char* hintMIME)
{
	if (source == NULL || destination == NULL)
		return B_BAD_VALUE;

	translator_info infoBuffer;

	if (info == NULL) {
		// look for a suitable translator
		status_t status = fPrivate->Identify(source, ioExtension, hintType,
			hintMIME, wantOutType, &infoBuffer);
		if (status < B_OK)
			return status;

		info = &infoBuffer;
	}

	if (!fPrivate->Lock())
		return B_ERROR;

	BTranslator* translator = fPrivate->FindTranslator(info->translator);
	if (translator != NULL) {
		translator->Acquire();
			// make sure this translator is not removed while we're playing with
			// it; translating shouldn't be serialized!
	}

	fPrivate->Unlock();

	if (translator == NULL)
		return B_NO_TRANSLATOR;

	status_t status = B_OK;
	off_t pos = source->Seek(0, SEEK_SET);
	if (pos != 0)
		status = pos < 0 ? (status_t)pos : B_IO_ERROR;
	if (status == B_OK) {
		status = translator->Translate(source, info, ioExtension, wantOutType,
			destination);
	}
	translator->Release();

	return status;
}


/*!
	This function is the whole point of the Translation Kit.
	This is for translating the data in \a source to \a destination
	using the format \a wantOutType and the translator identified
	by \a id.

	\param id the translator to be used
	\param source the data to be translated
	\param ioExtension the configuration data for the translator
	\param destination where \a source is translated to
	\param wantType the desired output type - if zero, any type is okay.

	\return B_OK, translation of \a source was successful,
		B_NO_TRANSLATOR, no appropriate translator found,
		and other errors from accessing the source and destination streams
*/
status_t
BTranslatorRoster::Translate(translator_id id, BPositionIO* source,
	BMessage* ioExtension, BPositionIO* destination, uint32 wantOutType)
{
	if (source == NULL || destination == NULL)
		return B_BAD_VALUE;

	if (!fPrivate->Lock())
		return B_ERROR;

	BTranslator* translator = fPrivate->FindTranslator(id);
	if (translator != NULL) {
		translator->Acquire();
			// make sure this translator is not removed while we're playing with
			// it; translating shouldn't be serialized!
	}

	fPrivate->Unlock();

	if (translator == NULL)
		return B_NO_TRANSLATOR;

	status_t status;
	off_t pos = source->Seek(0, SEEK_SET);
	if (pos == 0) {
		translator_info info;
		status = translator->Identify(source, NULL, ioExtension, &info,
			wantOutType);
		if (status >= B_OK) {
			off_t pos = source->Seek(0, SEEK_SET);
			if (pos != 0)
				status = pos < 0 ? (status_t)pos : B_IO_ERROR;
			else {
				status = translator->Translate(source, &info, ioExtension,
					wantOutType, destination);
			}
		}
	} else
		status = pos < 0 ? (status_t)pos : B_IO_ERROR;
	translator->Release();

	return status;
}


/*!
	Creates a BView in \a _view for configuring the translator specified
	by \a id. Not all translators support this, though.

	\param id identifies which translator you want the input formats for
	\param ioExtension the configuration data for the translator
	\param _view the view for configuring the translator
	\param _extent the bounds for the (resizable) view

	\return B_OK if successful,
		B_BAD_VALUE, if any parameter is NULL
		B_NO_TRANSLATOR, \id didn't identify an existing translator
*/
status_t
BTranslatorRoster::MakeConfigurationView(translator_id id,
	BMessage* ioExtension, BView** _view, BRect* _extent)
{
	if (_view == NULL || _extent == NULL)
		return B_BAD_VALUE;

	BAutolock locker(fPrivate);

	BTranslator* translator = fPrivate->FindTranslator(id);
	if (translator == NULL)
		return B_NO_TRANSLATOR;

	return translator->MakeConfigurationView(ioExtension, _view, _extent);
}


/*!
	Gets the configuration setttings for the translator
	specified by \a id and puts them into \a ioExtension.

	\param id identifies which translator you want the input formats for
	\param ioExtension the configuration data for the translator

	\return B_OK if successful,
		B_BAD_VALUE, if \a ioExtension is NULL
		B_NO_TRANSLATOR, \id didn't identify an existing translator
*/
status_t
BTranslatorRoster::GetConfigurationMessage(translator_id id,
	BMessage* ioExtension)
{
	if (!ioExtension)
		return B_BAD_VALUE;

	BAutolock locker(fPrivate);

	BTranslator* translator = fPrivate->FindTranslator(id);
	if (translator == NULL)
		return B_NO_TRANSLATOR;

	return translator->GetConfigurationMessage(ioExtension);
}


/*!
	Gets the entry_ref for the given translator (of course, this works only
	for disk based translators).

	\param id identifies which translator you want the input formats for
	\param ref the entry ref is stored there

	\return B_OK if successful,
		B_ERROR, if this is not a disk based translator
		B_BAD_VALUE, if \a ref is NULL
		B_NO_TRANSLATOR, \id didn't identify an existing translator
*/
status_t
BTranslatorRoster::GetRefFor(translator_id id, entry_ref* ref)
{
	if (ref == NULL)
		return B_BAD_VALUE;

	return fPrivate->GetRefFor(id, *ref);
}


status_t
BTranslatorRoster::StartWatching(BMessenger target)
{
	return fPrivate->StartWatching(target);
}


status_t
BTranslatorRoster::StopWatching(BMessenger target)
{
	return fPrivate->StopWatching(target);
}


//	#pragma mark - private


BTranslatorRoster::BTranslatorRoster(const BTranslatorRoster &other)
{
}


BTranslatorRoster &
BTranslatorRoster::operator=(const BTranslatorRoster &tr)
{
	return *this;
}


#if __GNUC__ == 2	// gcc 2

/*static*/ const char*
BTranslatorRoster::Version(int32* outCurVersion, int32* outMinVersion,
	int32 inAppVersion)
{
	if (!outCurVersion || !outMinVersion)
		return "";

	static char vString[50];
	static char vDate[] = __DATE__;
	if (!vString[0]) {
		sprintf(vString, "Translation Kit v%d.%d.%d %s\n",
			int(B_TRANSLATION_MAJOR_VERSION(B_TRANSLATION_CURRENT_VERSION)),
			int(B_TRANSLATION_MINOR_VERSION(B_TRANSLATION_CURRENT_VERSION)),
			int(B_TRANSLATION_REVISION_VERSION(B_TRANSLATION_CURRENT_VERSION)),
			vDate);
	}
	*outCurVersion = B_TRANSLATION_CURRENT_VERSION;
	*outMinVersion = B_TRANSLATION_MIN_VERSION;
	return vString;
}

#endif	// gcc 2


void BTranslatorRoster::ReservedTranslatorRoster1() {}
void BTranslatorRoster::ReservedTranslatorRoster2() {}
void BTranslatorRoster::ReservedTranslatorRoster3() {}
void BTranslatorRoster::ReservedTranslatorRoster4() {}
void BTranslatorRoster::ReservedTranslatorRoster5() {}
void BTranslatorRoster::ReservedTranslatorRoster6() {}
