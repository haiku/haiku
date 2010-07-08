/*
 * Copyright (c) 2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Alex Wilson (yourpalal2@gmail.com)
 */


#include <syslog.h>
#include <typeinfo>

#include "ArchivingManagers.h"


namespace BPrivate {
namespace Archiving {
	const char* kArchiveCountField = "_managed_archive_count";
	const char* kArchivableField = "_managed_archivable";
	const char* kTokenField = "_managed_token";
}
}


using namespace BPrivate::Archiving;


BArchiveManager*
BManagerBase::ArchiveManager(const BMessage* archive)
{
	BManagerBase* manager = ManagerPointer(archive);
	if (!manager)
		return NULL;

	if (manager->fType == ARCHIVE_MANAGER)
		return static_cast<BArchiveManager*>(manager);

	debugger("Overlapping managed unarchive/archive sessions.");
	return NULL;
}


BUnarchiveManager*
BManagerBase::UnarchiveManager(const BMessage* archive)
{
	BManagerBase* manager = ManagerPointer(archive);
	if (!manager)
		return NULL;

	if (manager->fType == UNARCHIVE_MANAGER)
		return static_cast<BUnarchiveManager*>(manager);

	debugger("More calls to BUnarchiver::PrepareMessage()"
		" than BUnarchivers created.");

	return NULL;
}


// #pragma mark -


struct BArchiveManager::ArchiveInfo {
	ArchiveInfo()
		:
		token(-1),
		archive(NULL)
	{
	}


	~ArchiveInfo()
	{
		delete archive;
	}


	int32		token;
	BMessage*	archive;
};


BArchiveManager::BArchiveManager(const BArchiver* creator)
	:
	BManagerBase(creator->ArchiveMessage(), BManagerBase::ARCHIVE_MANAGER),
	fTokenMap(),
	fCreator(creator)
{
}


BArchiveManager::~BArchiveManager()
{
	fTopLevelArchive->AddInt32(kArchiveCountField, fTokenMap.size());
}


status_t
BArchiveManager::GetTokenForArchivable(BArchivable* archivable, int32& _token)
{
	if (!archivable) {
		_token = -42;
		return B_OK;
	}

	TokenMap::iterator it = fTokenMap.find(archivable);

	if (it == fTokenMap.end())
		return B_ENTRY_NOT_FOUND;

	_token = it->second.token;
	return B_OK;
}


status_t
BArchiveManager::ArchiveObject(BArchivable* archivable, bool deep)
{
	if (IsArchived(archivable)){
		debugger("BArchivable requested to be archived"
			" was previously archived.");
	}

	ArchiveInfo& info = fTokenMap[archivable];

	info.archive = new BMessage();
	info.token = fTokenMap.size() - 1;

	MarkArchive(info.archive);
	status_t err = archivable->Archive(info.archive, deep);

	if (err != B_OK) {
		fTokenMap.erase(archivable);
			// info.archive gets deleted here
	}

	return err;
}


bool
BArchiveManager::IsArchived(BArchivable* archivable)
{
	if (!archivable)
		return true;

	return fTokenMap.find(archivable) != fTokenMap.end();
}


status_t
BArchiveManager::ArchiverLeaving(const BArchiver* archiver)
{
	if (archiver == fCreator) {
		// first, we must sort the objects into the order they were archived in
		typedef std::pair<BMessage*, const BArchivable*> ArchivePair;
		ArchivePair pairs[fTokenMap.size()];

		for(TokenMap::iterator it = fTokenMap.begin(), end = fTokenMap.end();
				it != end; it++) {
			ArchiveInfo& info = it->second;
			pairs[info.token].first = info.archive;
			pairs[info.token].second = it->first;

			// make sure fTopLevelArchive isn't deleted
			if (info.archive == fTopLevelArchive)
				info.archive = NULL;
		}

		status_t err = B_ERROR;
		int32 count = fTokenMap.size();
		for (int32 i = 0; i < count; i++) {
			const ArchivePair& pair = pairs[i];
			err = pair.second->AllArchived(pair.first);

			if (err == B_OK && i > 0) {
				err = fTopLevelArchive->AddMessage(kArchivableField,
					pair.first);
			}

			if (err != B_OK) {
				syslog(LOG_ERR, "AllArchived failed for object of type %s.",
					typeid(*pairs[i].second).name());
				break;
			}
		}

		delete this;
		return err;
	}

	return B_OK;
}


