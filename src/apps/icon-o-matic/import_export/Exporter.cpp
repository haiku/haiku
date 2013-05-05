/*
 * Copyright 2006, 2011, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "Exporter.h"

#include <fs_attr.h>
#include <new>
#include <stdio.h>

#include <Alert.h>
#include <Catalog.h>
#include <File.h>
#include <Locale.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>

#include "CommandStack.h"
#include "Document.h"
#include "Icon.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-Exporter"


using std::nothrow;


Exporter::Exporter()
	: fDocument(NULL),
	  fClonedIcon(NULL),
	  fRef(),
	  fExportThread(-1),
	  fSelfDestroy(false)
{
}


Exporter::~Exporter()
{
	WaitForExportThread();

	delete fClonedIcon;
}


status_t
Exporter::Export(Document* document, const entry_ref& ref)
{
	if (!document || ref.name == NULL)
		return B_BAD_VALUE;

	fDocument = document;
	fClonedIcon = fDocument->Icon()->Clone();
	if (!fClonedIcon)
		return B_NO_MEMORY;

	fRef = ref;

	fExportThread = spawn_thread(_ExportThreadEntry, "export",
								 B_NORMAL_PRIORITY, this);
	if (fExportThread < 0)
		return (status_t)fExportThread;

	resume_thread(fExportThread);

	return B_OK;
}


void
Exporter::SetSelfDestroy(bool selfDestroy)
{
	fSelfDestroy = selfDestroy;
}


void
Exporter::WaitForExportThread()
{
	if (fExportThread >= 0 && find_thread(NULL) != fExportThread) {
		status_t ret;
		wait_for_thread(fExportThread, &ret);
		fExportThread = -1;
	}
}


// #pragma mark -


int32
Exporter::_ExportThreadEntry(void* cookie)
{
	Exporter* exporter = (Exporter*)cookie;
	return exporter->_ExportThread();
}


int32
Exporter::_ExportThread()
{
	status_t ret = _Export(fClonedIcon, &fRef);
	if (ret < B_OK) {
		// inform user of failure at this point
		BString helper(B_TRANSLATE("Saving your document failed!"));
		helper << "\n\n" << B_TRANSLATE("Error: ") << strerror(ret);
		BAlert* alert = new BAlert(B_TRANSLATE("bad news"), helper.String(),
			B_TRANSLATE_CONTEXT("Bleep!", 
				"Exporter - Continue in error dialog"), 
			NULL, NULL);
		// launch alert asynchronously
		alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
		alert->Go(NULL);
	} else {
		// success
	
		// add to recent document list
		be_roster->AddToRecentDocuments(&fRef);
		// mark command stack state as saved,
		fDocument->CommandStack()->Save();
			// NOTE: CommandStack is thread safe
	
		if (fDocument->WriteLock()) {
			// set ref and name of document
//			fDocument->SetRef(fRef);
			fDocument->SetName(fRef.name);

			fDocument->WriteUnlock();
		}
	}

	if (fSelfDestroy)
		delete this;

	return ret;
}


status_t
Exporter::_Export(const Icon* icon, const entry_ref* docRef)
{
	// TODO: reenable the commented out code, but make it work
	// the opposite direction, ie *copy* the file contents

	BEntry entry(docRef, true);
	if (entry.IsDirectory())
		return B_BAD_VALUE;

	const entry_ref* ref = docRef;
//	entry_ref tempRef;
//
//	if (entry.Exists()) {
//		// if the file exists create a temporary file in the same folder
//		// and hope that it doesn't already exist...
//		BPath tempPath(docRef);
//		if (tempPath.GetParent(&tempPath) >= B_OK) {
//			BString helper(docRef->name);
//			helper << system_time();
//			if (tempPath.Append(helper.String()) >= B_OK
//				&& entry.SetTo(tempPath.Path()) >= B_OK
//				&& entry.GetRef(&tempRef) >= B_OK) {
//				// have the output ref point to the temporary
//				// file instead
//				ref = &tempRef;
//			}
//		}
//	}

	status_t ret = B_BAD_VALUE;

	// do the actual save operation into a file
	BFile outFile(ref, B_CREATE_FILE | B_READ_WRITE | B_ERASE_FILE);
	ret = outFile.InitCheck();
	if (ret == B_OK) {
		try {
			// export using the virtual Export() version
			ret = Export(icon, &outFile);
		} catch (...) {
			printf("Exporter::_Export() - "
				   "unkown exception occured!\n");
			ret = B_ERROR;
		}
		if (ret < B_OK) {
			printf("Exporter::_Export() - "
				   "failed to export icon: %s\n", strerror(ret));
		}
	} else {
		printf("Exporter::_Export() - "
			   "failed to create output file: %s\n", strerror(ret));
	}
	outFile.Unset();

//	if (ret < B_OK && ref != docRef) {
//		// in case of failure, remove temporary file
//		entry.Remove();
//	}
//
//	if (ret >= B_OK && ref != docRef) {
//		// move temp file overwriting actual document file
//		BEntry docEntry(docRef, true);
//		// copy attributes of previous document file
//		BNode sourceNode(&docEntry);
//		BNode destNode(&entry);
//		if (sourceNode.InitCheck() >= B_OK && destNode.InitCheck() >= B_OK) {
//			// lock the nodes
//			if (sourceNode.Lock() >= B_OK) {
//				if (destNode.Lock() >= B_OK) {
//					// iterate over the attributes
//					char attrName[B_ATTR_NAME_LENGTH];
//					while (sourceNode.GetNextAttrName(attrName) >= B_OK) {
////						// skip the icon, since we probably wrote that
////						// before
////						if (strcmp(attrName, "BEOS:ICON") == 0)
////							continue;
//						attr_info info;
//						if (sourceNode.GetAttrInfo(attrName, &info) >= B_OK) {
//							char *buffer = new (nothrow) char[info.size];
//							if (buffer && sourceNode.ReadAttr(attrName, info.type, 0,
//															  buffer, info.size) == info.size) {
//								destNode.WriteAttr(attrName, info.type, 0,
//												   buffer, info.size);
//							}
//							delete[] buffer;
//						}
//					}
//					destNode.Unlock();
//				}
//				sourceNode.Unlock();
//			}
//		}
//		// clobber the orginal file with the new temporary one
//		ret = entry.Rename(docRef->name, true);
//	}

	if (ret >= B_OK && MIMEType()) {
		// set file type
		BNode node(docRef);
		if (node.InitCheck() == B_OK) {
			BNodeInfo nodeInfo(&node);
			if (nodeInfo.InitCheck() == B_OK)
				nodeInfo.SetType(MIMEType());
		}
	}
	return ret;
}
