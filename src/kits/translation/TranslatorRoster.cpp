/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
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


#include "FuncTranslator.h"
#include "TranslatorRosterPrivate.h"

#include <Application.h>
#include <Autolock.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <image.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <String.h>
#include <TranslatorRoster.h>

#include <set>
#include <new>
#include <string.h>


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


BTranslatorRoster::Private::Private()
	: BHandler("translator roster"), BLocker("translator list"),
	fNextID(1)
{
	// we're sneaking us into the BApplication
	if (be_app != NULL)
		be_app->AddHandler(this);
}


BTranslatorRoster::Private::~Private()
{
	stop_watching(this);

	if (Looper())
		Looper()->RemoveHandler(this);

	// Release all translators, so that they can delete themselves

	TranslatorMap::iterator iterator = fTranslators.begin();
	std::set<image_id> images;

	while (iterator != fTranslators.end()) {
		BTranslator* translator = iterator->second.translator;
		
		translator->fOwningRoster = NULL;
			// we don't want to be notified about this anymore

		images.insert(iterator->second.image);
		translator->Release();
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
			printf("translator roster node monitor: ");
			message->PrintToStream();
			break;

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
		B_BEOS_ADDONS_DIRECTORY,
	};

	for (uint32 i = 0; i < sizeof(paths) / sizeof(paths[0]); i++) {
		BPath path;
		status_t status = find_directory(paths[i], &path, true);
		if (status == B_OK && path.Append("Translators") == B_OK)
			AddPath(path.Path());
	}
}


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


