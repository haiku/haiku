/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "Notifications.h"

#include "Directory.h"
#include "Volume.h"


// #pragma mark - EntryCreatedNotification


EntryCreatedNotification::EntryCreatedNotification(Directory* directory,
	const char* name, Node* node)
	:
	fDirectory(directory),
	fName(name),
	fNode(node)
{
}


void
EntryCreatedNotification::NotifyPostCommit() const
{
	notify_entry_created(fDirectory->GetVolume()->ID(),
		fDirectory->BlockIndex(), fName, fNode->BlockIndex());
}


// #pragma mark - EntryRemovedNotification


EntryRemovedNotification::EntryRemovedNotification(Directory* directory,
	const char* name, Node* node)
	:
	fDirectory(directory),
	fName(name),
	fNode(node)
{
}


void
EntryRemovedNotification::NotifyPostCommit() const
{
	notify_entry_removed(fDirectory->GetVolume()->ID(),
		fDirectory->BlockIndex(), fName, fNode->BlockIndex());
}


// #pragma mark - EntryMovedNotification


EntryMovedNotification::EntryMovedNotification(Directory* fromDirectory,
	const char* fromName, Directory* toDirectory, const char* toName,
	Node* node)
	:
	fFromDirectory(fromDirectory),
	fFromName(fromName),
	fToDirectory(toDirectory),
	fToName(toName),
	fNode(node)
{
}


void
EntryMovedNotification::NotifyPostCommit() const
{
	notify_entry_moved(fFromDirectory->GetVolume()->ID(),
		fFromDirectory->BlockIndex(), fFromName, fToDirectory->BlockIndex(),
		fToName, fNode->BlockIndex());
}


// #pragma mark - StatChangedNotification


StatChangedNotification::StatChangedNotification(Node* node, uint32 statFields)
	:
	fNode(node),
	fStatFields(statFields)
{
}


void
StatChangedNotification::NotifyPostCommit() const
{
	notify_stat_changed(fNode->GetVolume()->ID(), fNode->BlockIndex(),
		fStatFields);
}


// #pragma mark - AttributeChangedNotification


AttributeChangedNotification::AttributeChangedNotification(Node* node,
	const char* attribute, int32 cause)
	:
	fNode(node),
	fAttribute(attribute),
	fCause(cause)
{
}


void
AttributeChangedNotification::NotifyPostCommit() const
{
	notify_attribute_changed(fNode->GetVolume()->ID(), fNode->BlockIndex(),
		fAttribute, fCause);
}
