/*
 * Copyright 2001-2010, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
#ifndef _SPOOL_FOLDER_H
#define _SPOOL_FOLDER_H


#include "Jobs.h"


class PrintersWindow;
class PrinterItem;


class SpoolFolder : public Folder {
public:
								SpoolFolder(PrintersWindow* window, 
									PrinterItem* item, 
									const BDirectory& spoolDir);
			PrinterItem* 		Item() const { return fItem; }

protected:
			void				Notify(Job* job, int kind);

			PrintersWindow* 	fWindow;
			PrinterItem* 		fItem;
};


#endif // _SPOOL_FOLDER_H
