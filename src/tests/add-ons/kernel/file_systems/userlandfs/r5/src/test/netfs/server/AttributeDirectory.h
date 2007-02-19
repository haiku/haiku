// AttributeDirectory.h

#ifndef NET_FS_ATTRIBUTE_DIRECTORY_H
#define NET_FS_ATTRIBUTE_DIRECTORY_H

#include <fs_attr.h>

#include "SLList.h"

class BNode;

// attribute directory status
enum {
	ATTRIBUTE_DIRECTORY_NOT_LOADED,
	ATTRIBUTE_DIRECTORY_VALID,
	ATTRIBUTE_DIRECTORY_TOO_BIG,
};

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

// AttributeDirectory
class AttributeDirectory {
public:
								AttributeDirectory();
	virtual						~AttributeDirectory();

			uint32				GetAttrDirStatus() const;
			bool				IsAttrDirValid() const;

			status_t			LoadAttrDir();
			void				ClearAttrDir();

			status_t			AddAttribute(const char* name,
									const attr_info& info, const void* data);
			bool				RemoveAttribute(const char* name);
			void				RemoveAttribute(Attribute* attribute);
			status_t			UpdateAttribute(const char* name, bool* removed,
									attr_info* info, const void** data);
			Attribute*			GetAttribute(const char* name) const;
			Attribute*			GetFirstAttribute() const;
			Attribute*			GetNextAttribute(Attribute* attribute) const;

	virtual	status_t			OpenNode(BNode& node) = 0;

private:
			status_t			_LoadAttribute(BNode& node, const char* name,
									attr_info& info, void* data,
									bool& dataLoaded);

private:
			SLList<Attribute>	fAttributes;
			uint32				fStatus;
};

#endif	// NET_FS_ATTRIBUTE_DIRECTORY_H
