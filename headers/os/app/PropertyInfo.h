/*
 * Copyright 2009-2010, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef _PROPERTY_INFO_H
#define _PROPERTY_INFO_H


#include <BeBuild.h>
#include <Flattenable.h>
#include <SupportDefs.h>
#include <TypeConstants.h>


class BMessage;


struct compound_type {
	struct field_pair {
		const char*	name;
		type_code	type;
	};
	field_pair	pairs[5];
};


struct property_info {
	const char*		name;
	uint32			commands[10];
	uint32			specifiers[10];
	const char*		usage;
	uint32			extra_data;
	uint32			types[10];
	compound_type	ctypes[3];
	uint32			_reserved[10];
};


enum value_kind {
	B_COMMAND_KIND = 0,
	B_TYPE_CODE_KIND = 1
};


struct value_info {
	const char*		name;
	uint32			value;
	value_kind		kind;
	const char*		usage;
	uint32			extra_data;
	uint32			_reserved[10];
};


class BPropertyInfo : public BFlattenable {
public:
								BPropertyInfo(property_info* prop = NULL,
									value_info* value = NULL,
									bool freeOnDelete = false);
	virtual						~BPropertyInfo();

	virtual	int32				FindMatch(BMessage* msg, int32 index,
									BMessage* specifier, int32 form,
									const char* prop, void* data = NULL) const;

	virtual	bool				IsFixedSize() const;
	virtual	type_code			TypeCode() const;
	virtual	ssize_t				FlattenedSize() const;
	virtual	status_t			Flatten(void* buffer, ssize_t size) const;
	virtual	bool				AllowsTypeCode(type_code code) const;
	virtual	status_t			Unflatten(type_code code, const void* buffer,
									ssize_t size);

		const property_info*	Properties() const;
		const value_info*		Values() const;
		int32					CountProperties() const;
		int32					CountValues() const;

		void					PrintToStream() const;

protected:
	static	bool				FindCommand(uint32 what, int32 index,
									property_info* info);
	static	bool				FindSpecifier(uint32 form, property_info* info);

private:
	virtual	void				_ReservedPropertyInfo1();
	virtual	void				_ReservedPropertyInfo2();
	virtual	void				_ReservedPropertyInfo3();
	virtual	void				_ReservedPropertyInfo4();

								BPropertyInfo(const BPropertyInfo& other);
			BPropertyInfo&		operator=(const BPropertyInfo& other);
			void				FreeMem();

			property_info*		fPropInfo;
			value_info*			fValueInfo;
			int32				fPropCount;
			bool				fInHeap;
			uint16				fValueCount;
			uint32				_reserved[4];
};


#endif	/* _PROPERTY_INFO_H */
