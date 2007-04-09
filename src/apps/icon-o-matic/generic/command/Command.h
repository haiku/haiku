/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef COMMAND_H
#define COMMAND_H

#include <SupportDefs.h>
#include <String.h>

class BString;

class Command {
 public:
								Command();
	virtual						~Command();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();
	virtual status_t			Redo();

	virtual void				GetName(BString& name);

	virtual	bool				UndoesPrevious(const Command* previous);
	virtual	bool				CombineWithNext(const Command* next);
	virtual	bool				CombineWithPrevious(const Command* previous);

 protected:
			const char*			_GetString(uint32 key,
										   const char* defaultString) const;

			bigtime_t			fTimeStamp;
};

#endif // COMMAND_H
