/*
 * Copyright 2004-2006, Jérôme Duval. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef _EXPANDER_RULES_H
#define _EXPANDER_RULES_H

#include <File.h>
#include <FilePanel.h>
#include <List.h>
#include <Mime.h>
#include <String.h>


class ExpanderRule {
public:
								ExpanderRule(const char* mimetype,
									const BString& filenameExtension,
									const BString& listingCommand,
									const BString& expandCommand);

			const BMimeType&	MimeType() const
									{ return fMimeType; }
			const BString&		FilenameExtension() const
									{ return fFilenameExtension; }
			const BString&		ListingCmd() const
									{ return fListingCmd; }
			const BString&		ExpandCmd() const
									{ return fExpandCmd; }

private:
			BMimeType			fMimeType;
			BString 			fFilenameExtension;
			BString 			fListingCmd;
			BString 			fExpandCmd;
};


class ExpanderRules {
public:
								ExpanderRules();
								~ExpanderRules();

			ExpanderRule*		MatchingRule(BString& fileName,
									const char* filetype);
			ExpanderRule*		MatchingRule(const entry_ref* ref);

private:
			void				_LoadRulesFiles();
			void				_LoadRulesFile(const char* path);

			bool				_AddRule(const char* mimetype,
									const BString& filenameExtension,
									const BString& listingCommand,
									const BString& expandCommand);

private:
			BList				fList;
};


class RuleRefFilter : public BRefFilter {
public:
								RuleRefFilter(ExpanderRules& rules);
			bool				Filter(const entry_ref* ref, BNode* node,
									struct stat_beos* stat,
									const char* filetype);

protected:
			ExpanderRules&		fRules;
};


#endif	// _EXPANDER_RULES_H
