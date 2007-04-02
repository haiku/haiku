/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef PATH_COMMAND_H
#define PATH_COMMAND_H


#include "Command.h"


namespace BPrivate {
namespace Icon {
	class VectorPath;
}
}
using namespace BPrivate::Icon;

class PathCommand : public Command {
 public:
								PathCommand(VectorPath* path);
	virtual						~PathCommand();
	
	virtual	status_t			InitCheck();
	virtual void				GetName(BString& name);

 protected:
			void				_Select(const int32* indices, int32 count,
										bool extend = false) const;

			VectorPath*			fPath;
};

#endif // PATH_COMMAND_H
