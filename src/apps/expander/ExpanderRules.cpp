/*
 * Copyright 2004-2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ExpanderRules.h"

#include <FindDirectory.h>
#include <stdlib.h>
#include <Path.h>
#include <NodeInfo.h>
#include <stdio.h>
#include <unistd.h>


ExpanderRule::ExpanderRule(BString mimetype, BString filenameExtension, 
		BString listingCmd, BString expandCmd)
	:
	fMimeType(mimetype.String()),
	fFilenameExtension(filenameExtension),
	fListingCmd(listingCmd),
	fExpandCmd(expandCmd)
{
}


ExpanderRule::ExpanderRule(const char* mimetype, const char* filenameExtension,
		const char* listingCmd, const char* expandCmd)
	:
	fMimeType(mimetype),
	fFilenameExtension(filenameExtension),
	fListingCmd(listingCmd),
	fExpandCmd(expandCmd)
{
}


ExpanderRules::ExpanderRules()
{
	fList.AddItem(new ExpanderRule("", 								".tar.gz", 	"zcat %s | tar -tvf -", 		"zcat %s | tar -xf -"));
	fList.AddItem(new ExpanderRule("", 								".tar.bz2", "bzcat %s | tar -tvf -", 		"bzcat %s | tar -xf -"));
	fList.AddItem(new ExpanderRule("", 								".tar.Z", 	"zcat %s | tar -tvf -", 		"zcat %s | tar -xf -"));
	fList.AddItem(new ExpanderRule("", 								".tgz", 	"zcat %s | tar -tvf -", 		"zcat %s | tar -xf -"));
	fList.AddItem(new ExpanderRule("application/x-tar", 			".tar", 	"tar -tvf %s", 					"tar -xf %s"));
	fList.AddItem(new ExpanderRule("application/x-gzip",			".gz", 		"echo %s | sed 's/.gz$//g'", 	"gunzip %s"));
	fList.AddItem(new ExpanderRule("application/x-bzip2",			".bz2", 	"echo %s | sed 's/.bz2$//g'", 	"bunzip2 %s"));
	fList.AddItem(new ExpanderRule("application/zip", 				".zip", 	"unzip -l %s", 					"unzip -o %s"));
	fList.AddItem(new ExpanderRule("application/x-zip-compressed", 	".zip", 	"unzip -l %s", 					"unzip -o %s"));
	fList.AddItem(new ExpanderRule("application/x-rar",				".rar", 	"unrar v %s", 					"unrar x -y %s"));

	BFile file;
	if (Open(&file) != B_OK)
		return;

	int fd = file.Dup();
	FILE * f = fdopen(fd, "r");

	char buffer[1024];
	BString strings[4];
	while (fgets(buffer, 1024-1, f)!=NULL) {
		int32 i = 0, j = 0;
		int32 firstQuote = -1;
		while (buffer[i] != '#' && buffer[i] != '\n' && j < 4) {
			if ((j == 0 || j > 1) && buffer[i] == '"') {
				if (firstQuote >= 0) {
					strings[j++].SetTo(&buffer[firstQuote+1], i-firstQuote-1);
					firstQuote = -1;
				} else
					firstQuote = i;
			} else if (j == 1 && (buffer[i] == ' ' || buffer[i] == '\t')) {
				if (firstQuote >= 0) {
					if (firstQuote + 1 != i) {
						strings[j++].SetTo(&buffer[firstQuote+1], i-firstQuote-1);
						firstQuote = -1;
					} else
						firstQuote = i;
				} else
					firstQuote = i;
			}	
			i++;
		}
		if (j == 4) {
			fList.AddItem(new ExpanderRule(strings[0], strings[1], strings[2], strings[3]));
		} 
	}
	fclose(f);
	close(fd);
}


ExpanderRules::~ExpanderRules()
{
	void *item;
	while ((item = fList.RemoveItem((int32)0)))
		delete (ExpanderRule*)item;
}


status_t 
ExpanderRules::Open(BFile *file)
{
	BPath path;
	if (find_directory(B_USER_CONFIG_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("etc/expander.rules");
	if (file->SetTo(path.Path(), B_READ_ONLY) == B_OK)
		return B_OK;
	
	if (find_directory(B_COMMON_ETC_DIRECTORY, &path) != B_OK)
		return B_ERROR;

	path.Append("expander.rules");

	return file->SetTo(path.Path(), B_READ_ONLY);	
}


ExpanderRule *
ExpanderRules::MatchingRule(BString &fileName, const char *filetype)
{
	int32 count = fList.CountItems();
	int32 length = fileName.Length();
	for (int32 i=0; i<count; i++) {
		ExpanderRule *rule = (ExpanderRule *)fList.ItemAt(i);
		if ((rule->MimeType().IsValid() && rule->MimeType() == filetype)
			|| (fileName.FindLast(rule->FilenameExtension()) == (length - rule->FilenameExtension().Length())))
			return rule;
	}
	return NULL;
}


ExpanderRule *
ExpanderRules::MatchingRule(const entry_ref *ref)
{
	BEntry entry(ref, true);
	BNode node(&entry);
	BNodeInfo nodeInfo(&node);
	char type[B_MIME_TYPE_LENGTH];
	nodeInfo.GetType(type);
	BString fileName(ref->name);
	return MatchingRule(fileName, type); 
}


RuleRefFilter::RuleRefFilter(ExpanderRules &rules)
	: BRefFilter(),
	fRules(rules)
{
}


bool
RuleRefFilter::Filter(const entry_ref *ref, BNode* node, struct stat *st, 
	const char *filetype)
{
	if (node->IsDirectory() || node->IsSymLink())
		return true;

	BString fileName(ref->name);
	return fRules.MatchingRule(fileName, filetype) != NULL;
}
