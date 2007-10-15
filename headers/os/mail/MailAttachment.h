#ifndef ZOIDBERG_MAIL_ATTACHMENT_H
#define ZOIDBERG_MAIL_ATTACHMENT_H
/* Attachment - classes which handle mail attachments
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <Node.h>

#include <MailContainer.h>
#include <MailComponent.h>

class BFile;

class BMailAttachment : public BMailComponent {
	public:
		virtual void SetFileName(const char *name) = 0;
		virtual status_t FileName(char *name) = 0;

		virtual status_t SetTo(BFile *file, bool deleteFileWhenDone = false) = 0;
		virtual status_t SetTo(entry_ref *ref) = 0;

		virtual status_t InitCheck() = 0;

	private:
		virtual void _ReservedAttachment1();
		virtual void _ReservedAttachment2();
		virtual void _ReservedAttachment3();
		virtual void _ReservedAttachment4();
};

class BSimpleMailAttachment : public BMailAttachment {
	public:
		BSimpleMailAttachment(BPositionIO *dataToAttach, mail_encoding encoding = base64);
		BSimpleMailAttachment(const void *dataToAttach, size_t lengthOfData, mail_encoding encoding = base64);

		BSimpleMailAttachment(BFile *file, bool delete_when_done);
		BSimpleMailAttachment(entry_ref *ref);

		BSimpleMailAttachment();
		virtual ~BSimpleMailAttachment();

		virtual status_t SetTo(BFile *file, bool delete_file_when_done = false);
		virtual status_t SetTo(entry_ref *ref);

		virtual status_t InitCheck();

		virtual void SetFileName(const char *name);
		virtual status_t FileName(char *name);

		virtual status_t GetDecodedData(BPositionIO *data);
		virtual status_t SetDecodedData(BPositionIO *data);

		virtual BPositionIO *GetDecodedData();
		virtual status_t SetDecodedData(const void *data, size_t length /* data to attach */);
		virtual status_t SetDecodedDataAndDeleteWhenDone(BPositionIO *data);

		void SetEncoding(mail_encoding encoding = base64 /* note: we only support base64. This is a no-op */);
		mail_encoding Encoding();

		virtual status_t SetToRFC822(BPositionIO *data, size_t length, bool parse_now = false);
		virtual status_t RenderToRFC822(BPositionIO *render_to);

	private:
		void Initialize(mail_encoding encoding);
		void ParseNow();
		
		virtual void _ReservedSimple1();
		virtual void _ReservedSimple2();
		virtual void _ReservedSimple3();

		status_t fStatus;
		BPositionIO *_data, *_raw_data;
		size_t _raw_length;
		off_t _raw_offset;
		bool _we_own_data;
		mail_encoding _encoding;
		
		uint32 _reserved[5];
};

class BAttributedMailAttachment : public BMailAttachment {
	public:
		BAttributedMailAttachment(BFile *file, bool delete_when_done);
		BAttributedMailAttachment(entry_ref *ref);
		
		BAttributedMailAttachment();
		virtual ~BAttributedMailAttachment();
		
		virtual status_t SetTo(BFile *file, bool delete_file_when_done = false);
		virtual status_t SetTo(entry_ref *ref);

		virtual status_t InitCheck();

		void SaveToDisk(BEntry *entry);
		//-----we pay no attention to entry, but set it to the location of our file in /tmp
		
		void SetEncoding(mail_encoding encoding /* anything but uuencode */);
		mail_encoding Encoding();
		
		virtual status_t FileName(char *name);
		virtual void SetFileName(const char *name);
		
		virtual status_t GetDecodedData(BPositionIO *data);
		virtual status_t SetDecodedData(BPositionIO *data);
		
		virtual status_t SetToRFC822(BPositionIO *data, size_t length, bool parse_now = false);
		virtual status_t RenderToRFC822(BPositionIO *render_to);
		
		virtual status_t MIMEType(BMimeType *mime);

	private:
		status_t Initialize();

		virtual void _ReservedAttributed1();
		virtual void _ReservedAttributed2();
		virtual void _ReservedAttributed3();

		BMIMEMultipartMailContainer *fContainer;
		status_t fStatus;

		BSimpleMailAttachment *_data, *_attributes_attach;
		BMessage _attributes;

		uint32 _reserved[5];
};

#endif	/* ZOIDBERG_MAIL_ATTACHMENT_H */
