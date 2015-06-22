/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "Conditions.h"

#include <stdio.h>

#include <Entry.h>
#include <ObjectList.h>
#include <Message.h>
#include <StringList.h>

#include <DiskDevice.h>
#include <DiskDeviceRoster.h>
#include <Volume.h>


class ConditionContainer : public Condition {
protected:
								ConditionContainer(const BMessage& args);

			void				AddCondition(Condition* condition);

protected:
			BObjectList<Condition> fConditions;
};


class AndCondition : public ConditionContainer {
public:
								AndCondition(const BMessage& args);

	virtual	bool				Test(ConditionContext& context) const;
};


class OrCondition : public ConditionContainer {
public:
								OrCondition(const BMessage& args);

	virtual	bool				Test(ConditionContext& context) const;
};


class NotCondition : public ConditionContainer {
public:
								NotCondition(const BMessage& args);

	virtual	bool				Test(ConditionContext& context) const;
};


class SafeModeCondition : public Condition {
public:
	virtual	bool				Test(ConditionContext& context) const;
};


class ReadOnlyCondition : public Condition {
public:
								ReadOnlyCondition(const BMessage& args);

	virtual	bool				Test(ConditionContext& context) const;

private:
			dev_t				fDevice;
};


class FileExistsCondition : public Condition {
public:
								FileExistsCondition(const BMessage& args);

	virtual	bool				Test(ConditionContext& context) const;

private:
			BStringList			fPaths;
};


static Condition*
create_condition(const char* name, const BMessage& args)
{
	if (strcmp(name, "and") == 0)
		return new AndCondition(args);
	if (strcmp(name, "or") == 0)
		return new OrCondition(args);
	if (strcmp(name, "not") == 0)
		return new NotCondition(args);

	if (strcmp(name, "safemode") == 0)
		return new SafeModeCondition();
	if (strcmp(name, "read_only") == 0)
		return new ReadOnlyCondition(args);
	if (strcmp(name, "file_exists") == 0)
		return new FileExistsCondition(args);

	return NULL;
}


// #pragma mark -


Condition::Condition()
{
}


Condition::~Condition()
{
}


// #pragma mark -


ConditionContainer::ConditionContainer(const BMessage& args)
{
	char* name;
	type_code type;
	int32 count;
	for (int32 index = 0; args.GetInfo(B_MESSAGE_TYPE, index, &name, &type,
			&count) == B_OK; index++) {
		BMessage message;
		for (int32 messageIndex = 0; args.FindMessage(name, messageIndex,
				&message) == B_OK; messageIndex++) {
			AddCondition(create_condition(name, message));
		}
	}
}


void
ConditionContainer::AddCondition(Condition* condition)
{
	if (condition != NULL)
		fConditions.AddItem(condition);
}


// #pragma mark - and


AndCondition::AndCondition(const BMessage& args)
	:
	ConditionContainer(args)
{
}


bool
AndCondition::Test(ConditionContext& context) const
{
	for (int32 index = 0; index < fConditions.CountItems(); index++) {
		Condition* condition = fConditions.ItemAt(index);
		if (!condition->Test(context))
			return false;
	}
	return true;
}


// #pragma mark - or


OrCondition::OrCondition(const BMessage& args)
	:
	ConditionContainer(args)
{
}


bool
OrCondition::Test(ConditionContext& context) const
{
	if (fConditions.IsEmpty())
		return true;

	for (int32 index = 0; index < fConditions.CountItems(); index++) {
		Condition* condition = fConditions.ItemAt(index);
		if (condition->Test(context))
			return true;
	}
	return false;
}


// #pragma mark - or


NotCondition::NotCondition(const BMessage& args)
	:
	ConditionContainer(args)
{
}


bool
NotCondition::Test(ConditionContext& context) const
{
	for (int32 index = 0; index < fConditions.CountItems(); index++) {
		Condition* condition = fConditions.ItemAt(index);
		if (condition->Test(context))
			return false;
	}
	return true;
}


// #pragma mark - safemode


bool
SafeModeCondition::Test(ConditionContext& context) const
{
	return context.IsSafeMode();
}


// #pragma mark - read_only


ReadOnlyCondition::ReadOnlyCondition(const BMessage& args)
{
	fDevice = dev_for_path(args.GetString("args", "/boot"));
}


bool
ReadOnlyCondition::Test(ConditionContext& context) const
{
	BVolume volume;
	status_t status = volume.SetTo(fDevice);
	if (status != B_OK) {
		fprintf(stderr, "Failed to get BVolume for device %" B_PRIdDEV
			": %s\n", fDevice, strerror(status));
		return false;
	}

	BDiskDeviceRoster roster;
	BDiskDevice diskDevice;
	BPartition* partition;
	status = roster.FindPartitionByVolume(volume, &diskDevice, &partition);
	if (status != B_OK) {
		fprintf(stderr, "Failed to get partition for device %" B_PRIdDEV
			": %s\n", fDevice, strerror(status));
		return false;
	}

	return partition->IsReadOnly();
}


// #pragma mark - file_exists


FileExistsCondition::FileExistsCondition(const BMessage& args)
{
	for (int32 index = 0;
			const char* path = args.GetString("args", index, NULL); index++) {
		fPaths.Add(path);
	}
}


bool
FileExistsCondition::Test(ConditionContext& context) const
{
	for (int32 index = 0; index < fPaths.CountStrings(); index++) {
		BEntry entry;
		if (entry.SetTo(fPaths.StringAt(index)) != B_OK
			|| !entry.Exists())
			return false;
	}
	return true;
}


// #pragma mark -


/*static*/ Condition*
Conditions::FromMessage(const BMessage& message)
{
	return create_condition("and", message);
}
