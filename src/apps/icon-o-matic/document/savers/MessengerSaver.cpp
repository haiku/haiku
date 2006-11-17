/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "MessengerSaver.h"

#include <IconEditorProtocol.h>
#include <Message.h>

#include "Document.h"
#include "FlatIconExporter.h"

// constructor
MessengerSaver::MessengerSaver(const BMessenger& messenger)
	: fMessenger(messenger)
{
}

// destructor
MessengerSaver::~MessengerSaver()
{
}

// Save
status_t
MessengerSaver::Save(Document* document)
{
#if HAIKU_TARGET_PLATFORM_HAIKU
	if (!fMessenger.IsValid())
		return B_NO_INIT;

	FlatIconExporter exporter;
	BMallocIO stream;
	status_t ret = exporter.Export(document->Icon(), &stream);
	if (ret < B_OK)
		return ret;

	BMessage message(B_ICON_DATA_EDITED);
	ret = message.AddData("icon data", B_VECTOR_ICON_TYPE,
						  stream.Buffer(), stream.BufferLength());
	if (ret < B_OK)
		return ret;

	return fMessenger.SendMessage(&message);
#else
	return B_ERROR;
#endif
}

