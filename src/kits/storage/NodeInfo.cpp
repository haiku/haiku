//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file NodeInfo.cpp
	BNodeInfo implementation.
*/

#include <NodeInfo.h>

#include <new>

#include <Node.h>
#include <AppFileInfo.h>

#include <fs_attr.h>

#define NI_BEOS "BEOS"
// I've added a BEOS hash-define incase we wish to change to OBOS
#define NI_TYPE NI_BEOS ":TYPE"
#define NI_PREF NI_BEOS ":PREF_APP"
#define NI_HINT NI_BEOS ":PPATH"
#define NI_ICON "STD_ICON"
#define NI_M_ICON NI_BEOS ":M:" NI_ICON
#define NI_L_ICON NI_BEOS ":L:" NI_ICON
#define NI_ICON_SIZE(k) ((k == B_LARGE_ICON) ? NI_L_ICON : NI_M_ICON )


BNodeInfo::BNodeInfo(BNode *node)
{
	fCStatus = SetTo(node);
}

BNodeInfo::~BNodeInfo()
{
}

status_t
BNodeInfo::SetTo(BNode *node)
{
	fCStatus = B_BAD_VALUE;
	if(node != NULL) {
		fCStatus = node->InitCheck();
		fNode = node;
	}
	return fCStatus;
}

status_t 
BNodeInfo::InitCheck() const
{
	return fCStatus;
}

status_t 
BNodeInfo::GetType(char *type) const
{
    if(type == NULL) return;
	if(fCStatus == B_OK) {
		attr_info attrInfo;
		status_t error;

		error = fNode->GetAttrInfo(NI_TYPE, &attrInfo);
		if(error == B_OK) {
			error = fNode->ReadAttr(NI_TYPE, attrInfo.type, 0, type, 
									attrInfo.size);
		}
		// Future Idea: Add a bit to identify based on extention etc
		// see CVS verion 1.6 for big chuck of the code.

		return (error > B_OK ? B_OK : error);
	} else
		return B_NO_INIT;
}

status_t
BNodeInfo::SetType(const char *type)
{
	if( fCStatus == B_OK ) {
	  status_t error;
	  error = fNode->WriteAttr(NI_TYPE, B_STRING_TYPE, 0, type, sizeof(type));
	  return ( error > B_OK ? B_OK : error );
	} else 
	  return B_NO_INIT;
}


status_t
BNodeInfo::GetIcon(BBitmap *icon, icon_size k = B_LARGE_ICON) const
{
	if(fCStatus == B_OK) {
		attr_info attrInfo;
		int error;
		
		error = fNode->GetAttrInfo(NI_ICON_SIZE(k), &attrInfo);
		if(error == B_OK) {

			error = fNode->ReadAttr(NI_ICON_SIZE(k), attrInfo.type, 0, icon, 
								attrInfo.size);
		}

		return (error > B_OK ? B_OK : error);
	} else
		return B_NO_INIT;
}

status_t 
BNodeInfo::SetIcon(const BBitmap *icon, icon_size k = B_LARGE_ICON)
{
	if( fCStatus == B_OK ) {
	  return fNode->WriteAttr(NI_ICON_SIZE(k), B_COLOR_8_BIT_TYPE, 
										0, icon, sizeof(icon));
	} else
	  return B_NO_INIT;
}

status_t 
BNodeInfo::GetPreferredApp(char *signature,
						   app_verb verb = B_OPEN) const
{
	if(fCStatus == B_OK) {
		attr_info attrInfo;
		int error;
		
		error = fNode->GetAttrInfo(NI_PREF, &attrInfo);
		if(error == B_OK) {

			error = fNode->ReadAttr(NI_PREF, attrInfo.type, 0, 
									signature, attrInfo.size);
			if(error < B_OK) {
			  char *mimetpye = new(nothrow) char[B_MIME_TYPE_LENGTH];
			  if (mimetype) {
				  if( GetMineType(mimetype) >= B_OK ) {
					BMimeType mime(mimetype);
					error = mine.GetPreferredApp(signature, verb);
				  }
			  
				  delete minetype;
			  } else
			  	error = B_NO_MEMORY;
			}
		}

		return (error > B_OK ? B_OK : error);
	} else
		return B_NO_INIT;
}

