/*******************************************************************************
/
/	File:			PropertyInfo.h
/
/	Description:	Utility class for maintain scripting information
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/   Modified for use with OpenBeOS.
/
*******************************************************************************/

#ifndef _PROPERTY_INFO_H
#define _PROPERTY_INFO_H

#include <BeBuild.h>
#include <SupportDefs.h>
#include <Flattenable.h>
#include <TypeConstants.h>	/* For convenience */

/*
 * The following define is used to turn off a number of member functions 
 * and member variables in BPropertyInfo which are only there to faciliate
 * BeOS R3 compatibility.  Be changed the property_info structure between
 * R3 and R4 and these functions ensure that BPropertyInfo is backward
 * compatible.  However, between R3 and R4, Be changed the Intel executable
 * format so R4 was not backward compatible with R3.
 *
 * Because OpenBeOS is only targetting Intel platform compatibility at this
 * time, there is no need for R3 compatibility in OpenBeOS.  By default
 * these members will be turned off with the R3_COMPATIBLE define.
 */
#undef R3_COMPATIBLE

#ifdef R3_COMPATIBLE
struct _oproperty_info_;
#endif


/*----------------------------------------------------------------*/
/*----- the property_info structure ------------------------------*/

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

		property_info		*fPropInfo;
		value_info			*fValueInfo;
		int32				fPropCount;
		bool				fInHeap;
		uint16				fValueCount;
#ifndef R3_COMPATIBLE
		uint32				_reserved[4];
#else
		_oproperty_info_	*fOldPropInfo;
		bool				fOldInHeap;
		uint32				_reserved[2]; /* was 4 */

		/* Deprecated */
private:
static	property_info		*ConvertToNew(const _oproperty_info_ *p);
static	_oproperty_info_	*ConvertFromNew(const property_info *p);
		const property_info		*PropertyInfo() const;
								BPropertyInfo(property_info *p,
												bool free_on_delete);
static	bool					MatchCommand(uint32, int32, property_info *);
static	bool					MatchSpecifier(uint32, property_info *);

#endif
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

#endif /* _PROPERTY_INFO_H */
