/*
 * Copyright 2002-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Wilber
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "TranslatorRosterPrivate.h"

#include <Translator.h>


BTranslator::BTranslator()
	:
	fOwningRoster(NULL),
	fID(0),
	fRefCount(1)
{
}


BTranslator::~BTranslator()
{
	if (fOwningRoster != NULL)
		fOwningRoster->TranslatorDeleted(fID);
}


/*!
	Increments the refcount and returns a pointer to this object.
*/
BTranslator *BTranslator::Acquire()
{
	if (atomic_add(&fRefCount, 1) > 0)
		return this;

	return NULL;
}


/*!
	Decrements the refcount and returns a pointer to this object.
	When the refcount hits zero, the object is destroyed. This is
	so multiple objects can own the BTranslator and it won't get
	deleted until all of them are done with it.

	\return NULL, if the object was just deleted
*/
BTranslator *BTranslator::Release()
{
	int32 oldValue = atomic_add(&fRefCount, -1);
	if (oldValue > 0)
		return this;

	delete this;
	return NULL;
}


int32
BTranslator::ReferenceCount()
{
	return fRefCount;
}


/*!
	This virtual function is for creating a configuration view
	for the translator so the user can change its settings.
	This method is optional.
*/
status_t
BTranslator::MakeConfigurationView(BMessage* ioExtension,
	BView** outView, BRect* outExtent)
{
	return B_ERROR;
}


/*!
	Puts the current configuration for the translator into
	ioExtension. This method is optional.
*/
status_t
BTranslator::GetConfigurationMessage(BMessage* ioExtension)
{
	return B_ERROR;
}


status_t BTranslator::_Reserved_Translator_0(int32 n, void *p) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_1(int32 n, void *p) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_2(int32 n, void *p) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_3(int32 n, void *p) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_4(int32 n, void *p) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_5(int32 n, void *p) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_6(int32 n, void *p) { return B_ERROR; }
status_t BTranslator::_Reserved_Translator_7(int32 n, void *p) { return B_ERROR; }
