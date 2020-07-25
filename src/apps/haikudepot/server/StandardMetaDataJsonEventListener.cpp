/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "StandardMetaDataJsonEventListener.h"

#include "Logger.h"


#define KEY_CREATE_TIMESTAMP "createTimestamp"
#define KEY_DATA_MODIFIED_TIMESTAMP "dataModifiedTimestamp"


class SmdStackedEventListener : public BJsonEventListener {
public:
								SmdStackedEventListener(
									StandardMetaDataJsonEventListener*
										mainListener,
									SmdStackedEventListener* parent);
								~SmdStackedEventListener();

				void			HandleError(status_t status, int32 line,
									const char* message);
				void			Complete();

				status_t		ErrorStatus();

				SmdStackedEventListener*
								Parent();
				StandardMetaData*
								MetaData();

protected:
				void			SetStackedListenerMainListener(
									SmdStackedEventListener* stackedListener);

			StandardMetaDataJsonEventListener*
								fMainListener;
			SmdStackedEventListener*
								fParent;
};


class SmdStackedArrayEventListener : public SmdStackedEventListener {
public:
								SmdStackedArrayEventListener(
									StandardMetaDataJsonEventListener*
										mainListener,
									SmdStackedEventListener* parent);
								~SmdStackedArrayEventListener();

				bool			Handle(const BJsonEvent& event);

};


class SmdStackedObjectMessageEventListener : public SmdStackedEventListener {
public:
								SmdStackedObjectMessageEventListener(
									BStringList* jsonPathObjectNames,
									StandardMetaDataJsonEventListener*
										mainListener,
									SmdStackedEventListener* parent);
								~SmdStackedObjectMessageEventListener();

				bool			Handle(const BJsonEvent& event);

private:
			BStringList*		fJsonPathObjectNames;
			BString				fNextItemName;
};


// #pragma mark - SmdStackedEventListener

SmdStackedEventListener::SmdStackedEventListener(
	StandardMetaDataJsonEventListener* mainListener,
	SmdStackedEventListener* parent)
{
	fMainListener = mainListener;
	fParent = parent;
}


SmdStackedEventListener::~SmdStackedEventListener()
{
}


void
SmdStackedEventListener::HandleError(status_t status, int32 line,
	const char* message)
{
	fMainListener->HandleError(status, line, message);
}


void
SmdStackedEventListener::Complete()
{
	fMainListener->Complete();
}


status_t
SmdStackedEventListener::ErrorStatus()
{
	return fMainListener->ErrorStatus();
}


StandardMetaData*
SmdStackedEventListener::MetaData()
{
	return fMainListener->MetaData();
}


SmdStackedEventListener*
SmdStackedEventListener::Parent()
{
	return fParent;
}


void
SmdStackedEventListener::SetStackedListenerMainListener(
	SmdStackedEventListener* stackedListener)
{
	fMainListener->SetStackedListener(stackedListener);
}


// #pragma mark - SmdStackedArrayEventListener


SmdStackedArrayEventListener::SmdStackedArrayEventListener(
	StandardMetaDataJsonEventListener* mainListener,
	SmdStackedEventListener* parent)
	:
	SmdStackedEventListener(mainListener, parent)
{
}


SmdStackedArrayEventListener::~SmdStackedArrayEventListener()
{
}


bool
SmdStackedArrayEventListener::Handle(const BJsonEvent& event)
{
	if (ErrorStatus() != B_OK)
		return false;

	switch (event.EventType()) {

		case B_JSON_NUMBER:
		case B_JSON_STRING:
		case B_JSON_TRUE:
		case B_JSON_FALSE:
		case B_JSON_NULL:
			// ignore these atomic types in an array.
			break;

		case B_JSON_OBJECT_START:
			SetStackedListenerMainListener(
				new SmdStackedObjectMessageEventListener(NULL, fMainListener,
					this));
			break;

		case B_JSON_ARRAY_START:
			SetStackedListenerMainListener(new SmdStackedArrayEventListener(
				fMainListener, this));
			break;

		case B_JSON_ARRAY_END:
			SetStackedListenerMainListener(fParent);
			delete this;
			break;

		case B_JSON_OBJECT_END:
		case B_JSON_OBJECT_NAME:
			HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
				"illegal state when processing events in an array");
			return false;
	}

	return true;
}


// #pragma mark - SmdStackedObjectMessageEventListener

SmdStackedObjectMessageEventListener::SmdStackedObjectMessageEventListener(
	BStringList* jsonPathObjectNames,
	StandardMetaDataJsonEventListener* mainListener,
	SmdStackedEventListener* parent)
	:
	SmdStackedEventListener(mainListener, parent)
{
	fJsonPathObjectNames = jsonPathObjectNames;
}


SmdStackedObjectMessageEventListener::~SmdStackedObjectMessageEventListener()
{
	if (fJsonPathObjectNames != NULL)
		delete fJsonPathObjectNames;
}


