/*
 * Copyright 2006-2007, 2011, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "SimpleFileSaver.h"

#include "Exporter.h"


SimpleFileSaver::SimpleFileSaver(Exporter* exporter,
								 const entry_ref& ref)
	: FileSaver(ref),
	  fExporter(exporter)
{
	fExporter->SetSelfDestroy(false);
}


SimpleFileSaver::~SimpleFileSaver()
{
	delete fExporter;
}


status_t
SimpleFileSaver::Save(Document* document)
{
	return fExporter->Export(document, fRef);
}


void
SimpleFileSaver::WaitForExportThread()
{
	fExporter->WaitForExportThread();
}
