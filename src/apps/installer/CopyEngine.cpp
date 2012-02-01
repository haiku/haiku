/*
 * Copyright 2008-2009, Stephan Aßmus <superstippi@gmx.de>
 *  All rights reserved. Distributed under the terms of the MIT License.
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
#include <String.h>
#include <SymLink.h>

#include "InstallerWindow.h"
	// TODO: For PACKAGES_DIRECTORY and VAR_DIRECTORY, not so nice...
#include "SemaphoreLocker.h"
#include "ProgressReporter.h"


using std::nothrow;


CopyEngine::CopyEngine(ProgressReporter* reporter)
	:
	fBufferQueue(),
	fWriterThread(-1),
	fQuitting(false),

	fBytesRead(0),
	fLastBytesRead(0),
	fItemsCopied(0),
	fLastItemsCopied(0),
	fTimeRead(0),

	fBytesWritten(0),
	fTimeWritten(0),

	fBytesToCopy(0),
	fItemsToCopy(0),

	fCurrentTargetFolder(NULL),
	fCurrentItem(NULL),

	fProgressReporter(reporter)
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

	fBytesRead = 0;
	fLastBytesRead = 0;
	fItemsCopied = 0;
	fLastItemsCopied = 0;
	fTimeRead = 0;

	fBytesWritten = 0;
	fTimeWritten = 0;

	fBytesToCopy = 0;
	fItemsToCopy = 0;

	fCurrentTargetFolder = NULL;
	fCurrentItem = NULL;

	// init BEntry pointing to /var
	// There is no other way to retrieve the path to the var folder
	// on the source volume. Using find_directory() with
	// B_COMMON_VAR_DIRECTORY will only ever get the var folder on the
	// current /boot volume regardless of the volume of "source", which
	// makes sense, since passing a volume is meant to folders that are
	// volume specific, like "trash".
	BPath path(source);
	if (path.Append("common/var/swap") == B_OK)
		fSwapFileEntry.SetTo(path.Path());
	else
		fSwapFileEntry.Unset();
}


status_t
CopyEngine::CollectTargets(const char* source, sem_id cancelSemaphore)
{
	int32 level = 0;
	status_t ret = _CollectCopyInfo(source, level, cancelSemaphore);
	if (ret == B_OK && fProgressReporter != NULL)
		fProgressReporter->AddItems(fItemsToCopy, fBytesToCopy);
	return ret;
}


status_t
CopyEngine::CopyFolder(const char* source, const char* destination,
	sem_id cancelSemaphore)
{
	int32 level = 0;
	return _CopyFolder(source, destination, level, cancelSemaphore);
}


status_t
CopyEngine::CopyFile(const BEntry& _source, const BEntry& _destination,
	sem_id cancelSemaphore)
{
	SemaphoreLocker lock(cancelSemaphore);
	if (cancelSemaphore >= 0 && !lock.IsLocked()) {
		// We are supposed to quit
printf("CopyFile - cancled\n");
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
CopyEngine::_CollectCopyInfo(const char* _source, int32& level,
	sem_id cancelSemaphore)
{
	level++;

	BDirectory source(_source);
	status_t ret = source.InitCheck();
	if (ret < B_OK)
		return ret;

	BEntry entry;
	while (source.GetNextEntry(&entry) == B_OK) {
		SemaphoreLocker lock(cancelSemaphore);
		if (cancelSemaphore >= 0 && !lock.IsLocked()) {
			// We are supposed to quit
			return B_CANCELED;
		}

		struct stat statInfo;
		entry.GetStat(&statInfo);

		char name[B_FILE_NAME_LENGTH];
		status_t ret = entry.GetName(name);
		if (ret < B_OK)
			return ret;

		if (!_ShouldCopyEntry(entry, name, statInfo, level))
			continue;

		if (S_ISDIR(statInfo.st_mode)) {
			// handle recursive directory copy
			BPath srcFolder;
			ret = entry.GetPath(&srcFolder);
			if (ret < B_OK)
				return ret;

			if (cancelSemaphore >= 0)
				lock.Unlock();

			ret = _CollectCopyInfo(srcFolder.Path(), level, cancelSemaphore);
			if (ret < B_OK)
				return ret;
		} else if (S_ISLNK(statInfo.st_mode)) {
			// link, ignore size
		} else {
			// file data
			fBytesToCopy += statInfo.st_size;
		}

		fItemsToCopy++;
	}

	level--;
	return B_OK;
}


status_t
CopyEngine::_CopyFolder(const char* _source, const char* _destination,
	int32& level, sem_id cancelSemaphore)
{
	level++;
	fCurrentTargetFolder = _destination;

	BDirectory source(_source);
	status_t ret = source.InitCheck();
	if (ret < B_OK)
		return ret;

	ret = create_directory(_destination, 0777);
	if (ret < B_OK && ret != B_FILE_EXISTS) {
		fprintf(stderr, "Could not create '%s': %s\n", _destination,
			strerror(ret));
		return ret;
	}

	BDirectory destination(_destination);
	ret = destination.InitCheck();
	if (ret < B_OK)
		return ret;

	BEntry entry;
	while (source.GetNextEntry(&entry) == B_OK) {
		SemaphoreLocker lock(cancelSemaphore);
		if (cancelSemaphore >= 0 && !lock.IsLocked()) {
			// We are supposed to quit
			return B_CANCELED;
		}

		char name[B_FILE_NAME_LENGTH];
		status_t ret = entry.GetName(name);
		if (ret < B_OK)
			return ret;

		struct stat statInfo;
		entry.GetStat(&statInfo);

		if (!_ShouldCopyEntry(entry, name, statInfo, level))
			continue;

		fItemsCopied++;
		fCurrentItem = name;
		_UpdateProgress();

		BEntry copy(&destination, name);
		bool copyAttributes = true;

		if (S_ISDIR(statInfo.st_mode)) {
			// handle recursive directory copy

			if (copy.Exists()) {
				ret = B_OK;
				if (copy.IsDirectory()) {
					if (_ShouldClobberFolder(name, statInfo, level))
						ret = _RemoveFolder(copy);
					else {
						// Do not overwrite attributes on folders that exist.
						// This should work better when the install target
						// already contains a Haiku installation.
						copyAttributes = false;
					}
				} else
					ret = copy.Remove();

				if (ret != B_OK) {
					fprintf(stderr, "Failed to make room for folder '%s': "
						"%s\n", name, strerror(ret));
					return ret;
				}
			}

			BPath srcFolder;
			ret = entry.GetPath(&srcFolder);
			if (ret < B_OK)
				return ret;

			BPath dstFolder;
			ret = copy.GetPath(&dstFolder);
			if (ret < B_OK)
				return ret;

			if (cancelSemaphore >= 0)
				lock.Unlock();

			ret = _CopyFolder(srcFolder.Path(), dstFolder.Path(), level,
				cancelSemaphore);
			if (ret < B_OK)
				return ret;

			if (cancelSemaphore >= 0 && !lock.Lock()) {
				// We are supposed to quit
				return B_CANCELED;
			}
		} else {
			if (copy.Exists()) {
				if (copy.IsDirectory())
					ret = _RemoveFolder(copy);
				else
					ret = copy.Remove();
				if (ret != B_OK) {
					fprintf(stderr, "Failed to make room for entry '%s': "
						"%s\n", name, strerror(ret));
					return ret;
				}
			}
			if (S_ISLNK(statInfo.st_mode)) {
				// copy symbolic links
				BSymLink srcLink(&entry);
				if (ret < B_OK)
					return ret;

				char linkPath[B_PATH_NAME_LENGTH];
				ssize_t read = srcLink.ReadLink(linkPath, B_PATH_NAME_LENGTH - 1);
				if (read < 0)
					return (status_t)read;

				// just in case it already exists...
				copy.Remove();

				BSymLink dstLink;
				ret = destination.CreateSymLink(name, linkPath, &dstLink);
				if (ret < B_OK)
					return ret;
			} else {
				// copy file data
				// NOTE: Do not pass the locker, we simply keep holding the lock!
				ret = CopyFile(entry, copy);
				if (ret < B_OK)
					return ret;
			}
		}

		if (!copyAttributes)
			continue;

		ret = copy.SetTo(&destination, name);
		if (ret != B_OK)
			return ret;

		// copy attributes
		BNode sourceNode(&entry);
		BNode targetNode(&copy);
		char attrName[B_ATTR_NAME_LENGTH];
		while (sourceNode.GetNextAttrName(attrName) == B_OK) {
			attr_info info;
			if (sourceNode.GetAttrInfo(attrName, &info) < B_OK)
				continue;
			size_t size = 4096;
			uint8 buffer[size];
			off_t offset = 0;
			ssize_t read = sourceNode.ReadAttr(attrName, info.type,
				offset, buffer, min_c(size, info.size));
			// NOTE: It's important to still write the attribute even if
			// we have read 0 bytes!
			while (read >= 0) {
				targetNode.WriteAttr(attrName, info.type, offset, buffer, read);
				offset += read;
				read = sourceNode.ReadAttr(attrName, info.type,
				offset, buffer, min_c(size, info.size - offset));
				if (read == 0)
					break;
			}
		}

		// copy basic attributes
		copy.SetPermissions(statInfo.st_mode);
		copy.SetOwner(statInfo.st_uid);
		copy.SetGroup(statInfo.st_gid);
		copy.SetModificationTime(statInfo.st_mtime);
		copy.SetCreationTime(statInfo.st_crtime);
	}

	level--;
	return B_OK;
}


status_t
CopyEngine::_RemoveFolder(BEntry& entry)
{
	BDirectory directory(&entry);
	status_t ret = directory.InitCheck();
	if (ret != B_OK)
		return ret;

	BEntry subEntry;
	while (directory.GetNextEntry(&subEntry) == B_OK) {
		if (subEntry.IsDirectory()) {
			ret = _RemoveFolder(subEntry);
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


bool
CopyEngine::_ShouldCopyEntry(const BEntry& entry, const char* name,
	const struct stat& statInfo, int32 level) const
{
	if (level == 1 && S_ISDIR(statInfo.st_mode)) {
		if (strcmp(VAR_DIRECTORY, name) == 0) {
			// old location of /boot/var
			printf("ignoring '%s'.\n", name);
			return false;
		}
		if (strcmp(PACKAGES_DIRECTORY, name) == 0) {
			printf("ignoring '%s'.\n", name);
			return false;
		}
		if (strcmp(SOURCES_DIRECTORY, name) == 0) {
			printf("ignoring '%s'.\n", name);
			return false;
		}
		if (strcmp("rr_moved", name) == 0) {
			printf("ignoring '%s'.\n", name);
			return false;
		}
	}
	if (level == 1 && S_ISREG(statInfo.st_mode)) {
		if (strcmp("boot.catalog", name) == 0) {
			printf("ignoring '%s'.\n", name);
			return false;
		}
		if (strcmp("haiku-boot-floppy.image", name) == 0) {
			printf("ignoring '%s'.\n", name);
			return false;
		}
	}
	if (fSwapFileEntry == entry) {
		// current location of var
		printf("ignoring swap file\n");
		return false;
	}
	return true;
}


bool
CopyEngine::_ShouldClobberFolder(const char* name, const struct stat& statInfo,
	int32 level) const
{
	if (level == 1 && S_ISDIR(statInfo.st_mode)) {
		if (strcmp("system", name) == 0) {
			printf("clobbering '%s'.\n", name);
			return true;
		}
//		if (strcmp("develop", name) == 0) {
//			printf("clobbering '%s'.\n", name);
//			return true;
//		}
	}
	return false;
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

		Buffer* buffer;
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
	double seconds = (fTimeWritten / 1000000);
	if (seconds > 0) {
		printf("%.2f MB written (%.2f MB/s)\n", megaBytes,
			megaBytes / seconds);
	}
}


