/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TARGET_H
#define TARGET_H


#include "BaseJob.h"

#include <Message.h>


using namespace BSupportKit;


class Target : public BaseJob {
public:
								Target(const char* name);

			status_t			AddData(const char* name, BMessage& data);
			const BMessage&		Data() const
									{ return fData; }

			bool				HasLaunched() const
									{ return fLaunched; }
			void				SetLaunched(bool launched);

protected:
	virtual	status_t			Execute();

private:
			BMessage			fData;
			bool				fLaunched;
};


#endif // TARGET_H
