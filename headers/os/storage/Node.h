//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
/*!
	\file Node.h
	BNode and node_ref interface declarations.
*/

#ifndef __sk_node_h__
#define __sk_node_h__

#include <Statable.h>
#include <StorageDefs.Private.h>

#ifdef USE_OPENBEOS_NAMESPACE
namespace OpenBeOS {
#endif

//---------------------------------------------------------------
// node_ref
//---------------------------------------------------------------

//! Reference structure to a particular vnode on a particular device
/*! <b>node_ref</b> - A node reference.

	@author <a href="mailto:tylerdauwalder@users.sf.net">Tyler Dauwalder</a>
	@author Be Inc.
	@version 0.0.0
*/
struct node_ref {
	node_ref();	
	node_ref(const node_ref &ref);
	
	bool operator==(const node_ref &ref) const;
	bool operator!=(const node_ref &ref) const;
	node_ref& operator=(const node_ref &ref);
	
	dev_t device;
	ino_t node;
};


//---------------------------------------------------------------
// BNode
//---------------------------------------------------------------

//! A BNode represents a chunk of data in the filesystem.
/*! The BNode class provides an interface for manipulating the data and attributes
	belonging to filesystem entries. The BNode is unaware of the name that refers
	to it in the filesystem (i.e. its entry); a BNode is solely concerned with
	the entry's data and attributes.


	@author <a href='mailto:tylerdauwalder@users.sf.net'>Tyler Dauwalder</a>
	@version 0.0.0

*/
class BNode : public BStatable {
public:

	BNode();
	BNode(const entry_ref *ref);
	BNode(const BEntry *entry);
	BNode(const char *path);
	BNode(const BDirectory *dir, const char *path);
	BNode(const BNode &node);
	virtual ~BNode();

	status_t InitCheck() const;

	virtual status_t GetStat(struct stat *st) const;

	status_t SetTo(const entry_ref *ref);
	status_t SetTo(const BEntry *entry);
	status_t SetTo(const char *path);
	status_t SetTo(const BDirectory *dir, const char *path);
	void Unset();

	status_t Lock();
	status_t Unlock();

	status_t Sync();

	ssize_t WriteAttr(const char *name, type_code type, off_t offset,
		const void *buffer, size_t len);
	ssize_t ReadAttr(const char *name, type_code type, off_t offset,
		void *buffer, size_t len) const;
	status_t RemoveAttr(const char *name);
	status_t RenameAttr(const char *oldname, const char *newname);
	status_t GetAttrInfo(const char *name, struct attr_info *info) const;
	status_t GetNextAttrName(char *buffer);
	status_t RewindAttrs();
	status_t WriteAttrString(const char *name, const BString *data);
	status_t ReadAttrString(const char *name, BString *result) const;

	BNode& operator=(const BNode &node);
	bool operator==(const BNode &node) const;
	bool operator!=(const BNode &node) const;

	int Dup();  // This should be "const" but R5's is not... Ugggh.

private:
	friend class BFile;
	friend class BDirectory;
	friend class BSymLink;

	virtual void _ReservedNode1(); 	
	virtual void _ReservedNode2();
	virtual void _ReservedNode3();
	virtual void _ReservedNode4();
	virtual void _ReservedNode5();
	virtual void _ReservedNode6();

	uint32 rudeData[4];

private:
	status_t set_fd(StorageKit::FileDescriptor fd);
	virtual void close_fd();
	void set_status(status_t newStatus);

	virtual status_t set_stat(struct stat &st, uint32 what);

	StorageKit::FileDescriptor fFd;
	StorageKit::FileDescriptor fAttrFd;
	status_t fCStatus;

	status_t InitAttrDir();
};



#ifdef USE_OPENBEOS_NAMESPACE
};		// namespace OpenBeOS
#endif

#endif	// __sk_node_h__