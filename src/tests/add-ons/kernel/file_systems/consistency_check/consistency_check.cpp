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
#include <Path.h>

#include <SHA256.h>

#include "AdaptiveBuffering.h"


//#define TRACE(x...) printf(x)
#define TRACE(x...) ;


extern const char *__progname;
static const char *kProgramName = __progname;

const size_t kInitialBufferSize = 1 * 1024 * 1024;
const size_t kMaxBufferSize = 10 * 1024 * 1024;


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
	ino_t			node;
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


void
process_file(const file_entry& entry, int number)
{
	struct stat stat;
	if (::stat(entry.path.c_str(), &stat) != 0) {
		fprintf(stderr, "Could not stat file \"%s\": %s\n", entry.path.c_str(),
			strerror(errno));
		return;
	}

	if (stat.st_ino != entry.node) {
		fprintf(stderr, "\"%s\": inode changed from %Ld to %Ld\n",
			entry.path.c_str(), entry.node, stat.st_ino);
	}

	int file = open(entry.path.c_str(), O_RDONLY);
	if (file < 0) {
		fprintf(stderr, "Could not open file \"%s\": %s\n", entry.path.c_str(),
			strerror(errno));
		return;
	}

	status_t status = gSHA.Process(file);
	if (status != B_OK) {
		fprintf(stderr, "Computing SHA failed \"%s\": %s\n", entry.path.c_str(),
			strerror(status));
		return;
	}

	if (memcmp(entry.hash, gSHA.Digest(), SHA_DIGEST_LENGTH))
		fprintf(stderr, "\"%s\": Contents differ!\n", entry.path.c_str());

	static bigtime_t sLastUpdate = -1;
	if (system_time() - sLastUpdate > 500000) {
		printf("%ld files scanned\33[1A\n", number);
		sLastUpdate = system_time();
	}
}


int
main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <hash-file>\n", kProgramName);
		return 1;
	}

	const char* hashFileName = argv[1];

	status_t status = gSHA.Init();
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not initialize SHA processor: %s\n",
			kProgramName, strerror(status));
		return 1;
	}

	// read files from hash file

	int file = open(hashFileName, O_RDONLY);
	if (file < 0) {
		fprintf(stderr, "%s: Could not open hash file \"%s\": %s\n",
			kProgramName, hashFileName, strerror(status));
		return 1;
	}

	char buffer[2048];
	read(file, buffer, 4);
	if (memcmp(buffer, "HASH", 4)) {
		fprintf(stderr, "%s: \"%s\" is not a hash file\n",
			kProgramName, hashFileName);
		close(file);
		return 1;
	}

	int fileCount;
	read(file, &fileCount, sizeof(int));
	TRACE("Skip %d path(s)\n", fileCount);

	// Skip paths, we don't need it for the consistency check

	for (int i = 0; i < fileCount; i++) {
		int length;
		read(file, &length, sizeof(int));
		lseek(file, length + 1, SEEK_CUR);
	}

	// Read file names and their hash

	read(file, &fileCount, sizeof(int));
	TRACE("Found %d file(s)\n", fileCount);

	for (int i = 0; i < fileCount; i++) {
		file_entry entry;
		read(file, entry.hash, SHA_DIGEST_LENGTH);
		read(file, &entry.node, sizeof(ino_t));

		int length;
		read(file, &length, sizeof(int));
		read(file, buffer, length + 1);

		entry.path = buffer;

		gFiles.push_back(entry);
	}

	close(file);

	bigtime_t start = system_time();

	for (int i = 0; i < fileCount; i++) {
		process_file(gFiles[i], i);
	}

	bigtime_t runtime = system_time() - start;

	if (gFiles.size() > 0) {
		printf("Consistency check for %ld files in %g seconds, %g msec per "
			"file.\n", gFiles.size(), runtime / 1000000.0,
			runtime / 1000.0 / gFiles.size());
	}

	return 0;
}
