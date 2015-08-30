/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PEOPLE_H
#define PEOPLE_H


#include <map>

#include <Locker.h>
#include <ObjectList.h>
#include <String.h>
#include <StringList.h>

#include "QueryList.h"


class Person {
public:
								Person(const entry_ref& ref);
	virtual						~Person();

			const BString&		Name() const
									{ return fName; }

			int32				CountAddresses() const
									{ return fAddresses.CountStrings(); }
			BString				AddressAt(int32 index) const
									{ return fAddresses.StringAt(index); }

			int32				CountGroups() const
									{ return fGroups.CountStrings(); }
			BString				GroupAt(int32 index) const
									{ return fGroups.StringAt(index); }
			bool				IsInGroup(const char* group) const;

private:
			BString				fName;
			BStringList			fAddresses;
			BStringList			fGroups;
};


class PersonList : public QueryListener, public BLocker {
public:
								PersonList(QueryList& query);
								~PersonList();

			int32				CountPersons() const
									{ return fPersons.CountItems(); }
			const Person*		PersonAt(int32 index) const
									{ return fPersons.ItemAt(index); }

	virtual	void				EntryCreated(QueryList& source,
									const entry_ref& ref, ino_t node);
	virtual	void				EntryRemoved(QueryList& source,
									const node_ref& nodeRef);

private:
	typedef std::map<node_ref, Person*> PersonMap;

			QueryList&			fQueryList;
			BObjectList<Person>	fPersons;
			PersonMap			fPersonMap;
};


class GroupList : public QueryListener, public BLocker {
public:
								GroupList(QueryList& query);
								~GroupList();

			int32				CountGroups() const
									{ return fGroups.CountStrings(); }
			BString				GroupAt(int32 index) const
									{ return fGroups.StringAt(index); }

	virtual	void				EntryCreated(QueryList& source,
									const entry_ref& ref, ino_t node);
	virtual	void				EntryRemoved(QueryList& source,
									const node_ref& nodeRef);

private:
	typedef std::map<BString, int> StringCountMap;

			QueryList&			fQueryList;
			BStringList			fGroups;
			StringCountMap		fGroupMap;
};


#endif // ADDRESS_TEXT_CONTROL_H

