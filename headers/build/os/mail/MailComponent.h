#ifndef ZOIDBERG_MAIL_COMPONENT_H
#define ZOIDBERG_MAIL_COMPONENT_H
/* (Text)Component - message component base class and plain text
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <UTF8.h>
#include <Message.h>
#include <String.h>

#include <mail_encoding.h>

class BMimeType;

extern const char *kHeaderCharsetString;
extern const char *kHeaderEncodingString;
// Special field names in the headers which specify the character set (int32)
// and encoding (int8) to use when converting the headers from UTF-8 to the
// output e-mail format (rfc2047).  For use with SetHeaderField when you pass
// it a structured header in a BMessage.


enum component_type {
	B_MAIL_PLAIN_TEXT_BODY = 0,
	B_MAIL_SIMPLE_ATTACHMENT,
	B_MAIL_ATTRIBUTED_ATTACHMENT,
	B_MAIL_MULTIPART_CONTAINER
};

class BMailComponent {
	public:
		BMailComponent(uint32 defaultCharSet = B_MAIL_NULL_CONVERSION);
		virtual ~BMailComponent();

		//------Info on this component
		uint32 ComponentType();
		BMailComponent *WhatIsThis();
			// Takes any generic MailComponent, and returns an instance
			// of a MailComponent subclass that applies to this case,
			// ready for instantiation. Note that you still have to
			// Instantiate() it yourself.
		bool IsAttachment();
			// Returns true if this component is an attachment, false
			// otherwise

		void SetHeaderField(
			const char *key, const char *value,
			uint32 charset = B_MAIL_NULL_CONVERSION,
			mail_encoding encoding = null_encoding,
			bool replace_existing = true);
			// If you want to delete a header, pass in a zero length or NULL
			// string for the value field, or use RemoveHeader.
		void SetHeaderField(
			const char *key, BMessage *structured_header,
			bool replace_existing = true);

		const char *HeaderAt(int32 index);
		const char *HeaderField(const char *key, int32 index = 0);
		status_t	HeaderField(const char *key, BMessage *structured_header, int32 index = 0);

		status_t	RemoveHeader(const char *key);

		virtual status_t GetDecodedData(BPositionIO *data);
		virtual status_t SetDecodedData(BPositionIO *data);

		virtual status_t SetToRFC822(BPositionIO *data, size_t length, bool parse_now = false);
		virtual status_t RenderToRFC822(BPositionIO *render_to);

		virtual status_t MIMEType(BMimeType *mime);

	protected:
		uint32 _charSetForTextDecoding;
			// This is the character set to be used for decoding text
			// components, or if it is B_MAIL_NULL_CONVERSION then the character
			// set will be determined automatically.  Since we can't use a
			// global variable (different messages might have different values
			// of this), and since sub-components can't find their parents,
			// this is passed down during construction to some (just Component,
			// Container, Message, MIME, Text) child components and ends up
			// being used in the text components.

	private:
		virtual void _ReservedComponent1();
		virtual void _ReservedComponent2();
		virtual void _ReservedComponent3();
		virtual void _ReservedComponent4();
		virtual void _ReservedComponent5();

		BMessage headers;

		uint32 _reserved[5];
};


class BTextMailComponent : public BMailComponent {
	public:
		BTextMailComponent(const char *text = NULL, uint32 defaultCharSet = B_MAIL_NULL_CONVERSION);
		virtual ~BTextMailComponent();

		void SetEncoding(mail_encoding encoding, int32 charset);
			// encoding: you should always use quoted_printable, base64 is strongly not
			//		recommended, see rfc 2047 for the reasons why
			// charset: use Conversion flavor constants from UTF8.h

		void SetText(const char *text);
		void AppendText(const char *text);

		const char *Text();
		BString *BStringText();

		void Quote(const char *message = NULL,
				   const char *quote_style = "> ");

		virtual status_t GetDecodedData(BPositionIO *data);
		virtual status_t SetDecodedData(BPositionIO *data);

		virtual status_t SetToRFC822(BPositionIO *data, size_t length, bool parse_now = false);
		virtual status_t RenderToRFC822(BPositionIO *render_to);

	private:
		virtual void _ReservedText1();
		virtual void _ReservedText2();

		BString text;
		BString decoded;

		mail_encoding encoding;
		uint32 charset; // This character set is used for encoding, not decoding.

		status_t ParseRaw();
		BPositionIO *raw_data;
		size_t raw_length;
		off_t raw_offset;

		uint32 _reserved[5];
};

#endif	/* ZOIDBERG_MAIL_COMPONENT_H */
