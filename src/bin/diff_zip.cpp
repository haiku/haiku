/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <map>
#include <string>


using std::string;
using std::map;


class Directory;
class Node;

static Node* create_node(Directory* parent, const string& name,
	const struct stat& st);


enum diff_status {
	DIFF_UNCHANGED,
	DIFF_REMOVED,
	DIFF_CHANGED,
	DIFF_ERROR
};


class EntryWriter {
public:
	EntryWriter(int fd)
		: fFD(fd)
	{
	}

	void Write(const char* entry)
	{
		write(fFD, entry, strlen(entry));
		write(fFD, "\n", 1);
	}

private:
	int	fFD;
};


class Node {
public:
	Node(Directory* parent, const string& name, const struct stat& st)
		: fParent(parent),
		  fName(name),
		  fStat(st)
	{
	}

	virtual ~Node()
	{
	}

	Directory* Parent() const		{ return fParent; }
	const string& Name() const		{ return fName; }
	const struct stat& Stat() const	{ return fStat; }

	string Path() const;

	bool DoStat(struct stat& st) const
	{
		string path(Path());
		if (lstat(path.c_str(), &st) != 0)
			return false;
		return true;
	}

	virtual bool Scan()
	{
		return true;
	}

	virtual diff_status CollectDiffEntries(EntryWriter* out) const
	{
		string path(Path());
		struct stat st;

		diff_status status = DiffEntry(path, st);
		if (status == DIFF_CHANGED)
			out->Write(path.c_str());

		return status;
	}

	virtual void Dump(int level) const
	{
		printf("%*s%s\n", level, "", fName.c_str());
	}

protected:
	diff_status DiffEntry(const string& path, struct stat& st) const
	{
		if (lstat(path.c_str(), &st) == 0) {
			if (st.st_mode != fStat.st_mode
				|| st.st_mtime != fStat.st_mtime
				|| st.st_size != fStat.st_size) {
				return DIFF_CHANGED;
			}
			return DIFF_UNCHANGED;
		} else if (errno == ENOENT) {
			// that's OK, the entry was removed
			return DIFF_REMOVED;
		} else {
			// some error
			fprintf(stderr, "Error: Failed to stat \"%s\": %s\n",
				path.c_str(), strerror(errno));
			return DIFF_ERROR;
		}
	}

private:
	Directory*	fParent;
	string		fName;
	struct stat	fStat;
};


class Directory : public Node {
public:
	Directory(Directory* parent, const string& name, const struct stat& st)
		: Node(parent, name, st),
		  fEntries()
	{
	}

	void AddEntry(const char* name, Node* node)
	{
		fEntries[name] = node;
	}

	virtual bool Scan()
	{
		string path(Path());
		DIR* dir = opendir(path.c_str());
		if (dir == NULL) {
			fprintf(stderr, "Error: Failed to open directory \"%s\": %s\n",
				path.c_str(), strerror(errno));
			return false;
		}

		errno = 0;
		while (dirent* entry = readdir(dir)) {
			if (strcmp(entry->d_name, ".") == 0
				|| strcmp(entry->d_name, "..") == 0) {
				continue;
			}

			string entryPath = path + '/' + entry->d_name;
			struct stat st;
			if (lstat(entryPath.c_str(), &st) != 0) {
				fprintf(stderr, "Error: Failed to stat entry \"%s\": %s\n",
					entryPath.c_str(), strerror(errno));
				closedir(dir);
				return false;
			}

			Node* node = create_node(this, entry->d_name, st);
			fEntries[entry->d_name] = node;

			errno = 0;
		}

		if (errno != 0) {
			fprintf(stderr, "Error: Failed to read directory \"%s\": %s\n",
				path.c_str(), strerror(errno));
			closedir(dir);
			return false;
		}

		closedir(dir);

		// recursively scan the entries
		for (EntryMap::iterator it = fEntries.begin(); it != fEntries.end();
				++it) {
			Node* node = it->second;
			if (!node->Scan())
				return false;
		}

		return true;
	}

