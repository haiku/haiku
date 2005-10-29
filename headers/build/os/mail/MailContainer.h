#ifndef ZOIDBERG_MAIL_CONTAINER_H
#define ZOIDBERG_MAIL_CONTAINER_H
/* Container - message part container class
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <List.h>

#include <MailComponent.h>

class BPositionIO;

class BMailContainer : public BMailComponent {
	public:
		BMailContainer (uint32 defaultCharSet = B_MAIL_NULL_CONVERSION) :
			BMailComponent (defaultCharSet) {};

		virtual status_t AddComponent(BMailComponent *component) = 0;
		virtual status_t RemoveComponent(BMailComponent *component) = 0;
		virtual status_t RemoveComponent(int32 index) = 0;

		virtual BMailComponent *GetComponent(int32 index, bool parse_now = false) = 0;
		virtual int32 CountComponents() const = 0;
	
	private:
		virtual void _ReservedContainer1();
		virtual void _ReservedContainer2();
		virtual void _ReservedContainer3();
		virtual void _ReservedContainer4();
};

class BMIMEMultipartMailContainer : public BMailContainer {
	public:
		BMIMEMultipartMailContainer(
			const char *boundary = NULL,
			const char *this_is_an_MIME_message_text = NULL,
			uint32 defaultCharSet = B_MAIL_NULL_CONVERSION);
		BMIMEMultipartMailContainer(BMIMEMultipartMailContainer &copy);
		virtual ~BMIMEMultipartMailContainer();

		void SetBoundary(const char *boundary);
		void SetThisIsAnMIMEMessageText(const char *text);

		// MailContainer
		virtual status_t AddComponent(BMailComponent *component);
		virtual status_t RemoveComponent(BMailComponent *component);
		virtual status_t RemoveComponent(int32 index);

		virtual BMailComponent *GetComponent(int32 index, bool parse_now = false);
		virtual int32 CountComponents() const;

		// MailComponent
		virtual status_t GetDecodedData(BPositionIO *data);
		virtual status_t SetDecodedData(BPositionIO *data);
		
		virtual status_t SetToRFC822(BPositionIO *data, size_t length, bool parse_now = false);
		virtual status_t RenderToRFC822(BPositionIO *render_to);

	private:
		virtual void _ReservedMultipart1();
		virtual void _ReservedMultipart2();
		virtual void _ReservedMultipart3();

		const char *_boundary;
		const char *_MIME_message_warning;
		
		BPositionIO *_io_data;
		
		BList _components_in_raw;
		BList _components_in_code;
		

		uint32 _reserved[5];
};

#endif	/* ZOIDBERG_MAIL_CONTAINER_H */
