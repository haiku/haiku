/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef _JSON_EVENT_LISTENER_H
#define _JSON_EVENT_LISTENER_H


#include "JsonEvent.h"

#include <String.h>


/*! This constant line number can be used in raising an error where the
    client raising the error does not know on which line the error arose.
*/

#define JSON_EVENT_LISTENER_ANY_LINE -1

namespace BPrivate {

class BJsonEventListener {
public:
								BJsonEventListener();
		virtual					~BJsonEventListener();

		virtual bool			Handle(const BJsonEvent& event) = 0;
		virtual void			HandleError(status_t status, int32 line,
									const char* message) = 0;
		virtual void			Complete() = 0;
};

} // namespace BPrivate

using BPrivate::BJsonEventListener;

#endif	// _JSON_EVENT_LISTENER_H
