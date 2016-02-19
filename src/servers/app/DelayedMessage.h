/*
 * Copyright 2015, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *			Joseph Groover <looncraz@looncraz.net>
*/
#ifndef AS_DELAYED_MESSAGE_H
#define AS_DELAYED_MESSAGE_H


#include <ObjectList.h>
#include <OS.h>


//! Method by which to merge DelayedMessages with the same code.
enum DMMergeMode {
	DM_NO_MERGE			= 0, // Will send this message, and the other(s)
	DM_MERGE_REPLACE	= 1, // Replace older data with newer data
	DM_MERGE_CANCEL		= 2, // keeps older data, cancels this message
	DM_MERGE_DUPLICATES = 3  // If data is the same, cancel new message
};


//! Merge-mode data-matching, set which data must match to merge messages.
enum {
	DM_DATA_DEFAULT	= 0, // Match all for DUPLICATES & none for REPLACE modes.
	DM_DATA_1		= 1 << 0,
	DM_DATA_2		= 1 << 1,
	DM_DATA_3		= 1 << 2,
	DM_DATA_4		= 1 << 3,
	DM_DATA_5		= 1 << 4,
	DM_DATA_6		= 1 << 5
};


//! Convenient delay definitions.
enum {
	DM_MINIMUM_DELAY		= 500ULL,
	DM_SHORT_DELAY			= 1000ULL,
	DM_120HZ_DELAY			= 8888ULL,
	DM_60HZ_DELAY			= 16666ULL,
	DM_MEDIUM_DELAY			= 15000ULL,
	DM_30HZ_DELAY			= 33332ULL,
	DM_15HZ_DELAY			= 66664ULL,
	DM_LONG_DELAY			= 100000ULL,
	DM_QUARTER_SECOND_DELAY	= 250000ULL,
	DM_HALF_SECOND_DELAY	= 500000ULL,
	DM_ONE_SECOND_DELAY		= 1000000ULL,
	DM_ONE_MINUTE_DELAY		= DM_ONE_SECOND_DELAY * 60,
	DM_ONE_HOUR_DELAY		= DM_ONE_MINUTE_DELAY * 60
};


class DelayedMessageData;


/*!	\class DelayedMessage
	\brief Friendly API for creating messages to be sent at a future time.

	Messages can be sent with a relative delay, or at a set time. Messages with
	the same code can be merged according to various rules. Each message can
	have any number of target recipients.

	DelayedMessage is a throw-away object, it is to be created on the stack,
	Flush()'d, then left to be destructed when out of scope.
*/
class DelayedMessage {
	typedef void(*FailureCallback)(int32 code, port_id port, void* data);
public:
								DelayedMessage(int32 code, bigtime_t delay,
									bool isSpecificTime = false);

								~DelayedMessage();

			// At least one target port is required.
			bool				AddTarget(port_id port);

			// Merge messages with the same code according to the following
			// rules and data matching mask.
			void				SetMerge(DMMergeMode mode, uint32 match = 0);

			// Called for each port on which the message was failed to be sent.
			void				SetFailureCallback(FailureCallback callback,
									void* data = NULL);

			template <class Type>
			status_t			Attach(const Type& data);
			status_t			Attach(const void* data, size_t size);

			template <class Type>
			status_t			AttachList(const BObjectList<Type>& list);

			template <class Type>
			status_t			AttachList(const BObjectList<Type>& list,
									bool* whichArray);

			status_t			Flush();

		// Private
			DelayedMessageData*	HandOff();
			DelayedMessageData*	Data() {return fData;}

private:
		// Forbidden methods - these are one time-use objects.
			void*				operator new(size_t);
			void*				operator new[](size_t);

			DelayedMessageData*	fData;
			bool				fHandedOff;
};


// #pragma mark Implementation


template <class Type>
status_t
DelayedMessage::Attach(const Type& data)
{
	return Attach(&data, sizeof(Type));
}


template <class Type>
status_t
DelayedMessage::AttachList(const BObjectList<Type>& list)
{
	if (list.CountItems() == 0)
		return B_BAD_VALUE;

	status_t error = Attach<int32>(list.CountItems());

	for (int32 index = 0; index < list.CountItems(); ++index) {
		if (error != B_OK)
			break;

		error = Attach<Type>(*(list.ItemAt(index)));
	}

	return error;
}


template <class Type>
status_t
DelayedMessage::AttachList(const BObjectList<Type>& list, bool* which)
{
	if (list.CountItems() == 0)
		return B_BAD_VALUE;

	if (which == NULL)
		return AttachList(list);

	int32 count = 0;
	for (int32 index = 0; index < list.CountItems(); ++index) {
		if (which[index])
			++count;
	}

	if (count == 0)
		return B_BAD_VALUE;

	status_t error = Attach<int32>(count);

	for (int32 index = 0; index < list.CountItems(); ++index) {
		if (error != B_OK)
			break;

		if (which[index])
			error = Attach<Type>(*list.ItemAt(index));
	}

	return error;
}


#endif // AS_DELAYED_MESSAGE_H
