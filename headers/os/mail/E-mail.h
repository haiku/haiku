/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _E_MAIL_H
#define _E_MAIL_H


#include <UTF8.h>


class BList;
struct entry_ref;


// E-Mail attributes
#define B_MAIL_ATTR_NAME		"MAIL:name"				// indexed string
#define B_MAIL_ATTR_STATUS		"MAIL:status"			// indexed string
#define B_MAIL_ATTR_PRIORITY	"MAIL:priority"			// indexed string
#define B_MAIL_ATTR_TO			"MAIL:to"				// indexed string
#define B_MAIL_ATTR_CC			"MAIL:cc"				// indexed string
#define B_MAIL_ATTR_BCC			"MAIL:bcc"				// string
#define B_MAIL_ATTR_FROM		"MAIL:from"				// indexed string
#define B_MAIL_ATTR_SUBJECT		"MAIL:subject"			// indexed string
#define B_MAIL_ATTR_REPLY		"MAIL:reply"			// indexed string
#define B_MAIL_ATTR_WHEN		"MAIL:when"				// indexed time
#define B_MAIL_ATTR_FLAGS		"MAIL:flags"			// indexed int32
#define B_MAIL_ATTR_RECIPIENTS	"MAIL:recipients"		// string
#define B_MAIL_ATTR_MIME		"MAIL:mime"				// string
#define B_MAIL_ATTR_HEADER		"MAIL:header_length"	// int32
#define B_MAIL_ATTR_CONTENT		"MAIL:content_length"	// int32
#define B_MAIL_ATTR_READ		"MAIL:read"				// int32
#define B_MAIL_ATTR_THREAD		"MAIL:thread"			// string
#define B_MAIL_ATTR_ACCOUNT		"MAIL:account"			// string
#define B_MAIL_ATTR_ACCOUNT_ID	"MAIL:account_id"		// int32


// read flags
enum read_flags {
	B_UNREAD				= 0,
	B_SEEN					= 1,
	B_READ					= 2
	
};


// mail flags
enum mail_flags {
	B_MAIL_PENDING			= 1,						// waiting to be sent

	B_MAIL_SENT				= 2,						// has been sent

	B_MAIL_SAVE				= 4							// save mail after
														// sending
};

#define B_MAIL_TYPE				"text/x-email"			// mime type
#define B_PARTIAL_MAIL_TYPE		"text/x-partial-email"	// mime type


// WARNING: Everything past this point is deprecated. See MailMessage.h,
// mail_encoding.h and MailDaemon.h for alternatives.


// #pragma mark - defines

// schedule days
#define B_CHECK_NEVER			 0
#define B_CHECK_WEEKDAYS		 1
#define B_CHECK_DAILY			 2
#define B_CHECK_CONTINUOUSLY	 3
#define B_CHECK_CONTINUOSLY		 3

// max. lengths
#define B_MAX_USER_NAME_LENGTH	32
#define B_MAX_HOST_NAME_LENGTH	64

// rfc822 header field types
#define B_MAIL_TO				"To: "
#define B_MAIL_CC				"Cc: "
#define B_MAIL_BCC				"Bcc: "
#define B_MAIL_FROM				"From: "
#define B_MAIL_DATE				"Date: "
#define B_MAIL_REPLY			"Reply-To: "
#define B_MAIL_SUBJECT			"Subject: "
#define B_MAIL_PRIORITY			"Priority: "


// #pragma mark - structs


typedef struct {
	char		pop_name[B_MAX_USER_NAME_LENGTH];
	char		pop_password[B_MAX_USER_NAME_LENGTH];
	char		pop_host[B_MAX_HOST_NAME_LENGTH];
	char		real_name[128];
	char		reply_to[128];
	int32		days;			/* see flags above*/
	int32		interval;		/* in seconds*/
	int32		begin_time;		/* in seconds*/
	int32		end_time;		/* in seconds*/
} mail_pop_account;

typedef struct {
	bool		alert;
	bool		beep;
} mail_notification;


// #pragma mark - global functions

int32 count_pop_accounts(void);
status_t get_pop_account(mail_pop_account*, int32 index = 0);
status_t set_pop_account(mail_pop_account*, int32 index = 0,
	bool save = true);


// #pragma mark - BMailMessage

class BMailMessage {
public:
								BMailMessage();
								~BMailMessage();

			status_t			AddContent(const char* text, int32 length,
									uint32 encoding = B_ISO1_CONVERSION,
									bool clobber = false);
			status_t			AddContent(const char* text, int32 length,
									const char* encoding,
									bool clobber = false);
	
			status_t			AddEnclosure(entry_ref* ref,
									bool clobber = false);
			status_t			AddEnclosure(const char* path,
									bool clobber = false);
			status_t			AddEnclosure(const char* MIME_type, void* data,
									int32 len, bool clobber = false);
	
			status_t			AddHeaderField(uint32 encoding,
									const char* field_name, const char* str, 
									bool clobber = false);
			status_t			AddHeaderField(const char* field_name,
									const char* str, bool clobber = false);
	
			status_t			Send(bool sendNow = false,
									 bool removeAfterSending = false);

private:
			int32				concatinate(char**, int32, char*);
			int32				count_fields(char* name = NULL);
			status_t			find_field(char*, type_code*, char**, void**,
									int32*, uint32*, char**, bool*, int32);
			BList*				find_field(const char*);
			status_t			get_field_name(char**, int32);
			status_t			set_field(const char*, type_code, const char*,
									const void*, int32, uint32, const char*,
									bool);

private:
			BList*		fFields;
			bool		fMultiPart;
};

#endif // _MAIL_H