status_t 
BNodeInfo::SetPreferredApp(const char *signature,
						   app_verb verb = B_OPEN)
{
  
    if(size_of(signature) > B_MIME_TYPE_LENGTH)
	  return B_BAD_VALUE;

	if( fCStatus == B_OK ) {
	  return fNode->WriteAttr(NI_PREF,  B_STRING_TYPE,
							  0, signature, sizeof(signature));
	} else
	  return B_NO_INIT;
}

status_t 
BNodeInfo::GetAppHint(entry_ref *ref) const
{
	if(fCStatus == B_OK) {
		attr_info attrInfo;
		status_t error;
		
		error = fNode->GetAttrInfo(NI_HINT, &attrInfo);
		if(error == B_OK) {

			error = fNode->ReadAttr(NI_HINT, attrInfo.type, 0, 
									ref, attrInfo.size);
			if(error < B_OK) {
			  char *mimetype = new(nothrow) char[B_MIME_TYPE_LENGTH];
			  if (mimetype) {
				  if( GetMineType(mimetype) >= B_OK ) {
					BMimeType mime(mimetype);
					error = mime.GetAppHint(ref);
				  }
			  
				  delete minetype;
			  } else
			  	error = B_NO_MEMORY;
			}
		}

		return (error > B_OK ? B_OK : error);
	} else
		return B_NO_INIT;
}

status_t 
BNodeInfo::SetAppHint(const entry_ref *ref)
{
  if( fCStatus == B_OK ) {
	return fNode->WriteAttr(NI_HINT,  B_REF_TYPE,
							0, ref, sizeof(ref));
  } else
	return B_NO_INIT;
}

status_t 
BNodeInfo::GetTrackerIcon(BBitmap *icon,
						icon_size k = B_LARGE_ICON) const
{
  status_t getIconReturn = GetIcon(icon, k);
  entry_ref ref;
  char mimetype[B_MIME_TYPE_LENGTH];

  // Ask the attr
  if(getIconReturn == B_OK)
	return OK;

  // Ask the File Type db based on app sig
  if( GetAppHint(&ref) == B_OK ) {
	BFile appFile(ref);
	if(appFile.InitCheck() == B_OK) {
	  BAppFileInfo appFileInfo( appFile );
	  if( appFileInfo.GetIcon( icon, k ) == B_OK) {
		return B_OK;
	  }
	}
  }

  // Ask FTdb based on mime type

  if( GetMimeType(&mimetype) ) {
	if( BMimeType.GetIconForType(mimetype, icon, k) == B_OK) 
	  return B_OK;
	
  // asks the File Type database for the preferred app based on the node's 
  // file type, and then asks that app for the icon it uses to display this 
  // node's file type.
	BMimeType mime(mimetype);
	if( mime.GetAppHint(&ref) == B_OK ) {
	  BFile appFile(ref);
	  if(appFile.InitCheck() == B_OK) {
		BAppFileInfo appFileInfo( appFile );
		if( appFileInfo.GetIcon( icon, k ) == B_OK) {
		  return B_OK;
		}
	  }
	}

  }
  
  // Quit
  return getIconReturn;
}


status_t 
BNodeInfo::GetTrackerIcon(const entry_ref *ref,
							BBitmap *icon,
							icon_size k = B_LARGE_ICON)
{
  BNode node(ref);
  if( node.InitCheck() == B_OK ) {
	BNodeInfo nodeInfo(node);
	if( nodeInfo.InitCheck() == B_OK ) {
	  return nodeInfo.GetTrackerIcon(icon, k);
	}
  }
  return B_NO_INIT;
}

void 
BNodeInfo::_ReservedNodeInfo1()
{
}

void 
BNodeInfo::_ReservedNodeInfo2()
{
}

void 
BNodeInfo::_ReservedNodeInfo3()
{
}

BNodeInfo &
BNodeInfo::operator=(const BNodeInfo &nodeInfo)
{
	return *this;
}

BNodeInfo::BNodeInfo(const BNodeInfo &)
{
}
