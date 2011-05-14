/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Clemens Zeidler <haiku@clemens-zeidler.de>
 */
#ifndef QUERY_MONITOR_H
#define QUERY_MONITOR_H


#include <Query.h>

#include <ObjectList.h>

#include "FileMonitor.h"


/*! Handle live query messages, query reader messages, and node monitor messages
and dispatch them to a EntryViewInterface. */
class QueryHandler : public FileMonitor {
public:
								QueryHandler(EntryViewInterface* listener);

			void				MessageReceived(BMessage* message);
};


typedef BObjectList<BQuery> BQueryList;


class QueryReader : public ReadThread {
public:
								QueryReader(QueryHandler* handler);
								~QueryReader();

			bool				AddQuery(BQuery* query);
			void				Reset();

protected:
			bool				ReadNextEntry(entry_ref& entry);
		
private:
			BQueryList			fQueries;
			BQueryList			fLiveQueries;
};


#endif // QUERY_MONITOR_H
