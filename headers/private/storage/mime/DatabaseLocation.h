/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _MIME_DATABASE_LOCATION_H
#define _MIME_DATABASE_LOCATION_H


#include <Mime.h>
#include <StringList.h>


namespace BPrivate {
namespace Storage {
namespace Mime {


class DatabaseLocation {
public:
								DatabaseLocation();
								~DatabaseLocation();

			bool				AddDirectory(const BString& directory);

			// paths

			const BStringList&	Directories() const
									{ return fDirectories; }
			BString				WritableDirectory() const
									{ return fDirectories.StringAt(0); }

			BString				WritablePathForType(const char* type) const
									{ return _TypeToFilename(type, 0); }

			// opening type nodes

			status_t			OpenType(const char* type, BNode& _node) const;
			status_t			OpenWritableType(const char* type,
									BNode& _node, bool create,
									bool* _didCreate = NULL) const;

			// generic type attributes access

			ssize_t				ReadAttribute(const char* type,
									const char* attribute, void* data,
									size_t length, type_code datatype) const;
			status_t			ReadMessageAttribute(const char* type,
									const char* attribute, BMessage& _message)
									const;
			status_t			ReadStringAttribute(const char* type,
									const char* attribute, BString& _string)
									const;

			status_t			WriteAttribute(const char* type,
									const char* attribute, const void* data,
									size_t length, type_code datatype,
									bool* _didCreate) const;
			status_t			WriteMessageAttribute(const char* type,
									const char* attribute,
									const BMessage& message, bool* _didCreate)
									const;

			status_t			DeleteAttribute(const char* type,
									const char* attribute) const;

			// type attribute convenience getters

			status_t			GetAppHint(const char* type, entry_ref& _ref);
			status_t			GetAttributesInfo(const char* type,
									BMessage& _info);
			status_t			GetShortDescription(const char* type,
									char* description);
			status_t			GetLongDescription(const char* type,
									char* description);
			status_t			GetFileExtensions(const char* type,
									BMessage& _extensions);
			status_t			GetIcon(const char* type, BBitmap& _icon,
									icon_size size);
			status_t			GetIcon(const char* type, uint8*& _data,
									size_t& _size);
			status_t			GetIconForType(const char* type,
									const char* fileType, BBitmap& _icon,
									icon_size which);
			status_t			GetIconForType(const char* type,
									const char* fileType, uint8*& _data,
									size_t& _size);
			status_t			GetPreferredApp(const char* type,
									char* signature, app_verb verb);
			status_t			GetSnifferRule(const char* type,
									BString& _result);
			status_t			GetSupportedTypes(const char* type,
									BMessage& _types);

			bool				IsInstalled(const char* type);

private:
			BString				_TypeToFilename(const char* type, int32 index)
									const;
			status_t			_OpenType(const char* type, BNode& _node,
									int32& _index) const;
			status_t			_CreateTypeNode(const char* type, BNode& _node)
									const;
			status_t			_CopyTypeNode(BNode& source, const char* type,
									BNode& _target) const;

private:
			BStringList			fDirectories;
};


} // namespace Mime
} // namespace Storage
} // namespace BPrivate


#endif	// _MIME_DATABASE_LOCATION_H
