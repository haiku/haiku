/*
 * Copyright 2006, 2007, 2011, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef EXPORTER_H
#define EXPORTER_H


#include <string.h>

#include <Entry.h>
#include <OS.h>

#include "IconBuild.h"


class BPositionIO;
class Document;


_BEGIN_ICON_NAMESPACE
	class Icon;
_END_ICON_NAMESPACE

_USING_ICON_NAMESPACE

/*! Base class for all exporters to implement. */
class Exporter {
 public:
								Exporter();
	virtual						~Exporter();

	/*! Export the Document to the specified entry_ref.
		Spawns a separate thread to do the actual exporting.
		Uses the virtual Export function defined after this function to do the
		actual export.
	*/
			status_t			Export(Document* document,
									   const entry_ref& ref);

	virtual	status_t			Export(const Icon* icon,
									   BPositionIO* stream) = 0;
	/*! Turns the status_t error code returned by \c Export into a string. */
	virtual const char*			ErrorCodeToString(status_t code)
									{ return strerror(code); }
	virtual	const char*			MIMEType() = 0;

	/*! If \a selfDestroy is true, class deletes itself when export thread is
		finished.
	*/
			void				SetSelfDestroy(bool selfDestroy);

			void				WaitForExportThread();

 private:
	static	int32				_ExportThreadEntry(void* cookie);
			int32				_ExportThread();
			status_t			_Export(const Icon* icon,
										const entry_ref* docRef);

			Document*			fDocument;
			Icon*				fClonedIcon;
			entry_ref			fRef;
			thread_id			fExportThread;
			bool				fSelfDestroy;
};

#endif // EXPORTER_H
