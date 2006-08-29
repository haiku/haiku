#ifndef REVERSE_PATH_COMMAND_H
#define REVERSE_PATH_COMMAND_H

#include "PathCommand.h"

class ReversePathCommand : public PathCommand {
 public:
								ReversePathCommand(VectorPath* path);
	virtual						~ReversePathCommand();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);
};

#endif // REVERSE_PATH_COMMAND_H
