/*
* Copyright 2010, Haiku. All rights reserved.
* Distributed under the terms of the MIT License.
*
* Authors:
*		Michael Pfeiffer
*/
#ifndef GP_BINDING_H
#define GP_BINDING_H


#include <Bitmap.h>
#include <Message.h>

#include <list>

#include "GPBand.h"
#include "GPJob.h"
#include "GPJobConfiguration.h"
#include "OutputStream.h"
#include "ValidRect.h"

class GPCapabilities;

class GPBinding
{
public:
				GPBinding();
				~GPBinding();

	status_t	GetPrinterManufacturers(BMessage& manufacturers);
	bool		ExtractManufacturer(const BMessage& manufacturers, int32 index,
		BString& id, BString& displayName);
	void		AddManufacturer(BMessage& manufacturers, const char* id,
		const char* name);

	status_t	GetPrinterModels(const char* manufacturer, BMessage& models);
	bool 		ExtractModel(const BMessage& models, int32 index,
		BString& displayName, BString& driver);
	void		AddModel(BMessage& models, const char* displayName,
		const char* driver);

	status_t	GetCapabilities(const char* driver,
					GPCapabilities* capabilities);

	status_t	BeginJob(GPJobConfiguration* configuration,
		OutputStream* outputStream) /* throw(TransportException) */;
	void		EndJob() /* throw(TransportException) */;
	void		BeginPage() /* throw(TransportException) */;
	void		EndPage() /* throw(TransportException) */;
	status_t	AddBitmapToPage(BBitmap* bitmap, BRect validRect, BPoint where);

private:
	void		InitGutenprint();
	void		DeleteBands();

	bool			fInitialized;
	OutputStream*	fOutputStream;
	list<GPBand*>	fBands;
	GPJob 			fJob;
};

#endif
