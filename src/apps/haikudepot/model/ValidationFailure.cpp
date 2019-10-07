/*
 * Copyright 2019, Andrew Lindesay <apl@lindesay.co.nz>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "ValidationFailure.h"

// These are keys that are used to store this object's data into a BMessage
// instance.

#define KEY_PROPERTY							"property"
#define KEY_PREFIX_MESSAGE						"message_"
#define KEY_PREFIX_ITEM							"item_"


// #pragma mark - Single Validation Failure


ValidationFailure::ValidationFailure(BMessage* from)
{
	from->FindString(KEY_PROPERTY, &fProperty);

	if (fProperty.IsEmpty())
		debugger("illegal state; missing property in message");

	status_t result = B_OK;
	BString name;
	BString message;

	for (int32 i = 0; result == B_OK; i++) {
		name.SetToFormat("%s%" B_PRId32, KEY_PREFIX_MESSAGE, i);
		result = from->FindString(name, &message);

		if (result == B_OK)
			AddMessage(message);
	}
}


ValidationFailure::ValidationFailure(const BString& property)
{
	fProperty = property;
}


ValidationFailure::~ValidationFailure()
{
}


const BString&
ValidationFailure::Property() const
{
	return fProperty;
}


const BStringList&
ValidationFailure::Messages() const
{
	return fMessages;
}


bool
ValidationFailure::IsEmpty() const
{
	return fMessages.IsEmpty();
}


bool
ValidationFailure::Contains(const BString& message) const
{
	return fMessages.HasString(message);
}


void
ValidationFailure::AddMessage(const BString& value)
{
	fMessages.Add(value);
}


status_t
ValidationFailure::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	BString key;
	if (result == B_OK)
		result = into->AddString(KEY_PROPERTY, fProperty);
	for (int32 i = 0; result == B_OK && i < fMessages.CountStrings(); i++) {
		key.SetToFormat("%s%" B_PRId32, KEY_PREFIX_MESSAGE, i);
		result = into->AddString(key, fMessages.StringAt(i));
	}
	return result;
}


// #pragma mark - Collections of Validation Failures


ValidationFailures::ValidationFailures(BMessage* from)
	:
	fItems(20, true)
{
	_AddFromMessage(from);
}


ValidationFailures::ValidationFailures()
	:
	fItems(20, true)
{
}


ValidationFailures::~ValidationFailures()
{
}


void
ValidationFailures::AddFailure(const BString& property, const BString& message)
{
	_GetOrCreateFailure(property)->AddMessage(message);
}


int32
ValidationFailures::CountFailures() const
{
	return fItems.CountItems();
}


bool
ValidationFailures::IsEmpty() const
{
	return fItems.IsEmpty();
}


bool
ValidationFailures::Contains(const BString& property) const
{
	ValidationFailure* failure = _GetFailure(property);
	return failure != NULL && !failure->IsEmpty();
}


bool
ValidationFailures::Contains(const BString& property,
	const BString& message) const
{
	ValidationFailure* failure = _GetFailure(property);
	return failure != NULL && failure->Contains(message);
}


ValidationFailure*
ValidationFailures::FailureAtIndex(int32 index) const
{
	return fItems.ItemAt(index);
}


status_t
ValidationFailures::Archive(BMessage* into, bool deep) const
{
	status_t result = B_OK;
	BString key;

	for (int32 i = 0; i < fItems.CountItems() && result == B_OK; i++) {
		ValidationFailure* item = fItems.ItemAt(i);
		BMessage itemMessage;
		result = item->Archive(&itemMessage);
		if (result == B_OK) {
			key.SetToFormat("%s%" B_PRId32, KEY_PREFIX_ITEM, i);
			result = into->AddMessage(key, &itemMessage);
		}
	}

	return result;
}


ValidationFailure*
ValidationFailures::_GetFailure(const BString& property) const
{
	for (int32 i = 0; i < fItems.CountItems(); i++) {
		ValidationFailure* item = fItems.ItemAt(i);
		if (item->Property() == property)
			return item;
	}

	return NULL;
}


ValidationFailure*
ValidationFailures::_GetOrCreateFailure(const BString& property)
{
	ValidationFailure* item = _GetFailure(property);

	if (item == NULL) {
		item = new ValidationFailure(property);
		fItems.AddItem(item);
	}

	return item;
}


void
ValidationFailures::_AddFromMessage(const BMessage* from)
{
	int32 i = 0;
	BString key;

	while (true) {
		BMessage itemMessage;
		key.SetToFormat("%s%" B_PRId32, KEY_PREFIX_ITEM, i);
		if (from->FindMessage(key, &itemMessage) != B_OK)
			return;
		fItems.AddItem(new ValidationFailure(&itemMessage));
		i++;
	}
}
