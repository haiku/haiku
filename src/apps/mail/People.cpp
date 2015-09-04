/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "People.h"

#include <stdio.h>

#include <Autolock.h>
#include <Node.h>


static BString
PersonName(BNode& node)
{
	BString fullName;
	node.ReadAttrString("META:name", &fullName);

	return fullName;
}


static void
AddPersonAddresses(BNode& node, BStringList& addresses)
{
	BString email;
	if (node.ReadAttrString("META:email", &email) != B_OK || email.IsEmpty())
		return;

	addresses.Add(email);

	// Support for 3rd-party People apps
	for (int i = 2; i < 99; i++) {
		char attr[32];
		snprintf(attr, sizeof(attr), "META:email%d", i);

		if (node.ReadAttrString(attr, &email) != B_OK)
			break;

		addresses.Add(email);
	}
}


static void
AddPersonGroups(BNode& node, BStringList& groups)
{
	BString groupString;
	if (node.ReadAttrString("META:group", &groupString) != B_OK
		|| groupString.IsEmpty()) {
		return;
	}

	int first = 0;
	while (first < groupString.Length()) {
		int end = groupString.FindFirst(',', first);
		if (end < 0)
			end = groupString.Length();

		BString group;
		groupString.CopyInto(group, first, end - first);
		group.Trim();
		groups.Add(group);

		first = end + 1;
	}
}


// #pragma mark - Person


Person::Person(const entry_ref& ref)
{
	BNode node(&ref);
	if (node.InitCheck() != B_OK)
		return;

	fName = PersonName(node);
	AddPersonAddresses(node, fAddresses);
	AddPersonGroups(node, fGroups);
}


Person::~Person()
{
}


bool
Person::IsInGroup(const char* group) const
{
	for (int32 index = 0; index < CountGroups(); index++) {
		if (GroupAt(index) == group)
			return true;
	}
	return false;
}


// #pragma mark - PersonList


PersonList::PersonList(QueryList& query)
	:
	fQueryList(query),
	fPersons(10, true)
{
	fQueryList.AddListener(this);
}


PersonList::~PersonList()
{
	fQueryList.RemoveListener(this);
}


void
PersonList::EntryCreated(QueryList& source, const entry_ref& ref, ino_t node)
{
	BAutolock locker(this);

	Person* person = new Person(ref);
	fPersons.AddItem(person);
	fPersonMap.insert(std::make_pair(node_ref(ref.device, node), person));
}


void
PersonList::EntryRemoved(QueryList& source, const node_ref& nodeRef)
{
	BAutolock locker(this);

	PersonMap::iterator found = fPersonMap.find(nodeRef);
	if (found != fPersonMap.end()) {
		Person* person = found->second;
		fPersons.RemoveItem(person);
		fPersonMap.erase(found);
		delete person;
	}
}


// #pragma mark - GroupList


GroupList::GroupList(QueryList& query)
	:
	fQueryList(query)
{
	fQueryList.AddListener(this);
}


GroupList::~GroupList()
{
	fQueryList.RemoveListener(this);
}


void
GroupList::EntryCreated(QueryList& source, const entry_ref& ref, ino_t _node)
{
	BNode node(&ref);
	if (node.InitCheck() != B_OK)
		return;

	BAutolock locker(this);

	BStringList groups;
	AddPersonGroups(node, groups);

	for (int32 index = 0; index < groups.CountStrings(); index++) {
		BString group = groups.StringAt(index);

		StringCountMap::iterator found = fGroupMap.find(group);
		if (found != fGroupMap.end())
			found->second++;
		else {
			fGroupMap[group] = 1;
			fGroups.Add(group);
		}
	}

	// TODO: sort groups
}


void
GroupList::EntryRemoved(QueryList& source, const node_ref& nodeRef)
{
	// TODO!
}
