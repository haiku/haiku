// ShareAttrDir.h

#ifndef NET_FS_SHARE_ATTR_DIR_H
#define NET_FS_SHARE_ATTR_DIR_H

#include <fs_attr.h>

#include "ShareAttrDirIterator.h"
#include "SLList.h"

class AttrDirInfo;
class AttributeInfo;
class BNode;

// Attribute
class Attribute : public SLListLinkImpl<Attribute> {
								Attribute(const char* name,
									const attr_info& info, const void* data);
								~Attribute();
public:
	static	status_t			CreateAttribute(const char* name,
									const attr_info& info, const void* data,
									Attribute** attribute);
	static	void				DeleteAttribute(Attribute* attribute);

			const char*			GetName() const;
			void				GetInfo(attr_info* info) const;
			uint32				GetType() const;
			off_t				GetSize() const;
			const void*			GetData() const;

private:
			attr_info			fInfo;
			char				fDataAndName[1];
};

// ShareAttrDir
class ShareAttrDir {
public:
								ShareAttrDir();
								~ShareAttrDir();

			status_t			Init(const AttrDirInfo& dirInfo);
			status_t			Update(const AttrDirInfo& dirInfo,
									DoublyLinkedList<ShareAttrDirIterator>*
										iterators);

			void				SetRevision(int64 revision);
			int64				GetRevision() const;

			void				SetUpToDate(bool upToDate);
			bool				IsUpToDate() const;

			// These modifying methods are currently used internally only.
			// Init()/Update() should be sufficient.
			void				ClearAttrDir();

			status_t			AddAttribute(const char* name,
									const attr_info& info, const void* data);
			bool				RemoveAttribute(const char* name);
			void				RemoveAttribute(Attribute* attribute);

			Attribute*			GetAttribute(const char* name) const;
			Attribute*			GetFirstAttribute() const;
			Attribute*			GetNextAttribute(Attribute* attribute) const;

private:
			status_t			_GetAttributes(const AttrDirInfo& dirInfo,
									Attribute**& attributes, int32& count);

private:
			SLList<Attribute>	fAttributes;
// TODO: Rethink whether we rather want an array.
			int64				fRevision;
			bool				fUpToDate;	// to enforce reloading even if
											// a revision is cached
};

#endif	// NET_FS_SHARE_ATTR_DIR_H