bool
SmdStackedObjectMessageEventListener::Handle(const BJsonEvent& event)
{
	if (ErrorStatus() != B_OK)
		return false;

	switch (event.EventType()) {

		case B_JSON_OBJECT_NAME:
			fNextItemName = event.Content();
			break;

		case B_JSON_NUMBER:
			if (fJsonPathObjectNames != NULL
				&& fJsonPathObjectNames->IsEmpty()) {

				if (fNextItemName == KEY_CREATE_TIMESTAMP) {
					MetaData()->SetCreateTimestamp(
						event.ContentInteger());
				}

				if (fNextItemName == KEY_DATA_MODIFIED_TIMESTAMP) {
					MetaData()->SetDataModifiedTimestamp(
						event.ContentInteger());
				}
			}
			break;

		case B_JSON_STRING:
		case B_JSON_TRUE:
		case B_JSON_FALSE:
		case B_JSON_NULL:
			// ignore these atomic types as they are not required to fill the
			// data structure.
			break;

		case B_JSON_OBJECT_START:
		{
			BStringList* nextJsonPathObjectNames = NULL;

			// if this next object is on the path then remove it from the
			// path and carry on down.  If it's not on the path then just
			// drop the path from the next object.

			if (fJsonPathObjectNames != NULL
				&& !fJsonPathObjectNames->IsEmpty()
				&& fNextItemName == fJsonPathObjectNames->StringAt(0)) {
				nextJsonPathObjectNames = new BStringList(*fJsonPathObjectNames);
				nextJsonPathObjectNames->Remove(0);
			}

			SetStackedListenerMainListener(
				new SmdStackedObjectMessageEventListener(nextJsonPathObjectNames,
					fMainListener, this));
			break;
		}

		case B_JSON_ARRAY_START:
			SetStackedListenerMainListener(new SmdStackedArrayEventListener(
				fMainListener, this));
			break;

		case B_JSON_OBJECT_END:
			SetStackedListenerMainListener(fParent);
			delete this;
			break;

		case B_JSON_ARRAY_END:
			HandleError(B_NOT_ALLOWED, JSON_EVENT_LISTENER_ANY_LINE,
				"illegal state when processing events in an array");
			return false;
	}

	return true;
}


// #pragma mark - StandardMetaDataJsonEventListener


StandardMetaDataJsonEventListener::StandardMetaDataJsonEventListener(
	const BString& jsonPath,
	StandardMetaData& metaData)
{
	fMetaData = &metaData;
	SetJsonPath(jsonPath);
	fErrorStatus = B_OK;
	fStackedListener = NULL;
}


StandardMetaDataJsonEventListener::~StandardMetaDataJsonEventListener()
{
	if (fStackedListener != NULL)
		delete fStackedListener;
}


void
StandardMetaDataJsonEventListener::SetJsonPath(const BString& jsonPath)
{
	jsonPath.Split(".", true, fJsonPathObjectNames);

	if (fJsonPathObjectNames.IsEmpty()) {
		HandleError(B_BAD_VALUE, JSON_EVENT_LISTENER_ANY_LINE,
			"json path required");
	} else {
		if (fJsonPathObjectNames.First() != "$") {
			HandleError(B_BAD_VALUE, JSON_EVENT_LISTENER_ANY_LINE,
				"illegal json path; should start with '$");
		}
	}
}


void
StandardMetaDataJsonEventListener::SetStackedListener(
	SmdStackedEventListener *listener)
{
	fStackedListener = listener;
}


bool
StandardMetaDataJsonEventListener::Handle(const BJsonEvent& event)
{
	if (fErrorStatus != B_OK)
		return false;

		// best to exit early if the parsing has obtained all of the required
		// data already.

	if (fMetaData->IsPopulated())
		return false;

	if (fStackedListener != NULL)
		return fStackedListener->Handle(event);

		// the first thing that comes in must be an object container.  It is
		// bad data if it does not start with an object container.

	switch (event.EventType()) {

		case B_JSON_OBJECT_START:
		{
			BStringList* jsonPathObjectNames = new BStringList(
				fJsonPathObjectNames);
			jsonPathObjectNames->Remove(0);

			SetStackedListener(
				new SmdStackedObjectMessageEventListener(
					jsonPathObjectNames, this, NULL)
			);
		}
		break;

		default:
			HandleError(B_BAD_DATA, JSON_EVENT_LISTENER_ANY_LINE,
				"the top level element must be an object");
			return false;

	}

	return true;
}


void
StandardMetaDataJsonEventListener::HandleError(status_t status, int32 line,
	const char* message)
{
	HDERROR("an error has arisen processing the standard "
		"meta data; %s", message);
	fErrorStatus = status;
}


void
StandardMetaDataJsonEventListener::Complete()
{
}


status_t
StandardMetaDataJsonEventListener::ErrorStatus()
{
	return fErrorStatus;
}


StandardMetaData*
StandardMetaDataJsonEventListener::MetaData()
{
	return fMetaData;
}