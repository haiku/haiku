/*****************************************************************************/
// Expander
// Written by Jérôme Duval
//
// ExpanderRules.h
//
//
// Copyright (c) 2004 OpenBeOS Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/

#ifndef _ExpanderRules_h
#define _ExpanderRules_h

#include <List.h>
#include <File.h>
#include <String.h>
#include <MimeType.h>
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
		bool Filter(const entry_ref *ref, BNode* node, struct stat *st, 
			const char *filetype);
	protected:
		ExpanderRules &fRules;
};

#endif /* _ExpanderRules_h */
