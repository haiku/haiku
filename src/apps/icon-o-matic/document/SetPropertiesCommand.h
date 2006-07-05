/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef SET_PROPERTIES_COMMAND_H
#define SET_PROPERTIES_COMMAND_H

#include "Command.h"

class IconObject;
class PropertyObject;

class SetPropertiesCommand : public Command {
 public:
								SetPropertiesCommand(IconObject** objects,
											 	  	 int32 objectCount,
											 	  	 PropertyObject* previous,
											 	  	 PropertyObject* current);
	virtual						~SetPropertiesCommand();

	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
	IconObject**				fObjects;
	int32						fObjectCount;

	PropertyObject*				fOldProperties;
	PropertyObject*				fNewProperties;
};

#endif // SET_PROPERTIES_COMMAND_H
