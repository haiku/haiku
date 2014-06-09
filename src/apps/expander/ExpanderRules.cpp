/*
 * Copyright 2004-2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ExpanderRules.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <FindDirectory.h>
#include <NodeInfo.h>
#include <Path.h>

#include <compat/sys/stat.h>

#include "ExpanderSettings.h"


static const char* const kRulesDirectoryPath = "expander/rules";
static const char* const kUserRulesFileName = "rules";


// #pragma mark - ExpanderRule


ExpanderRule::ExpanderRule(const char* mimeType,
	const BString& filenameExtension, const BString& listingCommand,
	const BString& expandCommand)
	:
	fMimeType(mimeType),
	fFilenameExtension(filenameExtension),
	fListingCmd(listingCommand),
	fExpandCmd(expandCommand)
{
}


// #pragma mark - ExpanderRules


ExpanderRules::ExpanderRules()
{
	// Load the rules files first, then add the built-in rules. This way the
	// built-ins can be overridden, if the files contain matching rules.
	_LoadRulesFiles();

	_AddRule("", ".tar.gz", "tar -ztvf %s", "tar -zxf %s");
	_AddRule("", ".tar.bz2", "tar -jtvf %s", "tar -jxf %s");
	_AddRule("", ".tar.Z", "tar -Ztvf %s", "tar -Zxf %s");
	_AddRule("", ".tgz", "tar -ztvf %s", "tar -zxf %s");
	_AddRule("application/x-tar", ".tar", "tar -tvf %s", "tar -xf %s");
	_AddRule("application/x-gzip", ".gz", "echo %s | sed 's/.gz$//g'",
		"gunzip -c %s > `echo %s | sed 's/.gz$//g'`");
	_AddRule("application/x-bzip2", ".bz2", "echo %s | sed 's/.bz2$//g'",
		"bunzip2 -k %s");
	_AddRule("application/zip", ".zip", "unzip -l %s", "unzip -o %s");
	_AddRule("application/x-zip-compressed", ".zip", "unzip -l %s",
		"unzip -o %s");
	_AddRule("application/x-rar", ".rar", "unrar v %s", "unrar x -y %s");
	_AddRule("application/x-vnd.haiku-package", ".hpkg", "package list %s",
		"package extract %s");
}


ExpanderRules::~ExpanderRules()
{
	void* item;
	while ((item = fList.RemoveItem((int32)0)))
		delete (ExpanderRule*)item;
}


ExpanderRule*
ExpanderRules::MatchingRule(BString& fileName, const char* filetype)
{
	int32 count = fList.CountItems();
	int32 length = fileName.Length();
	for (int32 i = 0; i < count; i++) {
		ExpanderRule* rule = (ExpanderRule*)fList.ItemAt(i);
		if (rule->MimeType().IsValid() && rule->MimeType() == filetype)
			return rule;

		int32 extensionPosition = fileName.FindLast(rule->FilenameExtension());
		if (extensionPosition != -1 && extensionPosition
				== (length - rule->FilenameExtension().Length())) {
			return rule;
		}
	}

	return NULL;
}


ExpanderRule*
ExpanderRules::MatchingRule(const entry_ref* ref)
{
	BEntry entry(ref, true);
	BNode node(&entry);
	BNodeInfo nodeInfo(&node);
	char type[B_MIME_TYPE_LENGTH];
	nodeInfo.GetType(type);
	BString fileName(ref->name);

	return MatchingRule(fileName, type);
}


void
ExpanderRules::_LoadRulesFiles()
{
	// load the user editable rules first
	BPath path;
	if (ExpanderSettings::GetSettingsDirectoryPath(path) == B_OK
		&& path.Append(kUserRulesFileName) == B_OK) {
		_LoadRulesFile(path.Path());
	}

	// load the rules files from the data directories
	const directory_which kDirectories[] = {
		B_USER_NONPACKAGED_DATA_DIRECTORY,
		B_USER_DATA_DIRECTORY,
		B_SYSTEM_NONPACKAGED_DATA_DIRECTORY,
		B_SYSTEM_DATA_DIRECTORY
	};

	for (size_t i = 0; i < sizeof(kDirectories) / sizeof(kDirectories[0]);
			i++) {
		BDirectory directory;
		if (find_directory(kDirectories[i], &path) != B_OK
			|| path.Append(kRulesDirectoryPath) != B_OK
			|| directory.SetTo(path.Path()) != B_OK) {
			continue;
		}

		entry_ref entry;
		while (directory.GetNextRef(&entry) == B_OK) {
			BPath filePath;
			if (filePath.SetTo(path.Path(), entry.name) == B_OK)
				_LoadRulesFile(filePath.Path());
		}
	}
}


void
ExpanderRules::_LoadRulesFile(const char* path)
{
	FILE* file = fopen(path, "r");
	if (file == NULL)
		return;

	char buffer[1024];
	BString strings[4];
	while (fgets(buffer, 1024 - 1, file) != NULL) {
		int32 i = 0, j = 0;
		int32 firstQuote = -1;
		while (buffer[i] != '#' && buffer[i] != '\n' && j < 4) {
			if ((j == 0 || j > 1) && buffer[i] == '"') {
				if (firstQuote >= 0) {
					strings[j++].SetTo(&buffer[firstQuote+1],
						i - firstQuote - 1);
					firstQuote = -1;
				} else
					firstQuote = i;
			} else if (j == 1 && (buffer[i] == ' ' || buffer[i] == '\t')) {
				if (firstQuote >= 0) {
					if (firstQuote + 1 != i) {
						strings[j++].SetTo(&buffer[firstQuote+1],
							i - firstQuote - 1);
						firstQuote = -1;
					} else
						firstQuote = i;
				} else
					firstQuote = i;
			}
			i++;
		}

		if (j == 4)
			_AddRule(strings[0], strings[1], strings[2], strings[3]);
	}

	fclose(file);
}


bool
ExpanderRules::_AddRule(const char* mimeType, const BString& filenameExtension,
	const BString& listingCommand, const BString& expandCommand)
{
	ExpanderRule* rule = new(std::nothrow) ExpanderRule(mimeType,
		filenameExtension, listingCommand, expandCommand);
	if (rule == NULL)
		return false;

	if (!fList.AddItem(rule)) {
		delete rule;
		return false;
	}

	return true;
}


// #pragma mark - RuleRefFilter


RuleRefFilter::RuleRefFilter(ExpanderRules& rules)
	:
	BRefFilter(),
	fRules(rules)
{
}


bool
RuleRefFilter::Filter(const entry_ref* ref, BNode* node, struct stat_beos* stat,
	const char* filetype)
{
	if (node->IsDirectory() || node->IsSymLink())
		return true;

	BString fileName(ref->name);
	return fRules.MatchingRule(fileName, filetype) != NULL;
}
