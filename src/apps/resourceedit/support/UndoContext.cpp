/*
 * Copyright 2012-2013 Tri-Edge AI <triedgeai@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "UndoContext.h"

#include "Constants.h"

#include <List.h>
#include <String.h>


UndoContext::Action::Action(const BString& label)
{
	fLabel = label;
}


UndoContext::Action::~Action()
{

}


void
UndoContext::Action::Do()
{

}


void
UndoContext::Action::Undo()
{

}


void
UndoContext::Action::SetLabel(const BString& label)
{
	fLabel = label;
}


BString
UndoContext::Action::Label() const
{
	return fLabel;
}


UndoContext::UndoContext()
{
	fAt = 0;
	fLimit = 3;
	fHistory = new BList();
}


UndoContext::~UndoContext()
{
	for (int32 i = 0; i < fHistory->CountItems(); i++)
		delete (Action*)fHistory->ItemAt(i);

	delete fHistory;
}


void
UndoContext::SetLimit(int32 limit)
{
	int32 delta = fLimit - limit;

	if (limit > 1)
		fLimit = limit;
	else
		limit = 1;

	if (delta > 0) {
		for (int32 i = 0; i < delta; i++)
			delete (Action*)fHistory->ItemAt(i);

		fHistory->RemoveItems(0, delta);

		if (fAt > fLimit)
			fAt = fLimit;
	}
}


int32
UndoContext::Limit() const
{
	return fLimit;
}


void
UndoContext::Do(UndoContext::Action* action)
{
	int32 count = fHistory->CountItems();

	if (fAt >= fLimit)
		delete (Action*)fHistory->RemoveItem((int32)0);
	else {
		for (int32 i = fAt; i < count; i++)
			delete (Action*)fHistory->ItemAt(i);

		fHistory->RemoveItems(fAt, count - fAt);

		fAt++;
	}

	fHistory->AddItem(action);

	action->Do();
}


void
UndoContext::Undo()
{
	if (!CanUndo())
		return;

	fAt--;
	((Action*)fHistory->ItemAt(fAt))->Undo();
}


void
UndoContext::Redo()
{
	if (!CanRedo())
		return;

	((Action*)fHistory->ItemAt(fAt))->Do();
	fAt++;
}


bool
UndoContext::CanUndo() const
{
	return fAt > 0;
}


bool
UndoContext::CanRedo() const
{
	return fAt < fHistory->CountItems();
}


BString
UndoContext::UndoLabel() const
{
	if (CanUndo())
		return ((Action*)fHistory->ItemAt(fAt - 1))->Label();
	else
		return "";
}

BString
UndoContext::RedoLabel() const
{
	if (CanRedo())
		return ((Action*)fHistory->ItemAt(fAt))->Label();
	else
		return "";
}
