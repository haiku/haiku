//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Statable.h
	BStatable interface declaration.
*/

#ifndef __sk_statable_h__
#define __sk_statable_h__

#include <sys/types.h>
#include <sys/stat.h>
#include <SupportDefs.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif // USE_OPENBEOS_NAMESPACE
  
  struct node_ref;
  class BVolume;
  

	//! BStatable - A nice C++ wrapper to <code>\sa stat()</code>
	/*! A purly abstract class which provieds an expenive, but convenet 
	 * C++ wrapper to the posix <code>\sa stat()</code> command. 
	 *
	 * @see <a href="http://www.opensource.org/licenses/mit-license.html">MIT</a>
	 * @author <a href="mailto:mrmlk@users.sf.net"> Michael Lloyd Lee </a>
	 * @author Be Inc
	 * @version 0
	 */
class BStatable {
public:
	virtual status_t GetStat(struct stat *st) const = 0;

	bool IsFile() const;
	bool IsDirectory() const;
	bool IsSymLink() const;

	status_t GetNodeRef(node_ref *ref) const;

	status_t GetOwner(uid_t *owner) const;
	status_t SetOwner(uid_t owner);

	status_t GetGroup(gid_t *group) const;
	status_t SetGroup(gid_t group);

	status_t GetPermissions(mode_t *perms) const;
	status_t SetPermissions(mode_t perms);

	status_t GetSize(off_t *size) const; 

	status_t GetModificationTime(time_t *mtime) const;
	status_t SetModificationTime(time_t mtime);

	status_t GetCreationTime(time_t *ctime) const;
	status_t SetCreationTime(time_t ctime);

	status_t GetAccessTime(time_t *atime) const;
	status_t SetAccessTime(time_t atime);

	status_t GetVolume(BVolume *vol) const;

  private:

	friend class BEntry;
	friend class BNode;

	virtual	void _OhSoStatable1(); 	//< FBC
	virtual	void _OhSoStatable2(); 	//< FBC
	virtual	void _OhSoStatable3(); 	//< FBC
	uint32 _ohSoData[4]; 			//< FBC

	virtual	status_t set_stat(struct stat &st, uint32 what) = 0;
};

#ifdef USE_OPENBEOS_NAMESPACE
}
#endif // USE_OPENBEOS_NAMESPACE

#endif // __sk_statable_h__
