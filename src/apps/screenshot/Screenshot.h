/*
 * Copyright Karsten Heimrich, host.haiku@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <Application.h>


class Screenshot : public BApplication {
public:
						Screenshot();
	virtual				~Screenshot();

	virtual	void		ReadyToRun();
	virtual	void		RefsReceived(BMessage* message);
	virtual	void		ArgvReceived(int32 argc, char** argv);

private:
			void		_ShowHelp() const;

private:
	bool				fArgvReceived;
};
