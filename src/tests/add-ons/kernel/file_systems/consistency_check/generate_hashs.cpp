/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <algorithm>
#include <string>
#include <vector>

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <OS.h>

#include "SHA256.h"


//#define TRACE(x...) printf(x)
#define TRACE(x...) ;


extern const char *__progname;
static const char *kProgramName = __progname;

const size_t kInitialBufferSize = 1 * 1024 * 1024;
const size_t kMaxBufferSize = 10 * 1024 * 1024;


class AdaptiveBuffering {
public:
							AdaptiveBuffering(size_t initialBufferSize,
								size_t maxBufferSize, uint32 count);
	virtual					~AdaptiveBuffering();

	virtual status_t		Init();

	virtual status_t		Read(uint8* buffer, size_t* _length);
	virtual status_t		Write(uint8* buffer, size_t length);

			status_t		Run();

private:
			void			_QuitWriter();
			status_t		_Writer();
	static	status_t		_Writer(void* self);

			thread_id		fWriterThread;
			uint8**			fBuffers;
			size_t*			fReadBytes;
			uint32			fBufferCount;
			uint32			fReadIndex;
			uint32			fWriteIndex;
			uint32			fReadCount;
			uint32			fWriteCount;
			size_t			fMaxBufferSize;
			size_t			fCurrentBufferSize;
			sem_id			fReadSem;
			sem_id			fWriteSem;
			sem_id			fFinishedSem;
			status_t		fWriteStatus;
			uint32			fWriteTime;
			bool			fFinished;
			bool			fQuit;
};

class SHAProcessor : public AdaptiveBuffering {
public:
	SHAProcessor()
		: AdaptiveBuffering(kInitialBufferSize, kMaxBufferSize, 3),
		fFile(-1)
	{
	}

	virtual ~SHAProcessor()
	{
		Unset();
	}

	void Unset()
	{
		if (fFile >= 0)
			close(fFile);
	}

	status_t Process(int file)
	{
		Unset();
		fSHA.Init();
		fFile = file;

		return Run();
	}

	virtual status_t Read(uint8* buffer, size_t* _length)
	{
		ssize_t bytes = read(fFile, buffer, *_length);
		if (bytes < B_OK)
			return errno;

		*_length = bytes;
		return B_OK;
	}

	virtual status_t Write(uint8* buffer, size_t length)
	{
		fSHA.Update(buffer, length);
		return B_OK;
	}

	const uint8* Digest() { return fSHA.Digest(); }
	size_t DigestLength() const	{ return fSHA.DigestLength(); }

private:
	SHA256	fSHA;
	int		fFile;
};

struct file_entry {
	uint8			hash[SHA_DIGEST_LENGTH];
	std::string		path;

	bool operator<(const struct file_entry& other) const
	{
		return path < other.path;
	}

	std::string HashString() const
	{
		char buffer[128];
		for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
			sprintf(buffer + i * 2, "%02x", hash[i]);
		}

		return buffer;
	}
};

typedef std::vector<file_entry> FileList;

void process_file(const char* path);


SHAProcessor gSHA;
FileList gFiles;


AdaptiveBuffering::AdaptiveBuffering(size_t initialBufferSize,
		size_t maxBufferSize, uint32 count)
	:
	fWriterThread(-1),
	fBuffers(NULL),
	fReadBytes(NULL),
	fBufferCount(count),
	fReadIndex(0),
	fWriteIndex(0),
	fReadCount(0),
	fWriteCount(0),
	fMaxBufferSize(maxBufferSize),
	fCurrentBufferSize(initialBufferSize),
	fReadSem(-1),
	fWriteSem(-1),
	fFinishedSem(-1),
	fWriteStatus(B_OK),
	fWriteTime(0),
	fFinished(false),
	fQuit(false)
{
}


AdaptiveBuffering::~AdaptiveBuffering()
{
	_QuitWriter();

	delete_sem(fReadSem);
	delete_sem(fWriteSem);

	if (fBuffers != NULL) {
		for (uint32 i = 0; i < fBufferCount; i++) {
			if (fBuffers[i] == NULL)
				break;

			free(fBuffers[i]);
		}

		free(fBuffers);
	}

	free(fReadBytes);
}


status_t
AdaptiveBuffering::Init()
{
	fReadBytes = (size_t*)malloc(fBufferCount * sizeof(size_t));
	if (fReadBytes == NULL)
		return B_NO_MEMORY;

	fBuffers = (uint8**)malloc(fBufferCount * sizeof(uint8*));
	if (fBuffers == NULL)
		return B_NO_MEMORY;

	for (uint32 i = 0; i < fBufferCount; i++) {
		fBuffers[i] = (uint8*)malloc(fMaxBufferSize);
		if (fBuffers[i] == NULL)
			return B_NO_MEMORY;
	}

	fReadSem = create_sem(0, "reader");
	if (fReadSem < B_OK)
		return fReadSem;

	fWriteSem = create_sem(fBufferCount - 1, "writer");
	if (fWriteSem < B_OK)
		return fWriteSem;

	fFinishedSem = create_sem(0, "finished");
	if (fFinishedSem < B_OK)
		return fFinishedSem;

	fWriterThread = spawn_thread(&_Writer, "buffer reader", B_LOW_PRIORITY,
		this);
	if (fWriterThread < B_OK)
		return fWriterThread;

	return resume_thread(fWriterThread);
}


status_t
AdaptiveBuffering::Read(uint8* /*buffer*/, size_t* _length)
{
	*_length = 0;
	return B_OK;
}


