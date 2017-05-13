/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef _JSON_EVENT_H
#define _JSON_EVENT_H


#include <String.h>


/*! This enumeration defines the types of events that may arise when parsing a
    stream of JSON data.
*/

typedef enum json_event_type {
	B_JSON_NUMBER		= 1,
	B_JSON_STRING		= 2,
	B_JSON_TRUE			= 3,
	B_JSON_FALSE		= 4,
	B_JSON_NULL			= 5,
	B_JSON_OBJECT_START	= 6,
	B_JSON_OBJECT_END	= 7,
	B_JSON_OBJECT_NAME	= 8, // aka field
	B_JSON_ARRAY_START	= 9,
	B_JSON_ARRAY_END	= 10
} json_event_type;


namespace BPrivate {

class BJsonEvent {
public:
								BJsonEvent(json_event_type eventType,
									const char* content);
								BJsonEvent(const char* content);
								BJsonEvent(double content);
								BJsonEvent(int64 content);
								BJsonEvent(json_event_type eventType);
								~BJsonEvent();

			json_event_type		EventType() const;

			const char*			Content() const;
			double				ContentDouble() const;
			int64				ContentInteger() const;

private:

			json_event_type		fEventType;
			const char*			fContent;
			char*				fOwnedContent;
};

} // namespace BPrivate

using BPrivate::BJsonEvent;

#endif	// _JSON_EVENT_H
