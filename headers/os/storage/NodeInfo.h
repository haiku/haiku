/*
 * Copyright 2002-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _NODE_INFO_H
#define _NODE_INFO_H


#include <BeBuild.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <Mime.h>
#include <SupportDefs.h>


class BBitmap;
class BResources;


/*!	\brief BNodeInfo provides file type information
	BNodeInfo provides a nice wrapper to all sorts of usefull meta data. 
	Like it's mime type, the files icon and the application which will load
	the file.
*/
class BNodeInfo {
public:
								BNodeInfo();
								BNodeInfo(BNode* node);
	virtual						~BNodeInfo();

			status_t			SetTo(BNode* node);

			status_t			InitCheck() const;

	virtual status_t			GetType(char* type) const;
	virtual status_t			SetType(const char* type);
	virtual status_t			GetIcon(BBitmap* icon,
									icon_size size = B_LARGE_ICON) const;
	virtual status_t			SetIcon(const BBitmap* icon,
									icon_size size = B_LARGE_ICON);
			status_t			GetIcon(uint8** data, size_t* size,
									type_code* type) const;
			status_t			SetIcon(const uint8* data, size_t size);

			status_t			GetPreferredApp(char* signature,
									app_verb verb = B_OPEN) const;
			status_t			SetPreferredApp(const char* signature,
									app_verb verb = B_OPEN);
			status_t			GetAppHint(entry_ref* ref) const;
			status_t			SetAppHint(const entry_ref* ref);

			status_t			GetTrackerIcon(BBitmap* icon,
									icon_size size = B_LARGE_ICON) const;
	static	status_t			GetTrackerIcon(const entry_ref* ref,
									BBitmap* icon,
									icon_size size = B_LARGE_ICON);
private:
			friend class BAppFileInfo;
  
	virtual void				_ReservedNodeInfo1();
	virtual void				_ReservedNodeInfo2();
	virtual void				_ReservedNodeInfo3();

								BNodeInfo &operator=(const BNodeInfo& other);
								BNodeInfo(const BNodeInfo& other);
									// not implemented

private:
			BNode*				fNode;
			uint32				_reserved[2];
			status_t			fCStatus;
};


#endif // _NODE_INFO_H
