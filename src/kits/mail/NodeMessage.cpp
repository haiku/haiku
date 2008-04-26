#include "NodeMessage.h"
#include <StorageKit.h> 
#include <fs_attr.h>
#include <stdlib.h>

/*
   These functions gives a nice BMessage interface to node attributes,
   by letting you transfer attributes to and from BMessages.  It makes
   it so you can use all the convenient Find...() and Add...() functions
   provided by BMessage for attributes too.  You use it as follows:
   
   BMessage m;
   BNode n(path);
   if (reading) { n>>m; printf("woohoo=%s\n",m.FindString("woohoo")) }
   else { m.AddString("woohoo","it's howdy doody time"); n<<m; }
   
   If there is more than one data item with a given name, the first
   item is the one writen to the node.
*/
_EXPORT BNode& operator<<(BNode& n, const BMessage& m)
{
	#if defined(HAIKU_TARGET_PLATFORM_DANO)
	const
	#endif
	char *name;
	type_code   type;
	ssize_t     bytes;
	const void *data;
	
	for (int32 i = 0;
		m.GetInfo(B_ANY_TYPE, i, &name, &type) == 0;
		i++) {
		m.FindData (name,type,0,&data,&bytes);
		n.WriteAttr(name,type,0, data, bytes);
	}
	
	return n;
}

_EXPORT BNode& operator>>(BNode& n, BMessage& m)
{
	char        name[B_ATTR_NAME_LENGTH];
	attr_info	info;
	char *buf = NULL;
	
	n.RewindAttrs();
	while (n.GetNextAttrName(name) == B_OK) {
		if (n.GetAttrInfo(name,&info) != B_OK)
			continue;

		// resize the buffer
		if (char *newBuffer = (char*)realloc(buf, info.size))
			buf = newBuffer;
		else
			continue;

		info.size=n.ReadAttr(name,info.type,0,buf,info.size);
		if (info.size >= 0)
			m.AddData(name,info.type,buf,info.size);
	}
	n.RewindAttrs();

	free(buf);

	return n;
}