status_t
AdaptiveBuffering::Write(uint8* /*buffer*/, size_t /*length*/)
{
	return B_OK;
}


status_t
AdaptiveBuffering::Run()
{
	fReadIndex = 0;
	fWriteIndex = 0;
	fReadCount = 0;
	fWriteCount = 0;
	fWriteStatus = B_OK;
	fWriteTime = 0;

	while (fWriteStatus >= B_OK) {
		bigtime_t start = system_time();
		int32 index = fReadIndex;

		TRACE("%ld. read index %lu, buffer size %lu\n", fReadCount, index,
			fCurrentBufferSize);

		fReadBytes[index] = fCurrentBufferSize;
		status_t status = Read(fBuffers[index], &fReadBytes[index]);
		if (status < B_OK)
			return status;

		TRACE("%ld. read -> %lu bytes\n", fReadCount, fReadBytes[index]);

		fReadCount++;
		fReadIndex = (index + 1) % fBufferCount;
		if (fReadBytes[index] == 0)
			fFinished = true;
		release_sem(fReadSem);

		while (acquire_sem(fWriteSem) == B_INTERRUPTED)
			;

		if (fFinished)
			break;

		bigtime_t readTime = system_time() - start;
		uint32 writeTime = fWriteTime;
		if (writeTime) {
			if (writeTime > readTime) {
				fCurrentBufferSize = fCurrentBufferSize * 8/9;
				fCurrentBufferSize &= ~65535;
			} else {
				fCurrentBufferSize = fCurrentBufferSize * 9/8;
				fCurrentBufferSize = (fCurrentBufferSize + 65535) & ~65535;

				if (fCurrentBufferSize > fMaxBufferSize)
					fCurrentBufferSize = fMaxBufferSize;
			}
		}
	}

	while (acquire_sem(fFinishedSem) == B_INTERRUPTED)
		;

	return fWriteStatus;
}


void
AdaptiveBuffering::_QuitWriter()
{
	if (fWriterThread >= B_OK) {
		fQuit = true;
		release_sem(fReadSem);

		status_t status;
		wait_for_thread(fWriterThread, &status);

		fWriterThread = -1;
	}
}


status_t
AdaptiveBuffering::_Writer()
{
	while (true) {
		while (acquire_sem(fReadSem) == B_INTERRUPTED)
			;
		if (fQuit)
			break;

		bigtime_t start = system_time();

		TRACE("%ld. write index %lu, %p, bytes %lu\n", fWriteCount, fWriteIndex,
			fBuffers[fWriteIndex], fReadBytes[fWriteIndex]);

		fWriteStatus = Write(fBuffers[fWriteIndex], fReadBytes[fWriteIndex]);

		TRACE("%ld. write done\n", fWriteCount);

		fWriteIndex = (fWriteIndex + 1) % fBufferCount;
		fWriteTime = uint32(system_time() - start);
		fWriteCount++;

		release_sem(fWriteSem);

		if (fWriteStatus < B_OK)
			return fWriteStatus;
		if (fFinished)
			release_sem(fFinishedSem);
	}

	return B_OK;
}


/*static*/ status_t
AdaptiveBuffering::_Writer(void* self)
{
	return ((AdaptiveBuffering*)self)->_Writer();
}


//	#pragma mark -


void
process_directory(const char* path)
{
	DIR* dir = opendir(path);
	if (dir == NULL)
		return;

	while (struct dirent* entry = readdir(dir)) {
		if (!strcmp(entry->d_name, ".")
			|| !strcmp(entry->d_name, ".."))
			continue;

		char fullPath[1024];
		strlcpy(fullPath, path, sizeof(fullPath));
		strlcat(fullPath, "/", sizeof(fullPath));
		strlcat(fullPath, entry->d_name, sizeof(fullPath));

		process_file(fullPath);
	}

	closedir(dir);
}


void
process_file(const char* path)
{
	struct stat stat;
	if (::stat(path, &stat) != 0) {
		fprintf(stderr, "Could not stat file \"%s\": %s\n", path,
			strerror(errno));
		return;
	}

	if (S_ISDIR(stat.st_mode)) {
		process_directory(path);
		return;
	}

	int file = open(path, O_RDONLY);
	if (file < 0) {
		fprintf(stderr, "Could not open file \"%s\": %s\n", path,
			strerror(errno));
		return;
	}

	status_t status = gSHA.Process(file);
	if (status != B_OK) {
		fprintf(stderr, "Computing SHA failed \"%s\": %s\n", path,
			strerror(status));
		return;
	}

	file_entry entry;
	memcpy(entry.hash, gSHA.Digest(), SHA_DIGEST_LENGTH);
	entry.path = path;

	printf("%s  %s\n", entry.HashString().c_str(), path);

	gFiles.push_back(entry);
}


int
main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <hash-file> <files>\n", kProgramName);
		return 1;
	}

	status_t status = gSHA.Init();
	if (status != B_OK) {
		fprintf(stderr, "Could not initialize SHA processor: %s\n",
			strerror(status));
		return 1;
	}

	bigtime_t start = system_time();

	for (int i = 1; i < argc; i++) {
		process_file(argv[i]);
	}

	sort(gFiles.begin(), gFiles.end());

	bigtime_t runtime = system_time() - start;
	if (gFiles.size() > 0) {
		printf("Generated hashes for %ld files in %g seconds, %g msec per "
			"file.\n", gFiles.size(), runtime / 1000000.0,
			runtime / 1000.0 / gFiles.size());
	}

	return 0;
}
