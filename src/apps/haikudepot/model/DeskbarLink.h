/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2011, Ingo Weinhold, <ingo_weinhold@gmx.de>
 * Copyright 2013, Rene Gollent, <rene@gollent.com>
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>.
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Note that this file has been re-factored from `PackageManager.h` and
 * copyrights have been carried across in 2021.
 */
#ifndef DESKBAR_LINK_H
#define DESKBAR_LINK_H


#include <Archivable.h>
#include <String.h>


class DeskbarLink : public BArchivable {
public:
								DeskbarLink();
								DeskbarLink(const BString& path,
									const BString& link);
								DeskbarLink(const DeskbarLink& other);
								DeskbarLink(BMessage* from);

	virtual						~DeskbarLink();

	const	BString				Path() const;
	const	BString				Link() const;

			bool				operator==(const DeskbarLink& other);
			bool				operator!=(const DeskbarLink& other);
			DeskbarLink&		operator=(const DeskbarLink& other);

			status_t			Archive(BMessage* into, bool deep = true) const;

private:
			BString				fPath;
			BString				fLink;
};


#endif // DESKBAR_LINK_H
