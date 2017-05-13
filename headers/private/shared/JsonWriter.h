/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */
#ifndef _JSON_WRITER_H
#define _JSON_WRITER_H


#include "JsonEventListener.h"

#include <DataIO.h>
#include <String.h>


namespace BPrivate {

class BJsonWriter : public BJsonEventListener {
public:
								BJsonWriter();
		virtual					~BJsonWriter();

			void				HandleError(status_t status, int32 line,
									const char* message);
			status_t			ErrorStatus();

			status_t			WriteBoolean(bool value);
			status_t			WriteTrue();
			status_t			WriteFalse();
			status_t			WriteNull();

			status_t			WriteInteger(int64 value);
			status_t			WriteDouble(double value);

			status_t			WriteString(const char* value);

			status_t			WriteObjectStart();
			status_t			WriteObjectName(const char* value);
			status_t			WriteObjectEnd();

			status_t			WriteArrayStart();
			status_t			WriteArrayEnd();

protected:
			status_t			fErrorStatus;

};

} // namespace BPrivate

using BPrivate::BJsonWriter;

#endif	// _JSON_WRITER_H
