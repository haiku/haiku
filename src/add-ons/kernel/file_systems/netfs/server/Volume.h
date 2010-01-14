// Volume.h

#ifndef NET_FS_VOLUME_H
#define NET_FS_VOLUME_H

#include <SupportDefs.h>

class Directory;
class Entry;
class Node;

class Volume {
public:
								Volume(dev_t id);
								~Volume();

			status_t			Init();

			dev_t				GetID() const;
			Directory*			GetRootDirectory() const;
			ino_t				GetRootID() const;

			bool				KnowsQuery() const;

			status_t			AddNode(Node* node);
			bool				RemoveNode(Node* node);
			Node*				GetNode(ino_t nodeID);
			Node*				GetFirstNode() const;

			status_t			AddEntry(Entry* entry);
			bool				RemoveEntry(Entry* entry);
			Entry*				GetEntry(ino_t dirID, const char* name);
			Entry*				GetFirstEntry() const;

private:
			struct NodeMap;
			struct EntryKey;
			struct EntryMap;

			dev_t				fID;
			Directory*			fRootDir;
			uint32				fFSFlags;
			NodeMap*			fNodes;
			EntryMap*			fEntries;
};

#endif	// NET_FS_VOLUME_H
