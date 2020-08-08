/*
 * Copyright 2008-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2014, Axel Dörfler, axeld@pinc-software.de
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "CopyEngine.h"

#include <new>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

#include <Directory.h>
#include <fs_attr.h>
#include <NodeInfo.h>
#include <Path.h>
#include <SymLink.h>

#include "SemaphoreLocker.h"
#include "ProgressReporter.h"


using std::nothrow;


// #pragma mark - EntryFilter


CopyEngine::EntryFilter::~EntryFilter()
{
}


// #pragma mark - CopyEngine


CopyEngine::CopyEngine(ProgressReporter* reporter, EntryFilter* entryFilter)
	:
	fBufferQueue(),
	fWriterThread(-1),
	fQuitting(false),

	fAbsoluteSourcePath(),

	fBytesRead(0),
	fLastBytesRead(0),
	fItemsCopied(0),
	fLastItemsCopied(0),
	fTimeRead(0),

	fBytesWritten(0),
	fTimeWritten(0),

	fCurrentTargetFolder(NULL),
	fCurrentItem(NULL),

	fProgressReporter(reporter),
	fEntryFilter(entryFilter)
{
	fWriterThread = spawn_thread(_WriteThreadEntry, "buffer writer",
		B_NORMAL_PRIORITY, this);

	if (fWriterThread >= B_OK)
		resume_thread(fWriterThread);

	// ask for a bunch more file descriptors so that nested copying works well
	struct rlimit rl;
	rl.rlim_cur = 512;
	rl.rlim_max = RLIM_SAVED_MAX;
	setrlimit(RLIMIT_NOFILE, &rl);
}


CopyEngine::~CopyEngine()
{
	while (fBufferQueue.Size() > 0)
		snooze(10000);

	fQuitting = true;
	if (fWriterThread >= B_OK) {
		int32 exitValue;
		wait_for_thread(fWriterThread, &exitValue);
	}
}


void
CopyEngine::ResetTargets(const char* source)
{
	// TODO: One could subtract the bytes/items which were added to the
	// ProgressReporter before resetting them...

	fAbsoluteSourcePath = source;

	fBytesRead = 0;
	fLastBytesRead = 0;
	fItemsCopied = 0;
	fLastItemsCopied = 0;
	fTimeRead = 0;

	fBytesWritten = 0;
	fTimeWritten = 0;

	fCurrentTargetFolder = NULL;
	fCurrentItem = NULL;
}


status_t
CopyEngine::CollectTargets(const char* source, sem_id cancelSemaphore)
{
	off_t bytesToCopy = 0;
	uint64 itemsToCopy = 0;
	status_t ret = _CollectCopyInfo(source, cancelSemaphore, bytesToCopy,
			itemsToCopy);
	if (ret == B_OK && fProgressReporter != NULL)
		fProgressReporter->AddItems(itemsToCopy, bytesToCopy);
	return ret;
}


status_t
CopyEngine::Copy(const char* _source, const char* _destination,
	sem_id cancelSemaphore, bool copyAttributes)
{
	status_t ret;

	BEntry source(_source);
	ret = source.InitCheck();
	if (ret != B_OK)
		return ret;

	BEntry destination(_destination);
	ret = destination.InitCheck();
	if (ret != B_OK)
		return ret;

	return _Copy(source, destination, cancelSemaphore, copyAttributes);
}


status_t
CopyEngine::RemoveFolder(BEntry& entry)
{
	BDirectory directory(&entry);
	status_t ret = directory.InitCheck();
	if (ret != B_OK)
		return ret;

	BEntry subEntry;
	while (directory.GetNextEntry(&subEntry) == B_OK) {
		if (subEntry.IsDirectory()) {
			ret = CopyEngine::RemoveFolder(subEntry);
			if (ret != B_OK)
				return ret;
		} else {
			ret = subEntry.Remove();
			if (ret != B_OK)
				return ret;
		}
	}
	return entry.Remove();
}


status_t
CopyEngine::_CopyData(const BEntry& _source, const BEntry& _destination,
	sem_id cancelSemaphore)
{
	SemaphoreLocker lock(cancelSemaphore);
	if (cancelSemaphore >= 0 && !lock.IsLocked()) {
		// We are supposed to quit
		return B_CANCELED;
	}

	BFile source(&_source, B_READ_ONLY);
	status_t ret = source.InitCheck();
	if (ret < B_OK)
		return ret;

	BFile* destination = new (nothrow) BFile(&_destination,
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	ret = destination->InitCheck();
	if (ret < B_OK) {
		delete destination;
		return ret;
	}

	int32 loopIteration = 0;

	while (true) {
		if (fBufferQueue.Size() >= BUFFER_COUNT) {
			// the queue is "full", just wait a bit, the
			// write thread will empty it
			snooze(1000);
			continue;
		}

		// allocate buffer
		Buffer* buffer = new (nothrow) Buffer(destination);
		if (!buffer || !buffer->buffer) {
			delete destination;
			delete buffer;
			fprintf(stderr, "reading loop: out of memory\n");
			return B_NO_MEMORY;
		}

		// fill buffer
		ssize_t read = source.Read(buffer->buffer, buffer->size);
		if (read < 0) {
			ret = (status_t)read;
			fprintf(stderr, "Failed to read data: %s\n", strerror(ret));
			delete buffer;
			delete destination;
			break;
		}

		fBytesRead += read;
		loopIteration += 1;
		if (loopIteration % 2 == 0)
			_UpdateProgress();

		buffer->deleteFile = read == 0;
		if (read > 0)
			buffer->validBytes = (size_t)read;
		else
			buffer->validBytes = 0;

		// enqueue the buffer
		ret = fBufferQueue.Push(buffer);
		if (ret < B_OK) {
			buffer->deleteFile = false;
			delete buffer;
			delete destination;
			return ret;
		}

		// quit if done
		if (read == 0)
			break;
	}

	return ret;
}


// #pragma mark -


status_t
CopyEngine::_CollectCopyInfo(const char* _source, sem_id cancelSemaphore,
	off_t& bytesToCopy, uint64& itemsToCopy)
{
	BEntry source(_source);
	status_t ret = source.InitCheck();
	if (ret < B_OK)
		return ret;

	struct stat statInfo;
	ret = source.GetStat(&statInfo);
	if (ret < B_OK)
		return ret;

	SemaphoreLocker lock(cancelSemaphore);
	if (cancelSemaphore >= 0 && !lock.IsLocked()) {
		// We are supposed to quit
		return B_CANCELED;
	}

	if (fEntryFilter != NULL
		&& !fEntryFilter->ShouldCopyEntry(source,
			_RelativeEntryPath(_source), statInfo)) {
		// Skip this entry
		return B_OK;
	}

	if (cancelSemaphore >= 0)
		lock.Unlock();

	if (S_ISDIR(statInfo.st_mode)) {
		BDirectory srcFolder(&source);
		ret = srcFolder.InitCheck();
		if (ret < B_OK)
			return ret;

		BEntry entry;
		while (srcFolder.GetNextEntry(&entry) == B_OK) {
			BPath entryPath;
			ret = entry.GetPath(&entryPath);
			if (ret < B_OK)
				return ret;

			ret = _CollectCopyInfo(entryPath.Path(), cancelSemaphore,
					bytesToCopy, itemsToCopy);
			if (ret < B_OK)
				return ret;
		}
	} else if (S_ISLNK(statInfo.st_mode)) {
		// link, ignore size
	} else {
		bytesToCopy += statInfo.st_size;
	}

	itemsToCopy++;
	return B_OK;
}


status_t
CopyEngine::_Copy(BEntry &source, BEntry &destination,
	sem_id cancelSemaphore, bool copyAttributes)
{
	struct stat sourceInfo;
	status_t ret = source.GetStat(&sourceInfo);
	if (ret != B_OK)
		return ret;

	SemaphoreLocker lock(cancelSemaphore);
	if (cancelSemaphore >= 0 && !lock.IsLocked()) {
		// We are supposed to quit
		return B_CANCELED;
	}

	BPath sourcePath(&source);
	ret = sourcePath.InitCheck();
	if (ret != B_OK)
		return ret;

	BPath destPath(&destination);
	ret = destPath.InitCheck();
	if (ret != B_OK)
		return ret;

	const char *relativeSourcePath = _RelativeEntryPath(sourcePath.Path());
	if (fEntryFilter != NULL
		&& !fEntryFilter->ShouldCopyEntry(source, relativeSourcePath,
			sourceInfo)) {
		// Silently skip the filtered entry.
		return B_OK;
	}

	if (cancelSemaphore >= 0)
		lock.Unlock();

	bool copyAttributesToTarget = copyAttributes;
		// attributes of the current source to the destination will be copied
		// when copyAttributes is set to true, but there may be exceptions, so
		// allow the recursively used copyAttribute parameter to be overridden
		// for the current target.
	if (S_ISDIR(sourceInfo.st_mode)) {
		BDirectory sourceDirectory(&source);
		ret = sourceDirectory.InitCheck();
		if (ret != B_OK)
			return ret;

		if (destination.Exists()) {
			if (destination.IsDirectory()) {
				// Do not overwrite attributes on folders that exist.
				// This should work better when the install target
				// already contains a Haiku installation.
				copyAttributesToTarget = false;
			} else {
				ret = destination.Remove();
			}

			if (ret != B_OK) {
				fprintf(stderr, "Failed to make room for folder '%s': "
					"%s\n", sourcePath.Path(), strerror(ret));
				return ret;
			}
		}

		ret = create_directory(destPath.Path(), 0777);
			// Make sure the target path exists, it may have been deleted if
			// the existing destination was a file instead of a directory.
		if (ret != B_OK && ret != B_FILE_EXISTS) {
			fprintf(stderr, "Could not create '%s': %s\n", destPath.Path(),
				strerror(ret));
			return ret;
		}

		BDirectory destDirectory(&destination);
		ret = destDirectory.InitCheck();
		if (ret != B_OK)
			return ret;

		BEntry entry;
		while (sourceDirectory.GetNextEntry(&entry) == B_OK) {
			BEntry dest(&destDirectory, entry.Name());
			ret = dest.InitCheck();
			if (ret != B_OK)
				return ret;
			ret = _Copy(entry, dest, cancelSemaphore, copyAttributes);
			if (ret != B_OK)
				return ret;
		}
	} else {
		if (destination.Exists()) {
			if (destination.IsDirectory())
				ret = CopyEngine::RemoveFolder(destination);
			else
				ret = destination.Remove();
			if (ret != B_OK) {
				fprintf(stderr, "Failed to make room for entry '%s': "
					"%s\n", sourcePath.Path(), strerror(ret));
				return ret;
			}
		}

		fItemsCopied++;
		BPath destDirectory;
		ret = destPath.GetParent(&destDirectory);
		if (ret != B_OK)
			return ret;
		fCurrentTargetFolder = destDirectory.Path();
		fCurrentItem = sourcePath.Leaf();
		_UpdateProgress();

		if (S_ISLNK(sourceInfo.st_mode)) {
			// copy symbolic links
			BSymLink srcLink(&source);
			ret = srcLink.InitCheck();
			if (ret != B_OK)
				return ret;

			char linkPath[B_PATH_NAME_LENGTH];
			ssize_t read = srcLink.ReadLink(linkPath, B_PATH_NAME_LENGTH - 1);
			if (read < 0)
				return (status_t)read;

			BDirectory dstFolder;
			ret = destination.GetParent(&dstFolder);
			if (ret != B_OK)
				return ret;
			ret = dstFolder.CreateSymLink(sourcePath.Leaf(), linkPath, NULL);
			if (ret != B_OK)
				return ret;
		} else {
			// copy file data
			// NOTE: Do not pass the locker, we simply keep holding the lock!
			ret = _CopyData(source, destination);
			if (ret != B_OK)
				return ret;
		}
	}

	if (copyAttributesToTarget) {
		// copy attributes to the current target
		BNode sourceNode(&source);
		BNode targetNode(&destination);
		char attrName[B_ATTR_NAME_LENGTH];
		while (sourceNode.GetNextAttrName(attrName) == B_OK) {
			attr_info info;
			if (sourceNode.GetAttrInfo(attrName, &info) != B_OK)
				continue;
			size_t size = 4096;
			uint8 buffer[size];
			off_t offset = 0;
			ssize_t read = sourceNode.ReadAttr(attrName, info.type,
				offset, buffer, std::min((off_t)size, info.size));
			// NOTE: It's important to still write the attribute even if
			// we have read 0 bytes!
			while (read >= 0) {
				targetNode.WriteAttr(attrName, info.type, offset, buffer, read);
				offset += read;
				read = sourceNode.ReadAttr(attrName, info.type,
					offset, buffer, std::min((off_t)size, info.size - offset));
				if (read == 0)
					break;
			}
		}

		// copy basic attributes
		destination.SetPermissions(sourceInfo.st_mode);
		destination.SetOwner(sourceInfo.st_uid);
		destination.SetGroup(sourceInfo.st_gid);
		destination.SetModificationTime(sourceInfo.st_mtime);
		destination.SetCreationTime(sourceInfo.st_crtime);
	}

	return B_OK;
}


const char*
CopyEngine::_RelativeEntryPath(const char* absoluteSourcePath) const
{
	if (strncmp(absoluteSourcePath, fAbsoluteSourcePath,
			fAbsoluteSourcePath.Length()) != 0) {
		return absoluteSourcePath;
	}

	const char* relativePath
		= absoluteSourcePath + fAbsoluteSourcePath.Length();
	return relativePath[0] == '/' ? relativePath + 1 : relativePath;
}


void
CopyEngine::_UpdateProgress()
{
	if (fProgressReporter == NULL)
		return;

	uint64 items = 0;
	if (fLastItemsCopied < fItemsCopied) {
		items = fItemsCopied - fLastItemsCopied;
		fLastItemsCopied = fItemsCopied;
	}

	off_t bytes = 0;
	if (fLastBytesRead < fBytesRead) {
		bytes = fBytesRead - fLastBytesRead;
		fLastBytesRead = fBytesRead;
	}

	fProgressReporter->ItemsWritten(items, bytes, fCurrentItem,
		fCurrentTargetFolder);
}


int32
CopyEngine::_WriteThreadEntry(void* cookie)
{
	CopyEngine* engine = (CopyEngine*)cookie;
	engine->_WriteThread();
	return B_OK;
}


void
CopyEngine::_WriteThread()
{
	bigtime_t bufferWaitTimeout = 100000;

	while (!fQuitting) {

		bigtime_t now = system_time();

		Buffer* buffer = NULL;
		status_t ret = fBufferQueue.Pop(&buffer, bufferWaitTimeout);
		if (ret == B_TIMED_OUT) {
			// no buffer, timeout
			continue;
		} else if (ret == B_NO_INIT) {
			// real error
			return;
		} else if (ret != B_OK) {
			// no buffer, queue error
			snooze(10000);
			continue;
		}

		if (!buffer->deleteFile) {
			ssize_t written = buffer->file->Write(buffer->buffer,
				buffer->validBytes);
			if (written != (ssize_t)buffer->validBytes) {
				// TODO: this should somehow be propagated back
				// to the main thread!
				fprintf(stderr, "Failed to write data: %s\n",
					strerror((status_t)written));
			}
			fBytesWritten += written;
		}

		delete buffer;

		// measure performance
		fTimeWritten += system_time() - now;
	}

	double megaBytes = (double)fBytesWritten / (1024 * 1024);
	double seconds = (double)fTimeWritten / 1000000;
	if (seconds > 0) {
		printf("%.2f MB written (%.2f MB/s)\n", megaBytes,
			megaBytes / seconds);
	}
}
