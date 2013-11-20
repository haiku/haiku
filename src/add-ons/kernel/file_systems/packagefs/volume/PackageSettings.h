/*
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_SETTINGS_H
#define PACKAGE_SETTINGS_H


#include <packagefs.h>
#include <util/OpenHashTable.h>

#include "StringKey.h"


struct driver_parameter;


class PackageSettingsItem {
public:
			class Entry {
			public:
				Entry(Entry* parent, const String& name)
					:
					fParent(parent),
					fName(name),
					fIsBlackListed(false)
				{
				}

				Entry* Parent() const
				{
					return fParent;
				}

				const String& Name() const
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
				String	fName;
				bool	fIsBlackListed;
				Entry*	fHashNext;
			};

			class EntryKey {
			public:
				EntryKey(Entry* parent, const char* name)
					:
					fParent(parent),
					fName(name),
					fHash((addr_t)parent / 8 ^ hash_hash_string(name))
				{
				}

				EntryKey(Entry* parent, const String& name)
					:
					fParent(parent),
					fName(name),
					fHash((addr_t)parent / 8 ^ name.Hash())
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

				size_t Hash() const
				{
					return fHash;
				}

			private:
				Entry*		fParent;
				const char*	fName;
				size_t		fHash;
			};

public:
								PackageSettingsItem();
								~PackageSettingsItem();

			status_t			Init(const char* name);
			status_t			ApplySettings(
									const driver_parameter* parameters,
									int parameterCount);

			const String&		Name() const
									{ return fName; }

			void				AddEntry(Entry* entry);
			status_t			AddEntry(const char* path, Entry*& _entry);
			Entry*				FindEntry(Entry* parent, const String& name)
									const;
			Entry*				FindEntry(Entry* parent, const char* name)
									const;

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
					return key.Parent() == value->Parent()
						&& strcmp(key.Name(), value->Name()) == 0;
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
			String				fName;
			EntryTable			fEntries;
			PackageSettingsItem* fHashNext;
};


struct PackageSettingsItemHashDefinition {
	typedef StringKey			KeyType;
	typedef	PackageSettingsItem	ValueType;

	size_t HashKey(const StringKey& key) const
	{
		return key.Hash();
	}

	size_t Hash(const PackageSettingsItem* value) const
	{
		return HashKey(value->Name());
	}

	bool Compare(const StringKey& key, const PackageSettingsItem* value) const
	{
		return key == value->Name();
	}

	PackageSettingsItem*& GetLink(PackageSettingsItem* value) const
	{
		return value->HashNext();
	}
};


class PackageSettings {
public:
								PackageSettings();
								~PackageSettings();

			status_t			Load(dev_t mountPointDeviceID,
									ino_t mountPointNodeID,
									PackageFSMountType mountType);

			const PackageSettingsItem* PackageItemFor(const String& name) const;

private:
			typedef BOpenHashTable<PackageSettingsItemHashDefinition>
				PackageItemTable;

private:
			status_t			_AddPackageSettingsItem(const char* name,
									const driver_parameter* parameters,
									int parameterCount);

private:
			PackageItemTable	fPackageItems;
};


#endif	// PACKAGE_SETTINGS_H
