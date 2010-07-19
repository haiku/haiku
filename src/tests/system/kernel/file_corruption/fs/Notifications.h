/*
 * Copyright 2010, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef NOTIFICATIONS_H
#define NOTIFICATIONS_H


#include "Transaction.h"


class Directory;


class EntryCreatedNotification : public PostCommitNotification {
public:
								EntryCreatedNotification(Directory* directory,
									const char* name, Node* node);

	virtual	void				NotifyPostCommit() const;

private:
			Directory*			fDirectory;
			const char*			fName;
			Node*				fNode;
};


class EntryRemovedNotification : public PostCommitNotification {
public:
								EntryRemovedNotification(Directory* directory,
									const char* name, Node* node);

	virtual	void				NotifyPostCommit() const;

private:
			Directory*			fDirectory;
			const char*			fName;
			Node*				fNode;
};


class EntryMovedNotification : public PostCommitNotification {
public:
								EntryMovedNotification(Directory* fromDirectory,
									const char* fromName,
									Directory* toDirectory, const char* toName,
									Node* node);

	virtual	void				NotifyPostCommit() const;

private:
			Directory*			fFromDirectory;
			const char*			fFromName;
			Directory*			fToDirectory;
			const char*			fToName;
			Node*				fNode;
};


class StatChangedNotification : public PostCommitNotification {
public:
								StatChangedNotification(Node* node,
									uint32 statFields);

	virtual	void				NotifyPostCommit() const;

private:
			Node*				fNode;
			uint32				fStatFields;
};


class AttributeChangedNotification : public PostCommitNotification {
public:
								AttributeChangedNotification(Node* node,
									const char* attribute, int32 cause);

	virtual	void				NotifyPostCommit() const;

private:
			Node*				fNode;
			const char*			fAttribute;
			int32				fCause;
};


#endif	// NOTIFICATIONS_H