status_t
BTranslatorRoster::Private::AddPath(const char* path, int32* _added)
{
	BDirectory directory(path);
	status_t status = directory.InitCheck();
	if (status < B_OK)
		return status;

	node_ref nodeRef;
	status = directory.GetNodeRef(&nodeRef);
	if (status < B_OK)
		return status;

	if (Looper() != NULL) {
		// watch these directories
		watch_node(&nodeRef, B_WATCH_DIRECTORY, this);
	}

	int32 count = 0;
	int32 files = 0;

	entry_ref ref;
	while (directory.GetNextRef(&ref) == B_OK) {
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
	image_id image = -1, const entry_ref* ref = NULL)
{
	BAutolock locker(this);

	translator_item item;
	item.image = image;
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
BTranslatorRoster::Private::GetTranslatorData(image_id image, translator_data& data)
{
	// If this is a translator add-on, it is in the C format
	memset(&data, 0, sizeof(translator_data));

	// find all the symbols

	int32* version;
	if (get_image_symbol(image, "translatorName", B_SYMBOL_TYPE_DATA, (void**)&data.name) < B_OK
		|| get_image_symbol(image, "translatorInfo", B_SYMBOL_TYPE_DATA, (void**)&data.info) < B_OK
		|| get_image_symbol(image, "translatorVersion", B_SYMBOL_TYPE_DATA, (void**)&version) < B_OK || version == NULL
		|| get_image_symbol(image, "inputFormats", B_SYMBOL_TYPE_DATA, (void**)&data.input_formats) < B_OK
		|| get_image_symbol(image, "outputFormats", B_SYMBOL_TYPE_DATA, (void**)&data.output_formats) < B_OK
		|| get_image_symbol(image, "Identify", B_SYMBOL_TYPE_TEXT, (void**)&data.identify_hook) < B_OK
		|| get_image_symbol(image, "Translate", B_SYMBOL_TYPE_TEXT, (void**)&data.translate_hook) < B_OK
		|| get_image_symbol(image, "MakeConfig", B_SYMBOL_TYPE_TEXT, (void**)&data.make_config_hook) < B_OK
		|| get_image_symbol(image, "GetConfigMessage", B_SYMBOL_TYPE_TEXT, (void**)&data.get_config_message_hook) < B_OK)
		return B_BAD_TYPE;

	data.version = *version;
	return B_OK;
}


status_t
BTranslatorRoster::Private::CreateTranslators(const entry_ref& ref, int32& count)
{
	BAutolock locker(this);

	if (_FindTranslator(ref.name) != NULL) {
		// keep the existing add-on
		return B_OK;
	}

	BPath path(&ref);
	image_id image = load_add_on(path.Path());
	if (image < B_OK)
		return image;

	// Function pointer used to create post R4.5 style translators
	BTranslator *(*makeNthTranslator)(int32 n, image_id you, uint32 flags, ...);

	status_t status = get_image_symbol(image, "make_nth_translator",
		B_SYMBOL_TYPE_TEXT, (void**)&makeNthTranslator);
	if (status == B_OK) {
		// If the translator add-on supports the post R4.5
		// translator creation mechanism, keep loading translators
		// until MakeNthTranslator stops returning them.		
		BTranslator* translator = NULL;
		int32 created = 0;
		for (int32 n = 0; (translator = makeNthTranslator(n, image, 0)) != NULL; n++) {
			if (AddTranslator(translator, image, &ref) == B_OK) {
				count++;
				created++;
			} else {
				translator->Release();
					// this will delete the translator
			}
		}

		if (created == 0)
			unload_add_on(image);
		return B_OK;
	}

	// If this is a translator add-on, it is in the C format
	translator_data translatorData;
	status = GetTranslatorData(image, translatorData);

	// add this translator to the list
	BPrivate::BFuncTranslator* translator = NULL;
	if (status == B_OK) {
		translator = new (std::nothrow) BPrivate::BFuncTranslator(translatorData);
		if (translator == NULL)
			status = B_NO_MEMORY;
	}

	if (status == B_OK)
		status = AddTranslator(translator, image, &ref);

	if (status == B_OK)
		count++;
	else
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

	return B_OK;
}


status_t
BTranslatorRoster::Private::StopWatching(BMessenger target)
{
	MessengerList::iterator iterator = fMessengers.begin();
	
	while (iterator != fMessengers.end()) {
		if (*iterator == target) {
			fMessengers.erase(iterator);
			return B_OK;
		}
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

	TranslatorMap::const_iterator iterator = fTranslators.begin();
	float bestWeight = 0.0f;

	while (iterator != fTranslators.end()) {
		BTranslator& translator = *iterator->second.translator;

		status_t status = source->Seek(0, SEEK_SET);
		if (status != B_OK)
			return status;

		int32 formatsCount = 0;
		const translation_format* formats = translator.InputFormats(&formatsCount);
		const translation_format* format = _CheckHints(formats, formatsCount, hintType,
			hintMIME);

		translator_info info;
		if (translator.Identify(source, format, ioExtension, &info, wantType) == B_OK) {
			float weight = info.quality * info.capability;
			if (weight > bestWeight) {
				bestWeight = weight;

				info.translator = iterator->first;
				memcpy(_info, &info, sizeof(translator_info));
			}
		}
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

	int32 arraySize = fTranslators.size();
	translator_info* array = new (std::nothrow) translator_info[arraySize];
	if (array == NULL)
		return B_NO_MEMORY;

	TranslatorMap::const_iterator iterator = fTranslators.begin();
	int32 count = 0;

	while (iterator != fTranslators.end()) {
		BTranslator& translator = *iterator->second.translator;

		status_t status = source->Seek(0, SEEK_SET);
		if (status < B_OK) {
			delete[] array;
			return status;
		}

		int32 formatsCount = 0;
		const translation_format* formats = translator.InputFormats(&formatsCount);
		const translation_format* format = _CheckHints(formats, formatsCount, hintType,
			hintMIME);

		translator_info info;
		if (translator.Identify(source, format, ioExtension, &info, wantType) == B_OK) {
			info.translator = iterator->first;
			array[count++] = info;
		}
	}

	*_info = array;
	*_numInfo = count;
	qsort(array, count, sizeof(translator_info), BTranslatorRoster::Private::_CompareSupport);
		// translators are sorted by best support

	return B_OK;
}


status_t
BTranslatorRoster::Private::GetAllTranslators(translator_id** _ids, int32* _count)
{
	BAutolock locker(this);

	int32 arraySize = fTranslators.size();
	translator_id* array = new (std::nothrow) translator_id[arraySize];
	if (array == NULL)
		return B_NO_MEMORY;

	TranslatorMap::const_iterator iterator = fTranslators.begin();
	int32 count = 0;

	while (iterator != fTranslators.end()) {
		array[count++] = iterator->first;
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


const translation_format*
BTranslatorRoster::Private::_CheckHints(const translation_format* formats,
	int32 formatsCount, uint32 hintType, const char* hintMIME)
{
	if (!formats || formatsCount <= 0 || (!hintType && !hintMIME))
		return NULL;

	// The provided MIME type hint may be a super type
	int32 super = 0;
	if (hintMIME && !strchr(hintMIME, '/'))
		super = strlen(hintMIME);

	// scan for suitable format
	for (int32 i = 0; i < formatsCount && formats[i].type; i++) {
		if (formats[i].type == hintType
			|| hintMIME && ((super && !strncmp(formats[i].MIME, hintMIME, super))
				|| !strcmp(formats[i].MIME, hintMIME)))
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


//	#pragma mark -


BTranslatorRoster::BTranslatorRoster()
{
	_Initialize();
}


BTranslatorRoster::BTranslatorRoster(BMessage *model)
{
	_Initialize();

	if (model) {
		const char* path;
		for (int32 i = 0; model->FindString("be:translator_path", i, &path) == B_OK; i++) {
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


BArchivable *
BTranslatorRoster::Instantiate(BMessage* from)
{
	if (!from || !validate_instantiation(from, "BTranslatorRoster"))
		return NULL;

	return new BTranslatorRoster(from);
}


BTranslatorRoster *
BTranslatorRoster::Default()
{
// TODO: This code isn't thread safe
	// If the default translators have not been loaded,
	// create a new BTranslatorRoster for them, and load them.
	if (sDefaultRoster == NULL) {
		sDefaultRoster = new BTranslatorRoster();
		sDefaultRoster->AddTranslators(NULL);
	}

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
	BTranslator *(*makeNthTranslator)(int32 n, image_id you, uint32 flags, ...);

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
	if (!source || !_info)
		return B_BAD_VALUE;

	return fPrivate->Identify(source, ioExtension, hintType, hintMIME, wantType, _info);
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
		status_t status = Identify(source, ioExtension, &infoBuffer,
			hintType, hintMIME, wantOutType);
		if (status < B_OK)
			return status;

		info = &infoBuffer;
	}

	if (!fPrivate->Lock())
		return B_ERROR;

	BTranslator* translator = fPrivate->FindTranslator(info->translator);
	if (translator != NULL) {
		translator->Acquire();
			// make sure this translator is not removed while we're playing with it;
			// translating shouldn't be serialized!
	}

	fPrivate->Unlock();

	if (translator == NULL)
		return B_NO_TRANSLATOR;

	status_t status = source->Seek(0, SEEK_SET);
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

	BTranslator* translator = fPrivate->FindTranslator(id);
	if (translator != NULL) {
		translator->Acquire();
			// make sure this translator is not removed while we're playing with it;
			// translating shouldn't be serialized!
	}

	fPrivate->Unlock();

	if (translator == NULL)
		return B_NO_TRANSLATOR;

	status_t status = source->Seek(0, SEEK_SET);
	if (status == B_OK) {
		translator_info info;
		status = translator->Identify(source, NULL, ioExtension, &info, wantOutType);	
		if (status >= B_OK) {
			status = translator->Translate(source, &info, ioExtension, wantOutType,
				destination);
		}
	}
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
BTranslatorRoster::MakeConfigurationView(translator_id id, BMessage* ioExtension,
	BView** _view, BRect* _extent)
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
BTranslatorRoster::GetConfigurationMessage(translator_id id, BMessage* ioExtension)
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


const char *
Version__17BTranslatorRosterPlT1l(int32 *outCurVersion, int32 *outMinVersion,
	int32 inAppVersion)
{
	if (!outCurVersion || !outMinVersion)
		return "";

	static char vString[50];
	static char vDate[] = __DATE__;
	if (!vString[0]) {
		sprintf(vString, "Translation Kit v%d.%d.%d %s\n",
			static_cast<int>(B_TRANSLATION_MAJOR_VERSION(B_TRANSLATION_CURRENT_VERSION)),
			static_cast<int>(B_TRANSLATION_MINOR_VERSION(B_TRANSLATION_CURRENT_VERSION)),
			static_cast<int>(B_TRANSLATION_REVISION_VERSION(B_TRANSLATION_CURRENT_VERSION)),
			vDate);
	}
	*outCurVersion = B_TRANSLATION_CURRENT_VERSION;
	*outMinVersion = B_TRANSLATION_MIN_VERSION;
	return vString;
}


void BTranslatorRoster::ReservedTranslatorRoster1() {}
void BTranslatorRoster::ReservedTranslatorRoster2() {}
void BTranslatorRoster::ReservedTranslatorRoster3() {}
void BTranslatorRoster::ReservedTranslatorRoster4() {}
void BTranslatorRoster::ReservedTranslatorRoster5() {}
void BTranslatorRoster::ReservedTranslatorRoster6() {}
