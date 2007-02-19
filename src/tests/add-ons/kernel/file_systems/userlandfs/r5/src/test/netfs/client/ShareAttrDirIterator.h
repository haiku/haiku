// ShareAttrDirIterator.h

#ifndef NET_FS_SHARE_ATTR_DIR_ITERATOR_H
#define NET_FS_SHARE_ATTR_DIR_ITERATOR_H

#include "DLList.h"

class Attribute;
class ShareAttrDir;

class ShareAttrDirIterator : public DLListLinkImpl<ShareAttrDirIterator> {
public:
								ShareAttrDirIterator();
								~ShareAttrDirIterator();

			void				SetAttrDir(ShareAttrDir* attrDir);

			void				SetCurrentAttribute(Attribute* attribute);
			Attribute*			GetCurrentAttribute() const;
			Attribute*			NextAttribute();
			void				Rewind();

private:
			ShareAttrDir*		fAttrDir;
			Attribute*			fCurrentAttribute;
};

#endif	// NET_FS_SHARE_ATTR_DIR_ITERATOR_H
