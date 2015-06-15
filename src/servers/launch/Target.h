/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TARGET_H
#define TARGET_H


#include <Job.h>
#include <Message.h>


using namespace BSupportKit;


class Target : public BJob {
public:
								Target(const char* name);

			const char*			Name() const;

			status_t			AddData(const char* name, BMessage& data);
			const BMessage&		Data() const
									{ return fData; }

protected:
	virtual	status_t			Execute();

private:
			BMessage			fData;
};


#endif // TARGET_H
