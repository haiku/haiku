/*
 * Copyright Karsten Heimrich, host.haiku@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Karsten Heimrich
 *		Fredrik Modéen
 */
#ifndef SCREENSHOT_H
#define SCREENSHOT_H


#include <Application.h>
#include <Catalog.h>


class Screenshot : public BApplication {
public:
						Screenshot();
	virtual				~Screenshot();

	virtual	void		ReadyToRun();
	virtual	void		RefsReceived(BMessage* message);
	virtual	void		ArgvReceived(int32 argc, char** argv);

private:
			void		_ShowHelp() const;
			void		_SetImageTypeSilence(const char* name);

	bool				fArgvReceived;
	bool				fRefsReceived;
	int32				fImageFileType;
	BCatalog				fCatalog;
};

#endif	/* SCREENSHOT_H */