void
BArchiveManager::RegisterArchivable(const BArchivable* archivable)
{
	if (fTokenMap.size() == 0) {
		ArchiveInfo& info = fTokenMap[archivable];
		info.archive = fTopLevelArchive;
		info.token = 0;
	}
}


// #pragma mark -


struct BUnarchiveManager::ArchiveInfo {
	ArchiveInfo()
		:
		archivable(NULL),
		archive()
	{
	}

	bool
	operator<(const ArchiveInfo& other)
	{
		return archivable < other.archivable;
	}

	BArchivable*	archivable;
	BMessage		archive;
};


// #pragma mark -


BUnarchiveManager::BUnarchiveManager(BMessage* archive)
	:
	BManagerBase(archive, BManagerBase::UNARCHIVE_MANAGER),
	fObjects(NULL),
	fObjectCount(0),
	fTokenInProgress(0),
	fRefCount(0)
{
	archive->FindInt32(kArchiveCountField, &fObjectCount);
	fObjects = new ArchiveInfo[fObjectCount];

	// fObjects[0] is a placeholder for the object that started
	// this unarchiving session.
	for (int32 i = 0; i < fObjectCount - 1; i++) {
		BMessage* into = &fObjects[i + 1].archive;
		status_t err = archive->FindMessage(kArchivableField, i, into);
		MarkArchive(into);

		if (err != B_OK)
			syslog(LOG_ERR, "Failed to find managed archivable");
	}
}


BUnarchiveManager::~BUnarchiveManager()
{
	delete[] fObjects;
}


status_t
BUnarchiveManager::ArchivableForToken(BArchivable** _archivable, int32 token)
{
	if (!_archivable || token > fObjectCount)
		return B_BAD_VALUE;

	if (token < 0) {
		*_archivable = NULL;
		return B_OK;
	}

	status_t err = B_OK;
	if (!fObjects[token].archivable) {
		if (fRefCount > 0)
			err = _InstantiateObjectForToken(token);
		else {
			syslog(LOG_ERR, "Object requested from AllUnarchived()"
				" was not previously instantiated");
			err = B_ERROR;
		}
	}

	*_archivable = fObjects[token].archivable;
	return err;
}


bool
BUnarchiveManager::IsInstantiated(int32 token)
{
	if (token < 0 || token >= fObjectCount)
		return false;
	return fObjects[token].archivable;
}


void
BUnarchiveManager::RegisterArchivable(BArchivable* archivable)
{
	if (!archivable)
		debugger("Cannot register NULL pointer");

	fObjects[fTokenInProgress].archivable = archivable;
}


status_t
BUnarchiveManager::UnarchiverLeaving(const BUnarchiver* unarchiver)
{
	if (--fRefCount == 0) {
		fRefCount = -1;
			// make sure we de not end up here again!

		BArchivable* archivable = fObjects[0].archivable;
		status_t err = archivable->AllUnarchived(fTopLevelArchive);

		for (int32 i = 1; i < fObjectCount; i++) {
			if (err != B_OK)
				break;

			archivable = fObjects[i].archivable;
			err = archivable->AllUnarchived(&fObjects[i].archive);
		}

		if (err != B_OK) {
			syslog(LOG_ERR, "AllUnarchived failed for object of type %s.",
				typeid(*archivable).name());
		}

		delete this;
		return err;
	}

	return B_OK;
}


void
BUnarchiveManager::Acquire()
{
	if (fRefCount >= 0)
		fRefCount++;
}


status_t
BUnarchiveManager::_InstantiateObjectForToken(int32 token)
{
	fTokenInProgress = token;
	if(!instantiate_object(&fObjects[token].archive))
		return B_ERROR;
	return B_OK;
}

