/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */


#include "JsonMessageWriter.h"


namespace BPrivate {

/*! The class and sub-classes of it are used as a stack internal to the
    BJsonMessageWriter class.  As the JSON is parsed, the stack of these
    internal listeners follows the stack of the JSON parsing in terms of
    containers; arrays and objects.
*/

class BStackedMessageEventListener : public BJsonEventListener {
public:
								BStackedMessageEventListener(
									BJsonMessageWriter* writer,
									BStackedMessageEventListener* parent,
									uint32 messageWhat);
								BStackedMessageEventListener(
									BJsonMessageWriter* writer,
									BStackedMessageEventListener* parent,
									BMessage* message);
								~BStackedMessageEventListener();

				bool			Handle(const BJsonEvent& event);
				void			HandleError(status_t status, int32 line,
									const char* message);
				void			Complete();

				void			AddMessage(BMessage* value);

				status_t		ErrorStatus();
		virtual	const char*		NextItemName() = 0;

				BStackedMessageEventListener*
								Parent();

protected:
				void			AddBool(bool value);
				void			AddNull();
				void			AddDouble(double value);
				void			AddString(const char* value);

		virtual bool			WillAdd();
		virtual void			DidAdd();

				void			SetStackedListenerOnWriter(
									BStackedMessageEventListener*
									stackedListener);

			BJsonMessageWriter*	fWriter;
			bool				fOwnsMessage;
			BStackedMessageEventListener
								*fParent;
			BMessage*			fMessage;
};


class BStackedArrayMessageEventListener : public BStackedMessageEventListener {
public:
								BStackedArrayMessageEventListener(
									BJsonMessageWriter* writer,
									BStackedMessageEventListener* parent);
								BStackedArrayMessageEventListener(
									BJsonMessageWriter* writer,
									BStackedMessageEventListener* parent,
									BMessage* message);
								~BStackedArrayMessageEventListener();

				bool			Handle(const BJsonEvent& event);

				const char*		NextItemName();

protected:
				void			DidAdd();

private:
				uint32			fCount;
				BString			fNextItemName;

};


class BStackedObjectMessageEventListener : public BStackedMessageEventListener {
public:
								BStackedObjectMessageEventListener(
									BJsonMessageWriter* writer,
									BStackedMessageEventListener* parent);
								BStackedObjectMessageEventListener(
									BJsonMessageWriter* writer,
									BStackedMessageEventListener* parent,
									BMessage* message);
								~BStackedObjectMessageEventListener();

				bool			Handle(const BJsonEvent& event);

				const char*		NextItemName();

protected:
				bool			WillAdd();
				void			DidAdd();
private:
				BString			fNextItemName;
};

} // namespace BPrivate

using BPrivate::BStackedMessageEventListener;
using BPrivate::BStackedArrayMessageEventListener;
using BPrivate::BStackedObjectMessageEventListener;


// #pragma mark - BStackedMessageEventListener


BStackedMessageEventListener::BStackedMessageEventListener(
	BJsonMessageWriter* writer,
	BStackedMessageEventListener* parent,
	uint32 messageWhat)
{
	fWriter = writer;
	fParent = parent;
	fOwnsMessage = true;
	fMessage = new BMessage(messageWhat);
}


BStackedMessageEventListener::BStackedMessageEventListener(
	BJsonMessageWriter* writer,
	BStackedMessageEventListener* parent,
	BMessage* message)
{
	fWriter = writer;
	fParent = parent;
	fOwnsMessage = false;
	fMessage = message;
}


BStackedMessageEventListener::~BStackedMessageEventListener()
{
	if (fOwnsMessage)
		delete fMessage;
}


