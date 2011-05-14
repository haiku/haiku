/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */


#include "QueryMonitor.h"


QueryHandler::QueryHandler(EntryViewInterface* listener)
	:
	FileMonitor(listener)
{
	
}


void
QueryHandler::MessageReceived(BMessage* message)
{
	int32 opcode;
	if (message->what == B_QUERY_UPDATE
		&& message->FindInt32("opcode", &opcode) == B_OK) {
		switch (opcode) {
			case B_ENTRY_CREATED:
			case B_ENTRY_REMOVED:
				message->what = B_NODE_MONITOR;
				break;
		}
	}

	FileMonitor::MessageReceived(message);
}


QueryReader::QueryReader(QueryHandler* handler)
	:
	ReadThread(handler)
{

}


QueryReader::~QueryReader()
{
	Reset();
}


bool
QueryReader::AddQuery(BQuery* query)
{
	query->SetTarget(fTarget);
	query->Fetch();
	return fQueries.AddItem(query);
}


void
QueryReader::Reset()
{
	Stop();
	Wait();

	for (int32 i = 0; i < fLiveQueries.CountItems(); i++)
		delete fLiveQueries.ItemAt(i);
	fLiveQueries.MakeEmpty();

	for (int32 i = 0; i < fQueries.CountItems(); i++)
		delete fQueries.ItemAt(i);
	fQueries.MakeEmpty();	
}


bool
QueryReader::ReadNextEntry(entry_ref& entry)
{
	BQuery* query = fQueries.ItemAt(0);
	if (query == NULL)
		return false;
	if (query->GetNextRef(&entry) != B_OK) {
		fQueries.RemoveItemAt(0);
		fLiveQueries.AddItem(query);
		return ReadNextEntry(entry);
	}
	return true;
}
