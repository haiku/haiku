// VirtualDir.h

#ifndef NET_FS_VIRTUAL_DIR_H
#define NET_FS_VIRTUAL_DIR_H

#include <HashMap.h>
#include <HashString.h>
#include <util/DoublyLinkedList.h>

#include "Node.h"

class VirtualDir;
class VirtualDirEntry;

// VirtualDirIterator
class VirtualDirIterator : public DoublyLinkedListLinkImpl<VirtualDirIterator> {
public:
								VirtualDirIterator();
								~VirtualDirIterator();

			void				SetDirectory(VirtualDir* directory,
									bool onlyChildren = false);

			bool				GetCurrentEntry(const char** name, Node** node);
			VirtualDirEntry*	GetCurrentEntry() const;
									// for use by VirtualDir only
			void				NextEntry();
			void				Rewind();

private:
			enum {
				STATE_DOT		= 0,
				STATE_DOT_DOT	= 1,
				STATE_OTHERS	= 2,
			};

			VirtualDir*			fDirectory;
			VirtualDirEntry*	fCurrentEntry;
			int					fState;
};

// VirtualDirEntry
class VirtualDirEntry : public DoublyLinkedListLinkImpl<VirtualDirEntry> {
public:
								VirtualDirEntry();
								~VirtualDirEntry();

			status_t			SetTo(const char* name, Node* node);

			const char*			GetName() const;
			Node*				GetNode() const;

private:
			HashString			fName;
			Node*				fNode;
};

// VirtualDir
class VirtualDir : public Node, public DoublyLinkedListLinkImpl<VirtualDir> {
public:
								VirtualDir(Volume* volume, vnode_id nodeID);
	virtual						~VirtualDir();

			status_t			InitCheck() const;

			void				SetParent(VirtualDir* parent);
			VirtualDir*			GetParent() const;
				// TODO: Remove? We don't need it, do we?

			time_t				GetCreationTime() const;

			status_t			AddEntry(const char* name, Node* child);
			Node*				RemoveEntry(const char* name);
			VirtualDirEntry*	GetEntry(const char* name) const;
			Node*				GetChildNode(const char* name) const;
			VirtualDirEntry*	GetFirstEntry() const;
			VirtualDirEntry*	GetNextEntry(VirtualDirEntry* entry) const;

			void				AddDirIterator(VirtualDirIterator* iterator);
			void				RemoveDirIterator(VirtualDirIterator* iterator);

private:
			typedef HashMap<HashString, VirtualDirEntry*> EntryMap;

			VirtualDir*			fParent;
			time_t				fCreationTime;
			EntryMap			fEntries;
			DoublyLinkedList<VirtualDirEntry> fEntryList;
			DoublyLinkedList<VirtualDirIterator> fIterators;
};

#endif	// NET_FS_VIRTUAL_DIR_H