bool
BStackedMessageEventListener::Handle(const BJsonEvent& event)
{
	if (fWriter->ErrorStatus() != B_OK)
		return false;

	switch (event.EventType()) {

		case B_JSON_NUMBER:
			AddDouble(event.ContentDouble());
			break;

		case B_JSON_STRING:
			AddString(event.Content());
			break;

		case B_JSON_TRUE:
			AddBool(true);
			break;

		case B_JSON_FALSE:
			AddBool(false);
			break;

		case B_JSON_NULL:
			AddNull();
			break;

		case B_JSON_OBJECT_START:
		{
			SetStackedListenerOnWriter(new BStackedObjectMessageEventListener(
				fWriter, this));
			break;
		}

		case B_JSON_ARRAY_START:
		{
			SetStackedListenerOnWriter(new BStackedArrayMessageEventListener(
				fWriter, this));
			break;
		}

		default:
		{
			HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
				"unexpected type of json item to add to container");
			return false;
		}
	}

	return ErrorStatus() == B_OK;
}


void
BStackedMessageEventListener::HandleError(status_t status, int32 line,
	const char* message)
{
	fWriter->HandleError(status, line, message);
}


void
BStackedMessageEventListener::Complete()
{
	// illegal state.
	HandleError(JSON_EVENT_LISTENER_ANY_LINE, B_NOT_ALLOWED,
		"Complete() called on stacked message listener");
}


void
BStackedMessageEventListener::AddMessage(BMessage* message)
{
	if (WillAdd()) {
		fMessage->AddMessage(NextItemName(), message);
		DidAdd();
	}
}


status_t
BStackedMessageEventListener::ErrorStatus()
{
	return fWriter->ErrorStatus();
}


BStackedMessageEventListener*
BStackedMessageEventListener::Parent()
{
	return fParent;
}


void
BStackedMessageEventListener::AddBool(bool value)
{
	if (WillAdd()) {
		fMessage->AddBool(NextItemName(), value);
		DidAdd();
	}
}

void
BStackedMessageEventListener::AddNull()
{
	if (WillAdd()) {
		fMessage->AddPointer(NextItemName(), (void*)NULL);
		DidAdd();
	}
}

void
BStackedMessageEventListener::AddDouble(double value)
{
	if (WillAdd()) {
		fMessage->AddDouble(NextItemName(), value);
		DidAdd();
	}
}

void
BStackedMessageEventListener::AddString(const char* value)
{
	if (WillAdd()) {
		fMessage->AddString(NextItemName(), value);
		DidAdd();
	}
}


bool
BStackedMessageEventListener::WillAdd()
{
	return true;
}


void
BStackedMessageEventListener::DidAdd()
{
	// noop - present for overriding
}


void
BStackedMessageEventListener::SetStackedListenerOnWriter(
	BStackedMessageEventListener* stackedListener)
{
	fWriter->SetStackedListener(stackedListener);
}


// #pragma mark - BStackedArrayMessageEventListener


BStackedArrayMessageEventListener::BStackedArrayMessageEventListener(
	BJsonMessageWriter* writer,
	BStackedMessageEventListener* parent)
	:
	BStackedMessageEventListener(writer, parent, B_JSON_MESSAGE_WHAT_ARRAY)
{
	fCount = 0;
}


BStackedArrayMessageEventListener::BStackedArrayMessageEventListener(
	BJsonMessageWriter* writer,
	BStackedMessageEventListener* parent,
	BMessage* message)
	:
	BStackedMessageEventListener(writer, parent, message)
{
	message->what = B_JSON_MESSAGE_WHAT_ARRAY;
	fCount = 0;
}


BStackedArrayMessageEventListener::~BStackedArrayMessageEventListener()
{
}


bool
BStackedArrayMessageEventListener::Handle(const BJsonEvent& event)
{
	if (fWriter->ErrorStatus() != B_OK)
		return false;

	switch (event.EventType()) {
		case B_JSON_ARRAY_END:
		{
			if (fParent != NULL)
				fParent->AddMessage(fMessage);
			SetStackedListenerOnWriter(fParent);
			delete this;
			break;
		}

		default:
			return BStackedMessageEventListener::Handle(event);
	}

	return true;
}


