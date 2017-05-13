/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef _JSON_MESSAGE_WRITER_H
#define _JSON_MESSAGE_WRITER_H


#include "JsonWriter.h"

#include <Message.h>
#include <String.h>


enum json_message_container_what {
	B_JSON_MESSAGE_WHAT_OBJECT	= '_JTM',
	B_JSON_MESSAGE_WHAT_ARRAY	= '_JTA'
};


namespace BPrivate {

class BStackedMessageEventListener;

class BJsonMessageWriter : public BJsonWriter {
friend class BStackedMessageEventListener;
public:
								BJsonMessageWriter(BMessage& message);
		virtual					~BJsonMessageWriter();

			bool				Handle(const BJsonEvent& event);
			void				Complete();

private:
			void				SetStackedListener(
									BStackedMessageEventListener* listener);

			BMessage*			fTopLevelMessage;
			BStackedMessageEventListener*
								fStackedListener;
};


} // namespace BPrivate

using BPrivate::BJsonMessageWriter;

#endif	// _JSON_MESSAGE_WRITER_H
