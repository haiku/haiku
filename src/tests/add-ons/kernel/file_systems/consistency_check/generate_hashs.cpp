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
process_directory(const char* path)
{
	DIR* dir = opendir(path);
	if (dir == NULL)
		return;

	size_t pathLength = strlen(path);

	while (struct dirent* entry = readdir(dir)) {
		if (!strcmp(entry->d_name, ".")
			|| !strcmp(entry->d_name, ".."))
			continue;

		char fullPath[1024];
		strlcpy(fullPath, path, sizeof(fullPath));
		if (path[pathLength - 1] != '/')
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
	if (::lstat(path, &stat) != 0) {
		fprintf(stderr, "Could not stat file \"%s\": %s\n", path,
			strerror(errno));
		return;
	}

	if (S_ISDIR(stat.st_mode)) {
		process_directory(path);
		return;
	}
	if (S_ISLNK(stat.st_mode))
		return;

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
	entry.node = stat.st_ino;
	entry.path = path;

	//printf("%s  %s\n", entry.HashString().c_str(), path);

	gFiles.push_back(entry);

	static bigtime_t sLastUpdate = -1;
	if (system_time() - sLastUpdate > 500000) {
		printf("%ld files scanned\33[1A\n", gFiles.size());
		sLastUpdate = system_time();
	}
}


void
write_hash_file(const char* name, int fileCount, char** files)
{
	int file = open(name, O_WRONLY | O_TRUNC | O_CREAT);
	if (file < 0) {
		fprintf(stderr, "%s: Could not write hash file \"%s\": %s\n",
			kProgramName, name, strerror(errno));
		return;
	}

	write(file, "HASH", 4);

	write(file, &fileCount, sizeof(int));
	for (int i = 0; i < fileCount; i++) {
		int length = strlen(files[i]);
		write(file, &length, sizeof(int));
		write(file, files[i], length + 1);
	}

	fileCount = gFiles.size();
	write(file, &fileCount, sizeof(int));
	for (int i = 0; i < fileCount; i++) {
		file_entry& entry = gFiles[i];

		write(file, entry.hash, SHA_DIGEST_LENGTH);
		write(file, &entry.node, sizeof(ino_t));

		int length = entry.path.size();
		write(file, &length, sizeof(int));
		write(file, entry.path.c_str(), length + 1);
	}

	close(file);
}


int
main(int argc, char** argv)
{
	if (argc < 2) {
		fprintf(stderr, "usage: %s <hash-file> [<files> ...]\n"
			"\tWhen invoked without files, the hash-file is updated only.\n",
			kProgramName);
		return 1;
	}

	const char* hashFileName = argv[1];

	status_t status = gSHA.Init();
	if (status != B_OK) {
		fprintf(stderr, "%s: Could not initialize SHA processor: %s\n",
			kProgramName, strerror(status));
		return 1;
	}

	int fileCount = argc - 2;
	char** files = argv + 2;

	if (argc == 2) {
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
		read(file, &fileCount, sizeof(int));
		TRACE("Found %d path(s):\n", fileCount);

		files = (char**)malloc(fileCount * sizeof(char*));
		if (files == NULL) {
			fprintf(stderr, "%s: Could not allocate %ld bytes\n",
				kProgramName, fileCount * sizeof(char*));
			close(file);
			return 1;
		}

		for (int i = 0; i < fileCount; i++) {
			int length;
			read(file, &length, sizeof(int));

			files[i] = (char*)malloc(length + 1);
			if (files[i] == NULL) {
				fprintf(stderr, "%s: Could not allocate %d bytes\n",
					kProgramName, length + 1);
				close(file);
				// TODO: we actually leak memory here, but it's not important in this context
				return 1;
			}
			read(file, files[i], length + 1);
			TRACE("\t%s\n", files[i]);
		}

		close(file);
	} else {
		// Normalize paths
		char** normalizedFiles = (char**)malloc(fileCount * sizeof(char*));
		if (normalizedFiles == NULL) {
			fprintf(stderr, "%s: Could not allocate %ld bytes\n",
				kProgramName, fileCount * sizeof(char*));
			return 1;
		}

		for (int i = 0; i < fileCount; i++) {
			BPath path(files[i], NULL, true);
			normalizedFiles[i] = strdup(path.Path());
			if (normalizedFiles[i] == NULL) {
				fprintf(stderr, "%s: Could not allocate %ld bytes\n",
					kProgramName, strlen(path.Path()) + 1);
				return 1;
			}
		}

		files = normalizedFiles;
	}

	bigtime_t start = system_time();

	for (int i = 0; i < fileCount; i++) {
		process_file(files[i]);
	}

	sort(gFiles.begin(), gFiles.end());

	bigtime_t runtime = system_time() - start;

	write_hash_file(hashFileName, fileCount, files);

	if (gFiles.size() > 0) {
		printf("Generated hashes for %ld files in %g seconds, %g msec per "
			"file.\n", gFiles.size(), runtime / 1000000.0,
			runtime / 1000.0 / gFiles.size());
	}

	for (int i = 0; i < fileCount; i++) {
		free(files[i]);
	}
	free(files);

	return 0;
}