	virtual diff_status CollectDiffEntries(EntryWriter* out) const
	{
		string path(Path());
		struct stat st;

		diff_status status = DiffEntry(path, st);
		if (status == DIFF_REMOVED || status == DIFF_ERROR)
			return status;

		// we print it only if it is no longer a directory
		if (!S_ISDIR(st.st_mode)) {
			out->Write(path.c_str());
			return DIFF_CHANGED;
		}

		// iterate through the "new" entries
		DIR* dir = opendir(path.c_str());
		if (dir == NULL) {
			fprintf(stderr, "Error: Failed to open directory \"%s\": %s\n",
				path.c_str(), strerror(errno));
			return DIFF_ERROR;
		}

		errno = 0;
		while (dirent* entry = readdir(dir)) {
			if (strcmp(entry->d_name, ".") == 0
				|| strcmp(entry->d_name, "..") == 0) {
				continue;
			}

			EntryMap::const_iterator it = fEntries.find(entry->d_name);
			if (it == fEntries.end()) {
				// new entry
				string entryPath = path + "/" + entry->d_name;
				out->Write(entryPath.c_str());
			} else {
				// old entry -- recurse
				diff_status entryStatus = it->second->CollectDiffEntries(out);
				if (entryStatus == DIFF_ERROR) {
					closedir(dir);
					return status;
				}
				if (entryStatus != DIFF_UNCHANGED)
					status = DIFF_CHANGED;
			}

			errno = 0;
		}

		if (errno != 0) {
			fprintf(stderr, "Error: Failed to read directory \"%s\": %s\n",
				path.c_str(), strerror(errno));
			closedir(dir);
			return DIFF_ERROR;
		}

		closedir(dir);
		return status;
	}

	virtual void Dump(int level) const
	{
		Node::Dump(level);

		for (EntryMap::const_iterator it = fEntries.begin();
				it != fEntries.end(); ++it) {
			it->second->Dump(level + 1);
		}
	}


private:
	typedef map<string, Node*> EntryMap;

	EntryMap	fEntries;
};


string
Node::Path() const
{
	if (fParent == NULL)
		return fName;

	return fParent->Path() + '/' + fName;
}


static Node*
create_node(Directory* parent, const string& name, const struct stat& st)
{
	if (S_ISDIR(st.st_mode))
		return new Directory(parent, name, st);
	return new Node(parent, name, st);
}


int
main(int argc, const char* const* argv)
{
	// the paths are listed after a "--" argument
	int argi = 1;
	for (; argi < argc; argi++) {
		if (strcmp(argv[argi], "--") == 0)
			break;
	}

	if (argi + 1 >= argc) {
		fprintf(stderr, "Usage: %s <zip arguments> ... -- <paths>\n", argv[0]);
		exit(1);
	}

	int zipArgCount = argi;
	const char* const* paths = argv + argi + 1;
	int pathCount = argc - argi - 1;

	// snapshot the hierarchy
	Node** rootNodes = new Node*[pathCount];

	for (int i = 0; i < pathCount; i++) {
		const char* path = paths[i];
		struct stat st;
		if (lstat(path, &st) != 0) {
			fprintf(stderr, "Error: Failed to stat \"%s\": %s\n", path,
				strerror(errno));
			exit(1);
		}

		rootNodes[i] = create_node(NULL, path, st);
		if (!rootNodes[i]->Scan())
			exit(1);
	}

	// create a temp file
	char tmpFileName[64];
	sprintf(tmpFileName, "/tmp/diff_zip_%d_XXXXXX", (int)getpid());
	int tmpFileFD = mkstemp(tmpFileName);
	if (tmpFileFD < 0) {
		fprintf(stderr, "Error: Failed create temp file: %s\n",
			strerror(errno));
		exit(1);
	}
	unlink(tmpFileName);

	// wait
	{
		printf("Waiting for changes. Press RETURN to continue...");
		fflush(stdout);
		char buffer[32];
		fgets(buffer, sizeof(buffer), stdin);
	}

	EntryWriter tmpFile(tmpFileFD);

	for (int i = 0; i < pathCount; i++) {
		if (rootNodes[i]->CollectDiffEntries(&tmpFile) == DIFF_ERROR)
			exit(1);
	}

	// dup the temp file FD to our stdin and exec()
	dup2(tmpFileFD, 0);
	close(tmpFileFD);
	lseek(0, 0, SEEK_SET);

	char** zipArgs = new char*[zipArgCount + 2];
	zipArgs[0] = strdup("zip");
	zipArgs[1] = strdup("-@");
	for (int i = 1; i < zipArgCount; i++)
		zipArgs[i + 1] = strdup(argv[i]);
	zipArgs[zipArgCount + 1] = NULL;

	execvp("zip", zipArgs);

	fprintf(stderr, "Error: Failed exec*() zip: %s\n", strerror(errno));
	delete[] rootNodes;

	return 1;
}
