/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef BOOT_LOADER_FILE_SYSTEMS_PACKAGEFS_PACKAGE_SETTINGS_ITEM_H
#define BOOT_LOADER_FILE_SYSTEMS_PACKAGEFS_PACKAGE_SETTINGS_ITEM_H


#include <string.h>

#include <util/OpenHashTable.h>
#include <util/StringHash.h>


struct driver_parameter;
class Directory;


namespace PackageFS {


class PackageSettingsItem {
public:
			class Entry {
			public:
				Entry(Entry* parent)
					:
					fParent(parent),
					fName(NULL),
					fIsBlackListed(false),
					fHashNext(NULL)
				{
				}

				~Entry()
				{
					free(fName);
				}

				bool SetName(const char* name, size_t nameLength)
				{
					fName = (char*)malloc(nameLength + 1);
					if (fName == NULL)
						return false;

					memcpy(fName, name, nameLength);
					fName[nameLength] = '\0';
					return true;
				}

				Entry* Parent() const
				{
					return fParent;
				}

				const char* Name() const
				{
					return fName;
				}

				bool IsBlackListed() const
				{
					return fIsBlackListed;
				}

				void SetBlackListed(bool blackListed)
				{
					fIsBlackListed = blackListed;
				}

				Entry*& HashNext()
				{
					return fHashNext;
				}

			private:
				Entry*	fParent;
				char*	fName;
				bool	fIsBlackListed;
				Entry*	fHashNext;
			};

			class EntryKey {
			public:
				EntryKey(Entry* parent, const char* name, size_t nameLength)
					:
					fParent(parent),
					fName(name),
					fNameLength(nameLength)
				{
				}

				EntryKey(Entry* parent, const char* name)
					:
					fParent(parent),
					fName(name),
					fNameLength(strlen(name))
				{
				}

				Entry* Parent() const
				{
					return fParent;
				}

				const char* Name() const
				{
					return fName;
				}

				size_t NameLength() const
				{
					return fNameLength;
				}

				size_t Hash() const
				{
					return (addr_t)fParent / 8
						^ hash_hash_string_part(fName, fNameLength);
				}

			private:
				Entry*		fParent;
				const char*	fName;
				size_t		fNameLength;
			};

public:
								PackageSettingsItem();
								~PackageSettingsItem();

	static	PackageSettingsItem* Load(::Directory* systemDirectory,
									const char* name);

			status_t			Init(const driver_parameter& parameter);

			void				AddEntry(Entry* entry);
			status_t			AddEntry(const char* path, Entry*& _entry);
			Entry*				FindEntry(Entry* parent, const char* name)
									const;
			Entry*				FindEntry(Entry* parent, const char* name,
									size_t nameLength) const;

			PackageSettingsItem*& HashNext()
									{ return fHashNext; }

private:
			struct EntryHashDefinition {
				typedef EntryKey	KeyType;
				typedef	Entry		ValueType;

				size_t HashKey(const EntryKey& key) const
				{
					return key.Hash();
				}

				size_t Hash(const Entry* value) const
				{
					return HashKey(EntryKey(value->Parent(), value->Name()));
				}

				bool Compare(const EntryKey& key, const Entry* value) const
				{
					const char* name = value->Name();
					return key.Parent() == value->Parent()
						&& strncmp(key.Name(), name, key.NameLength()) == 0
						&& name[key.NameLength()] == '\0';
				}

				Entry*& GetLink(Entry* value) const
				{
					return value->HashNext();
				}
			};

			typedef BOpenHashTable<EntryHashDefinition> EntryTable;

private:
			status_t			_AddBlackListedEntries(
									const driver_parameter& parameter);

private:
			EntryTable			fEntries;
			PackageSettingsItem* fHashNext;
};


}	// namespace PackageFS


#endif	// BOOT_LOADER_FILE_SYSTEMS_PACKAGEFS_PACKAGE_SETTINGS_ITEM_H
