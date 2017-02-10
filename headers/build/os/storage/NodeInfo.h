//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the MIT License.
//---------------------------------------------------------------------
/*!
	\file NodeInfo.h
	BNodeInfo interface declaration.
*/

#ifndef _NODE_INFO_H
#define _NODE_INFO_H

#ifndef _BE_BUILD_H
#include <BeBuild.h>
#endif
#include <SupportDefs.h>
#include <Mime.h>
#include <Message.h>
#include <File.h>
#include <Entry.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif // USE_OPENBEOS_NAMESPACE

class BBitmap;
class BResources;


//!	BNodeInfo provides file type information
/*!	BNodeInfo provides a nice wrapper to all sorts of usefull meta data. 
	Like it's mime type, the files icon and the application which will load
	the file.

	\see <a href="http://www.opensource.org/licenses/mit-license.html">MIT</a>
	\author <a href='mailto:bonefish@users.sf.net'>Ingo Weinhold</a>
	\author <a href="mailto:mrmlk@users.sf.net"> Michael Lloyd Lee </a>
	\author Be Inc
	\version 0
 */
class BNodeInfo {
public:

	BNodeInfo();

	BNodeInfo(BNode *node);
	virtual ~BNodeInfo();

	status_t SetTo(BNode *node);

	status_t InitCheck() const;

	virtual status_t GetType(char *type) const;
	virtual status_t SetType(const char *type);
	virtual status_t GetIcon(BBitmap *icon, icon_size k = B_LARGE_ICON) const;
	virtual status_t SetIcon(const BBitmap *icon, icon_size k = B_LARGE_ICON);

	status_t GetPreferredApp(char *signature,
								app_verb verb = B_OPEN) const;
	status_t SetPreferredApp(const char *signature,
								app_verb verb = B_OPEN);
	status_t GetAppHint(entry_ref *ref) const;
	status_t SetAppHint(const entry_ref *ref);

	status_t GetTrackerIcon(BBitmap *icon,
							icon_size k = B_LARGE_ICON) const;
	static status_t GetTrackerIcon(const entry_ref *ref,
									BBitmap *icon,
									icon_size k = B_LARGE_ICON);
private:
	friend class BAppFileInfo;
  
	virtual void _ReservedNodeInfo1(); //< FBC
	virtual void _ReservedNodeInfo2(); //< FBC
	virtual void _ReservedNodeInfo3(); //< FBC

	BNodeInfo &operator=(const BNodeInfo &);
	BNodeInfo(const BNodeInfo &);

	BNode *fNode; //< The Node in question
	uint32 _reserved[2]; //< FBC
	status_t fCStatus; //< The status to return from InitCheck
};

#ifdef USE_OPENBEOS_NAMESPACE
}
#endif // USE_OPENBEOS_NAMESPACE

#endif // _NODE_INFO_H


