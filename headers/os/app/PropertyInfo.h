/*******************************************************************************
/
/	File:			PropertyInfo.h
/
/	Description:	Utility class for maintain scripting information
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#ifndef _PROPERTY_INFO_H
#define _PROPERTY_INFO_H

#include <BeBuild.h>
#include <SupportDefs.h>
#include <Flattenable.h>
#include <TypeConstants.h>	/* For convenience */

/*----------------------------------------------------------------*/
/*----- the property_info structure ------------------------------*/


struct _oproperty_info_;

struct compound_type {
	struct field_pair {
		char		*name;			// name of entry in message
		type_code	type;			// type_code of entry in message
	};
	field_pair	pairs[5];
};

struct property_info {
	char			*name;
	uint32			commands[10];
	uint32			specifiers[10];
	char			*usage;
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
	char			*name;
	uint32			value;
	value_kind		kind;
	char			*usage;
	uint32			extra_data;
	uint32			_reserved[10];
};

#define B_PROPERTY_INFO_TYPE		'SCTD'

/*----------------------------------------------------------------*/
/*----- BPropertyInfo class --------------------------------------*/

class BPropertyInfo : public BFlattenable {
public:
								BPropertyInfo(property_info *p = NULL,
												value_info *ci = NULL,
												bool free_on_delete = false);
virtual							~BPropertyInfo();

virtual	int32					FindMatch(BMessage *msg,
											int32 index,
											BMessage *spec,
											int32 form,
											const char *prop,
											void *data = NULL) const;

virtual	bool					IsFixedSize() const;
virtual	type_code				TypeCode() const;
virtual	ssize_t					FlattenedSize() const;
virtual	status_t				Flatten(void *buffer, ssize_t size) const;
virtual	bool					AllowsTypeCode(type_code code) const;
virtual	status_t				Unflatten(type_code c,
											const void *buf,
											ssize_t s);
		const property_info		*Properties() const;
		const value_info		*Values() const;
		int32					CountProperties() const;
		int32					CountValues() const;

		void					PrintToStream() const;

/*----- Private or reserved -----------------------------------------*/
protected:
static	bool					FindCommand(uint32, int32, property_info *);
static	bool					FindSpecifier(uint32, property_info *);

private:
virtual	void					_ReservedPropertyInfo1();
virtual	void					_ReservedPropertyInfo2();
virtual	void					_ReservedPropertyInfo3();
virtual	void					_ReservedPropertyInfo4();

							BPropertyInfo(const BPropertyInfo &);
		BPropertyInfo		&operator=(const BPropertyInfo &);
		void				FreeMem();
		void				FreeInfoArray(property_info *p, int32);
		void				FreeInfoArray(value_info *p, int32);
		void				FreeInfoArray(_oproperty_info_ *p, int32);
static	property_info		*ConvertToNew(const _oproperty_info_ *p);
static	_oproperty_info_	*ConvertFromNew(const property_info *p);

		property_info		*fPropInfo;
		value_info			*fValueInfo;
		int32				fPropCount;
		bool				fInHeap;
		bool				fOldInHeap;
		uint16				fValueCount;
		_oproperty_info_	*fOldPropInfo;
		uint32				_reserved[2]; /* was 4 */

		/* Deprecated */
private:
		const property_info		*PropertyInfo() const;
								BPropertyInfo(property_info *p,
												bool free_on_delete);
static	bool					MatchCommand(uint32, int32, property_info *);
static	bool					MatchSpecifier(uint32, property_info *);

};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _PROPERTY_INFO_H */
