#ifndef ZOIDBERG_MAIL_MESSAGE_H
#define ZOIDBERG_MAIL_MESSAGE_H
/* Message - the main general purpose mail message class
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <MailContainer.h>


// add our additional attributes 
#define B_MAIL_ATTR_ACCOUNT "MAIL:account"
#define B_MAIL_ATTR_THREAD "MAIL:thread"


class BDirectory;

enum mail_reply_to_mode {
	B_MAIL_REPLY_TO = 0,
	B_MAIL_REPLY_TO_ALL,
	B_MAIL_REPLY_TO_SENDER
};

class BEmailMessage : public BMailContainer {
	public:
		BEmailMessage(BPositionIO *mail_file = NULL, bool own = false, uint32 defaultCharSet = B_MAIL_NULL_CONVERSION);
		BEmailMessage(const entry_ref *ref,
			uint32 defaultCharSet = B_MAIL_NULL_CONVERSION);
		virtual ~BEmailMessage();

		status_t InitCheck() const;
		BPositionIO *Data() const { return fData; }
			// is only set if the message owns the data

		BEmailMessage *ReplyMessage(mail_reply_to_mode replyTo, bool accountFromMail, const char *quote_style = "> ");
		BEmailMessage *ForwardMessage(bool accountFromMail, bool includeAttachments = false);
			// These return messages with the body quoted and
			// ready to send via the appropriate channel. ReplyMessage()
			// addresses the message appropriately, but ForwardMessage()
			// leaves it unaddressed.

		const char *To();
		const char *From();
		const char *ReplyTo();
		const char *CC();
		const char *Subject();
		const char *Date();
		int Priority();

		void SetSubject(const char *to, uint32 charset = B_MAIL_NULL_CONVERSION, mail_encoding encoding = null_encoding);
		void SetReplyTo(const char *to, uint32 charset = B_MAIL_NULL_CONVERSION, mail_encoding encoding = null_encoding);
		void SetFrom(const char *to, uint32 charset = B_MAIL_NULL_CONVERSION, mail_encoding encoding = null_encoding);
		void SetTo(const char *to, uint32 charset = B_MAIL_NULL_CONVERSION, mail_encoding encoding = null_encoding);
		void SetCC(const char *to, uint32 charset = B_MAIL_NULL_CONVERSION, mail_encoding encoding = null_encoding);
		void SetBCC(const char *to);
		void SetPriority(int to);

		status_t GetName(char *name,int32 maxLength) const;
		status_t GetName(BString *name) const;

		void SendViaAccountFrom(BEmailMessage *message);
		void SendViaAccount(const char *account_name);
		void SendViaAccount(int32 account);
		int32 Account() const;
		status_t GetAccountName(BString& accountName) const;

		virtual status_t AddComponent(BMailComponent *component);
		virtual status_t RemoveComponent(BMailComponent *component);
		virtual status_t RemoveComponent(int32 index);

		virtual BMailComponent *GetComponent(int32 index, bool parse_now = false);
		virtual int32 CountComponents() const;

		void Attach(entry_ref *ref, bool include_attributes = true);
		bool IsComponentAttachment(int32 index);

		void SetBodyTextTo(const char *text);
		const char *BodyText();

		status_t SetBody(BTextMailComponent *body);
		BTextMailComponent *Body();

		virtual status_t SetToRFC822(BPositionIO *data, size_t length, bool parse_now = false);
		virtual status_t RenderToRFC822(BPositionIO *render_to);

		status_t RenderTo(BDirectory *dir, BEntry *message = NULL);
			//---message will be set to the message file if not equal to NULL

		status_t Send(bool send_now);

	private:
		BTextMailComponent *RetrieveTextBody(BMailComponent *);

		virtual void _ReservedMessage1();
		virtual void _ReservedMessage2();
		virtual void _ReservedMessage3();

		BPositionIO *fData;

		status_t _status;
		int32 _account_id;
		char *_bcc;

		int32 _num_components;
		BMailComponent *_body;
		BTextMailComponent *_text_body;

		uint32 _reserved[5];
};

#endif	/* ZOIDBERG_MAIL_MESSAGE_H */
