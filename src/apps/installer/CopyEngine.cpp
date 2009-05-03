/*
 * Copyright 2008-2009, Stephan Aßmus <superstippi@gmx.de>
 *  All rights reserved. Distributed under the terms of the MIT License.
 */

#include "CopyEngine.h"

#include <new>

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <Directory.h>
#include <fs_attr.h>
#include <NodeInfo.h>
#include <Path.h>
#include <String.h>
#include <SymLink.h>

#include "AutoLocker.h"
#include "InstallerWindow.h"
	// TODO: For PACKAGES_DIRECTORY and VAR_DIRECTORY, not so nice...

using std::nothrow;


CopyEngine::CopyEngine(const BMessenger& messenger, BMessage* message)
	:
	fBufferQueue(),
	fWriterThread(-1),
	fQuitting(false),

	fBytesRead(0),
	fItemsCopied(0),
	fTimeRead(0),

	fBytesWritten(0),
	fTimeWritten(0),

	fBytesToCopy(0),
	fItemsToCopy(0),

	fCurrentTargetFolder(NULL),
	fCurrentItem(NULL),

	fMessenger(messenger),
	fMessage(message)
{
	fWriterThread = spawn_thread(_WriteThreadEntry, "buffer writer",
		B_NORMAL_PRIORITY, this);

	if (fWriterThread >= B_OK)
		resume_thread(fWriterThread);
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

	delete fMessage;
}


void
CopyEngine::ResetTargets()
{
	fBytesRead = 0;
	fItemsCopied = 0;
	fTimeRead = 0;

	fBytesWritten = 0;
	fTimeWritten = 0;

	fBytesToCopy = 0;
	fItemsToCopy = 0;

	fCurrentTargetFolder = NULL;
	fCurrentItem = NULL;

	if (fMessage) {
		BMessage message(*fMessage);
		message.AddString("status", "Collecting copy information.");
		fMessenger.SendMessage(&message);
	}
}


status_t
CopyEngine::CollectTargets(const char* source)
{
	int32 level = 0;
	return _CollectCopyInfo(source, level);
}


status_t
CopyEngine::CopyFolder(const char* source, const char* destination,
	BLocker* locker)
{
	printf("%lld bytes to read in %lld files\n", fBytesToCopy, fItemsToCopy);

	if (fMessage) {
		BMessage message(*fMessage);
		message.AddString("status", "Performing installation.");
		fMessenger.SendMessage(&message);
	}

	int32 level = 0;
	return _CopyFolder(source, destination, level, locker);
}


status_t
CopyEngine::CopyFile(const BEntry& _source, const BEntry& _destination,
	BLocker* locker)
{
	AutoLocker<BLocker> lock(locker);
	if (locker != NULL && !lock.IsLocked()) {
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
			break;
		}

		fBytesRead += read;
		loopIteration += 1;
		if (loopIteration % 10 == 0)
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
CopyEngine::_CollectCopyInfo(const char* _source, int32& level)
{
	level++;

	BDirectory source(_source);
	status_t ret = source.InitCheck();
	if (ret < B_OK)
		return ret;

	BEntry entry;
	while (source.GetNextEntry(&entry) == B_OK) {
		struct stat statInfo;
		entry.GetStat(&statInfo);

		char name[B_FILE_NAME_LENGTH];
		status_t ret = entry.GetName(name);
		if (ret < B_OK)
			return ret;

		if (!_ShouldCopyEntry(name, statInfo, level))
			continue;

		if (S_ISDIR(statInfo.st_mode)) {
			// handle recursive directory copy
			BPath srcFolder;
			ret = entry.GetPath(&srcFolder);
			if (ret < B_OK)
				return ret;

			ret = _CollectCopyInfo(srcFolder.Path(), level);
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
	int32& level, BLocker* locker)
{
	level++;
	fCurrentTargetFolder = _destination;

	BDirectory source(_source);
	status_t ret = source.InitCheck();
	if (ret < B_OK)
		return ret;

	ret = create_directory(_destination, 0777);
	if (ret < B_OK && ret != B_FILE_EXISTS)
		return ret;

	BDirectory destination(_destination);
	ret = destination.InitCheck();
	if (ret < B_OK)
		return ret;

	BEntry entry;
	while (source.GetNextEntry(&entry) == B_OK) {
		AutoLocker<BLocker> lock(locker);
		if (locker != NULL && !lock.IsLocked()) {
			// We are supposed to quit
			return B_CANCELED;
		}

		char name[B_FILE_NAME_LENGTH];
		status_t ret = entry.GetName(name);
		if (ret < B_OK)
			return ret;

		struct stat statInfo;
		entry.GetStat(&statInfo);

		if (!_ShouldCopyEntry(name, statInfo, level))
			continue;

		fItemsCopied++;
		fCurrentItem = name;
		_UpdateProgress();

		BEntry copy(&destination, name);
		bool copyAttributes = true;

		if (S_ISDIR(statInfo.st_mode)) {
			// handle recursive directory copy

			if (copy.Exists()) {
				// Do not overwrite attributes on folders that exist.
				// This should work better when the install target already
				// contains a Haiku installation.
				copyAttributes = false;
			}

			BPath srcFolder;
			ret = entry.GetPath(&srcFolder);
			if (ret < B_OK)
				return ret;

			BPath dstFolder;
			ret = copy.GetPath(&dstFolder);
			if (ret < B_OK)
				return ret;

			if (locker != NULL)
				lock.Unlock();

			ret = _CopyFolder(srcFolder.Path(), dstFolder.Path(), level,
				locker);
			if (ret < B_OK)
				return ret;

			if (locker != NULL && !lock.Lock()) {
				// We are supposed to quit
				return B_CANCELED;
			}
		} else if (S_ISLNK(statInfo.st_mode)) {
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

		if (!copyAttributes)
			continue;

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


void
CopyEngine::_UpdateProgress()
{
	if (fMessage != NULL) {
		BMessage message(*fMessage);
		float progress = 100.0 * fBytesRead / fBytesToCopy;
		message.AddFloat("progress", progress);
		message.AddInt32("current", fItemsCopied);
		message.AddInt32("maximum", fItemsToCopy);
		message.AddString("item", fCurrentItem);
		message.AddString("folder", fCurrentTargetFolder);
		fMessenger.SendMessage(&message);
	}
}


bool
CopyEngine::_ShouldCopyEntry(const char* name, const struct stat& statInfo,
	int32 level) const
{
	if (level == 1 && S_ISDIR(statInfo.st_mode)) {
		if (strcmp(VAR_DIRECTORY, name) == 0) {
			printf("ignoring '%s'.\n", name);
			return false;
		}
		if (strcmp(PACKAGES_DIRECTORY, name) == 0) {
			printf("ignoring '%s'.\n", name);
			return false;
		}
	}
	return true;
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


