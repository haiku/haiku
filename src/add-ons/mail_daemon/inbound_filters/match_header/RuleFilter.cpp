/* Match Header - performs action depending on matching a header value
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Node.h>
#include <String.h>

#include <stdlib.h>
#include <stdio.h>

#include "RuleFilter.h"

//class StatusView;

RuleFilter::RuleFilter(BMessage *settings) : BMailFilter(settings) {
	// attribute is adapted to our "capitalize-each-word-in-the-header" policy
	BString attr;
	settings->FindString("attribute",&attr);
	attr.CapitalizeEachWord();
	attribute = strdup(attr.String());

	const char *regex = NULL;	
	settings->FindString("regex",&regex);
	matcher.SetPattern(regex,true);
	
	settings->FindString("argument",&arg);
	settings->FindInt32("do_what",(long *)&do_what);
	if (do_what == Z_SET_REPLY)
		settings->FindInt32("argument",&chain_id);
}

RuleFilter::~RuleFilter()
{
	if (attribute)
		free((void *)attribute);
}

status_t RuleFilter::InitCheck(BString* out_message) {
	return B_OK;
}

status_t RuleFilter::ProcessMailMessage
(
	BPositionIO** , BEntry* entry,
	BMessage* io_headers, BPath* io_folder, const char* 
) {
	const char *data;
	if (!attribute)
		return B_OK; //----That field doesn't exist? NO match
	
	if (io_headers->FindString(attribute,&data) < B_OK) { //--Maybe the capitalization was wrong?
		BString capped(attribute);
		capped.CapitalizeEachWord(); //----Enfore capitalization
		if (io_headers->FindString(capped.String(),&data) < B_OK) //----This time it's *really* not there
			return B_OK; //---No match
	}
	
	if (data == NULL) //--- How would this happen? No idea
		return B_OK;

	if (!matcher.Match(data))
		return B_OK; //-----There wasn't an error. We're just not supposed to do anything
		
	switch (do_what) {
		case Z_MOVE_TO:
			if (io_headers->ReplaceString("DESTINATION",arg) != B_OK)
				io_headers->AddString("DESTINATION",arg);
			break;
		case Z_TRASH:
			return B_MAIL_DISCARD;
		case Z_FLAG:
			{
			BString string = arg;
			BNode(entry).WriteAttrString("MAIL:filter_flags",&string);
			}
			break;
		case Z_SET_REPLY:
			BNode(entry).WriteAttr("MAIL:reply_with",B_INT32_TYPE,0,&chain_id,4);
			break;
		case Z_SET_READ:
			if (io_headers->ReplaceString("STATUS", "Read") != B_OK)
				io_headers->AddString("STATUS", "Read");
			break;			
		default:
			fprintf(stderr,"Unknown do_what: 0x%04x!\n",do_what);
	}
	
	return B_OK;
}

status_t descriptive_name(BMessage *settings, char *buffer) {
	const char *attribute = NULL;
	settings->FindString("attribute",&attribute);
	const char *regex = NULL;
	settings->FindString("regex",&regex);

	if (!attribute || strlen(attribute) > 15)
		return B_ERROR;
	sprintf(buffer, "Match \"%s\"", attribute);

	if (!regex)
		return B_OK;

	char reg[20];
	strncpy(reg, regex, 16);
	if (strlen(regex) > 15)
		strcpy(reg + 15, "...");

	sprintf(buffer + strlen(buffer), " against \"%s\"", reg);

	return B_OK;
}

BMailFilter* instantiate_mailfilter(BMessage* settings,BMailChainRunner *)
{
	return new RuleFilter(settings);
}
