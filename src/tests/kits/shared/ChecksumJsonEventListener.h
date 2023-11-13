/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef CHECKSUM_JSON_EVENT_LISTENER_H
#define CHECKSUM_JSON_EVENT_LISTENER_H


#include <JsonEventListener.h>


/*!	This class can be used by a text to accept JSON events and then to maintain a checksum of the
	strings and numbers that it encounters in order to get some sort of a checksum on the data
	that it is seeing. It can then compare this to the data that was emitted in order to verify
	that the JSON parser has parsed and passed-through all of the data correctly.
*/

class ChecksumJsonEventListener : public BJsonEventListener {
public:
								ChecksumJsonEventListener(int32 checksumLimit);
	virtual						~ChecksumJsonEventListener();

	virtual bool				Handle(const BJsonEvent& event);
	virtual void				HandleError(status_t status, int32 line, const char* message);
	virtual void				Complete();

			uint32				Checksum() const;
			status_t			Error() const;

private:
			void				_ChecksumProcessCharacters(const char* content, size_t len);

private:
			uint32				fChecksum;
			uint32				fChecksumLimit;
			status_t			fError;
			bool				fCompleted;
};

#endif // CHECKSUM_JSON_EVENT_LISTENER_H