const char*
BStackedArrayMessageEventListener::NextItemName()
{
	fNextItemName.SetToFormat("%" B_PRIu32, fCount);
	return fNextItemName.String();
}


void
BStackedArrayMessageEventListener::DidAdd()
{
	BStackedMessageEventListener::DidAdd();
	fCount++;
}


// #pragma mark - BStackedObjectMessageEventListener


BStackedObjectMessageEventListener::BStackedObjectMessageEventListener(
	BJsonMessageWriter* writer,
	BStackedMessageEventListener* parent)
	:
	BStackedMessageEventListener(writer, parent, B_JSON_MESSAGE_WHAT_OBJECT)
{
}


BStackedObjectMessageEventListener::BStackedObjectMessageEventListener(
	BJsonMessageWriter* writer,
	BStackedMessageEventListener* parent,
	BMessage* message)
	:
	BStackedMessageEventListener(writer, parent, message)
{
	message->what = B_JSON_MESSAGE_WHAT_OBJECT;
}


BStackedObjectMessageEventListener::~BStackedObjectMessageEventListener()
{
}


bool
BStackedObjectMessageEventListener::Handle(const BJsonEvent& event)
{
	if (fWriter->ErrorStatus() != B_OK)
		return false;

	switch (event.EventType()) {
		case B_JSON_OBJECT_END:
		{
			if (fParent != NULL)
				fParent->AddMessage(fMessage);
			SetStackedListenerOnWriter(fParent);
			delete this;
			break;
		}

		case B_JSON_OBJECT_NAME:
			fNextItemName.SetTo(event.Content());
			break;

		default:
			return BStackedMessageEventListener::Handle(event);
	}

	return true;
}


const char*
BStackedObjectMessageEventListener::NextItemName()
{
	return fNextItemName.String();
}


bool
BStackedObjectMessageEventListener::WillAdd()
{
	if (0 == fNextItemName.Length()) {
		HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
				"missing name for adding value into an object");
		return false;
	}

	return true;
}


void
BStackedObjectMessageEventListener::DidAdd()
{
	BStackedMessageEventListener::DidAdd();
	fNextItemName.SetTo("", 0);
}


// #pragma mark - BJsonMessageWriter


BJsonMessageWriter::BJsonMessageWriter(BMessage& message)
{
	fTopLevelMessage = &message;
	fStackedListener = NULL;
}


BJsonMessageWriter::~BJsonMessageWriter()
{
	BStackedMessageEventListener* listener = fStackedListener;

	while (listener != NULL) {
		BStackedMessageEventListener* nextListener = listener->Parent();
		delete listener;
		listener = nextListener;
	}

	fStackedListener = NULL;
}


bool
BJsonMessageWriter::Handle(const BJsonEvent& event)
{
	if (fErrorStatus != B_OK)
		return false;

	if (fStackedListener != NULL)
		return fStackedListener->Handle(event);
	else {
		switch(event.EventType()) {
			case B_JSON_OBJECT_START:
			{
				SetStackedListener(new BStackedObjectMessageEventListener(
					this, NULL, fTopLevelMessage));
				break;
			}

			case B_JSON_ARRAY_START:
			{
				fTopLevelMessage->what = B_JSON_MESSAGE_WHAT_ARRAY;
				SetStackedListener(new BStackedArrayMessageEventListener(
					this, NULL, fTopLevelMessage));
				break;
			}

			default:
			{
				HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
					"a message object can only handle an object or an array"
					"at the top level");
				return false;
			}
		}
	}

	return true; // keep going
}


void
BJsonMessageWriter::Complete()
{
	if (fStackedListener != NULL) {
		HandleError(B_BAD_DATA, JSON_EVENT_LISTENER_ANY_LINE,
			"unexpected end of input data processing structure");
	}
}


void
BJsonMessageWriter::SetStackedListener(
	BStackedMessageEventListener* listener)
{
	fStackedListener = listener;
}