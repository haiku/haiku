/*
 * Copyright 2004-2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef _ExpanderRules_h
#define _ExpanderRules_h

#include <List.h>
#include <File.h>
#include <String.h>
#include <Mime.h>
#include <FilePanel.h>

class ExpanderRule {
	public:
		ExpanderRule(BString mimetype, BString filenameExtension,
			BString listingCmd, BString expandCmd);
		ExpanderRule(const char*  mimetype, const char*  filenameExtension,
			const char*  listingCmd, const char* expandCmd);
		BMimeType	&MimeType() { return fMimeType;}
		BString		&FilenameExtension() { return fFilenameExtension;}
		BString		&ListingCmd() { return fListingCmd;}
		BString		&ExpandCmd() { return fExpandCmd;}
	private:
		BMimeType 	fMimeType;
		BString 	fFilenameExtension;
		BString 	fListingCmd;
		BString 	fExpandCmd;
};


class ExpanderRules {
	public:
		ExpanderRules();
		~ExpanderRules();
		ExpanderRule *MatchingRule(BString &fileName, const char *filetype);
		ExpanderRule *MatchingRule(const entry_ref *ref);
	private:
		status_t Open(BFile *file);

		BList	fList;
};


class RuleRefFilter : public BRefFilter {
	public:
		RuleRefFilter(ExpanderRules &rules);
		bool Filter(const entry_ref *ref, BNode* node, struct stat_beos *st,
			const char *filetype);
	protected:
		ExpanderRules &fRules;
};

#endif /* _ExpanderRules_h */
