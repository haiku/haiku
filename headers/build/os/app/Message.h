//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Message.h
//	Author(s):		Erik Jaesler (erik@cgsoftware.com)
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	BMessage class creates objects that store data and that
//					can be processed in a message loop.  BMessage objects
//					are also used as data containers by the archiving and 
//					the scripting mechanisms.
//------------------------------------------------------------------------------

#ifndef _MESSAGE_H
#define _MESSAGE_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <BeBuild.h>
#include <OS.h>
#include <Rect.h>
#include <DataIO.h>
#include <Flattenable.h>

#include <AppDefs.h>		/* For convenience */
#include <TypeConstants.h>	/* For convenience */

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------



class	BBlockCache;
class	BMessenger;
class	BHandler;
class	BString;

// Private or reserved ---------------------------------------------------------
extern "C" void		_msg_cache_cleanup_();
extern "C" int		_init_message_();
extern "C" int		_delete_message_();
//------------------------------------------------------------------------------


// Name lengths and Scripting specifiers ---------------------------------------
#define B_FIELD_NAME_LENGTH			255
#define B_PROPERTY_NAME_LENGTH		255

enum
{
	B_NO_SPECIFIER = 0,
	B_DIRECT_SPECIFIER = 1,
	B_INDEX_SPECIFIER,
	B_REVERSE_INDEX_SPECIFIER,
	B_RANGE_SPECIFIER,
	B_REVERSE_RANGE_SPECIFIER,
	B_NAME_SPECIFIER,
	B_ID_SPECIFIER,

	B_SPECIFIERS_END = 128
	// app-defined specifiers start at B_SPECIFIERS_END+1
};

namespace BPrivate {
	class BMessageBody;
}

// BMessage class --------------------------------------------------------------
class BMessage
{
public:
		uint32		what;

					BMessage();
					BMessage(uint32 what);
					BMessage(const BMessage &a_message);
virtual				~BMessage();

		BMessage	&operator=(const BMessage &msg);

// Statistics and misc info
		status_t	GetInfo(type_code typeRequested, int32 which, char **name,
							type_code *typeReturned, int32 *count = NULL) const;

		status_t	GetInfo(const char *name, type_code *type, int32 *c = 0) const;
		status_t	GetInfo(const char *name, type_code *type, bool *fixed_size) const;

		int32		CountNames(type_code type) const;
		bool		IsEmpty() const;
		bool		IsSystem() const;
		bool		IsReply() const;
		void		PrintToStream() const;

		status_t	Rename(const char *old_entry, const char *new_entry);

// Delivery info
		bool		WasDelivered() const;
		bool		IsSourceWaiting() const;
		BMessenger	ReturnAddress() const;
		const BMessage	*Previous() const;
		bool		WasDropped() const;
		BPoint		DropPoint(BPoint *offset = NULL) const;

// Flattening data
		ssize_t		FlattenedSize() const;
		status_t	Flatten(char *buffer, ssize_t size) const;
		status_t	Flatten(BDataIO *stream, ssize_t *size = NULL) const;
		status_t	Unflatten(const char *flat_buffer);
		status_t	Unflatten(BDataIO *stream);


// Specifiers (scripting)
		status_t	AddSpecifier(const char *property);
		status_t	AddSpecifier(const char *property, int32 index);
		status_t	AddSpecifier(const char *property, int32 index, int32 range);
		status_t	AddSpecifier(const char *property, const char *name);
		status_t	AddSpecifier(const BMessage *specifier);

		status_t	SetCurrentSpecifier(int32 index);
		status_t	GetCurrentSpecifier(int32 *index, BMessage *specifier = NULL,
							int32 *form = NULL, const char **property = NULL) const;
		bool		HasSpecifiers() const;
		status_t	PopSpecifier();

// Adding data
		status_t	AddRect(const char *name, BRect a_rect);
		status_t	AddPoint(const char *name, BPoint a_point);
		status_t	AddString(const char *name, const char *a_string);
		status_t	AddString(const char *name, const BString& a_string);
		status_t	AddInt8(const char *name, int8 val);
		status_t	AddInt16(const char *name, int16 val);
		status_t	AddInt32(const char *name, int32 val);
		status_t	AddInt64(const char *name, int64 val);
		status_t	AddBool(const char *name, bool a_boolean);
		status_t	AddFloat(const char *name, float a_float);
		status_t	AddDouble(const char *name, double a_double);
		status_t	AddPointer(const char *name, const void *ptr);
		status_t	AddMessenger(const char *name, BMessenger messenger);
		status_t	AddRef(const char *name, const entry_ref *ref);
		status_t	AddMessage(const char *name, const BMessage *msg);
		status_t	AddFlat(const char *name, BFlattenable *obj, int32 count = 1);
		status_t	AddData(const char *name, type_code type, const void *data,
						ssize_t numBytes, bool is_fixed_size = true, int32 count = 1);

// Removing data
		status_t	RemoveData(const char *name, int32 index = 0);
		status_t	RemoveName(const char *name);
		status_t	MakeEmpty();

// Finding data
		status_t	FindRect(const char *name, BRect *rect) const;
		status_t	FindRect(const char *name, int32 index, BRect *rect) const;
		status_t	FindPoint(const char *name, BPoint *pt) const;
		status_t	FindPoint(const char *name, int32 index, BPoint *pt) const;
		status_t	FindString(const char *name, const char **str) const;
		status_t	FindString(const char *name, int32 index, const char **str) const;
		status_t	FindString(const char *name, BString *str) const;
		status_t	FindString(const char *name, int32 index, BString *str) const;
		status_t	FindInt8(const char *name, int8 *value) const;
		status_t	FindInt8(const char *name, int32 index, int8 *val) const;
		status_t	FindInt16(const char *name, int16 *value) const;
		status_t	FindInt16(const char *name, int32 index, int16 *val) const;
		status_t	FindInt32(const char *name, int32 *value) const;
		status_t	FindInt32(const char *name, int32 index, int32 *val) const;
		status_t	FindInt64(const char *name, int64 *value) const;
		status_t	FindInt64(const char *name, int32 index, int64 *val) const;
		status_t	FindBool(const char *name, bool *value) const;
		status_t	FindBool(const char *name, int32 index, bool *value) const;
		status_t	FindFloat(const char *name, float *f) const;
		status_t	FindFloat(const char *name, int32 index, float *f) const;
		status_t	FindDouble(const char *name, double *d) const;
		status_t	FindDouble(const char *name, int32 index, double *d) const;
		status_t	FindPointer(const char *name, void **ptr) const;
		status_t	FindPointer(const char *name, int32 index,  void **ptr) const;
		status_t	FindMessenger(const char *name, BMessenger *m) const;
		status_t	FindMessenger(const char *name, int32 index, BMessenger *m) const;
		status_t	FindRef(const char *name, entry_ref *ref) const;
		status_t	FindRef(const char *name, int32 index, entry_ref *ref) const;
		status_t	FindMessage(const char *name, BMessage *msg) const;
		status_t	FindMessage(const char *name, int32 index, BMessage *msg) const;
		status_t	FindFlat(const char *name, BFlattenable *obj) const;
		status_t	FindFlat(const char *name, int32 index, BFlattenable *obj) const;
		status_t	FindData(const char *name, type_code type,
							const void **data, ssize_t *numBytes) const;
		status_t	FindData(const char *name, type_code type, int32 index,
							const void **data, ssize_t *numBytes) const;

// Replacing data
		status_t	ReplaceRect(const char *name, BRect a_rect);
		status_t	ReplaceRect(const char *name, int32 index, BRect a_rect);
		status_t	ReplacePoint(const char *name, BPoint a_point);
		status_t	ReplacePoint(const char *name, int32 index, BPoint a_point);
		status_t	ReplaceString(const char *name, const char *string);
		status_t	ReplaceString(const char *name, int32 index, const char *string);
		status_t	ReplaceString(const char *name, const BString& string);
		status_t	ReplaceString(const char *name, int32 index, const BString& string);
		status_t	ReplaceInt8(const char *name, int8 val);
		status_t	ReplaceInt8(const char *name, int32 index, int8 val);
		status_t	ReplaceInt16(const char *name, int16 val);
		status_t	ReplaceInt16(const char *name, int32 index, int16 val);
		status_t	ReplaceInt32(const char *name, int32 val);
		status_t	ReplaceInt32(const char *name, int32 index, int32 val);
		status_t	ReplaceInt64(const char *name, int64 val);
		status_t	ReplaceInt64(const char *name, int32 index, int64 val);
		status_t	ReplaceBool(const char *name, bool a_bool);
		status_t	ReplaceBool(const char *name, int32 index, bool a_bool);
		status_t	ReplaceFloat(const char *name, float a_float);
		status_t	ReplaceFloat(const char *name, int32 index, float a_float);
		status_t	ReplaceDouble(const char *name, double a_double);
		status_t	ReplaceDouble(const char *name, int32 index, double a_double);
		status_t	ReplacePointer(const char *name, const void *ptr);
		status_t	ReplacePointer(const char *name,int32 index,const void *ptr);
		status_t	ReplaceMessenger(const char *name, BMessenger messenger);
		status_t	ReplaceMessenger(const char *name, int32 index, BMessenger msngr);
		status_t	ReplaceRef(	const char *name,const entry_ref *ref);
		status_t	ReplaceRef(	const char *name, int32 index, const entry_ref *ref);
		status_t	ReplaceMessage(const char *name, const BMessage *msg);
		status_t	ReplaceMessage(const char *name, int32 index, const BMessage *msg);
		status_t	ReplaceFlat(const char *name, BFlattenable *obj);
		status_t	ReplaceFlat(const char *name, int32 index, BFlattenable *obj);
		status_t	ReplaceData(const char *name, type_code type,
								const void *data, ssize_t data_size);
		status_t	ReplaceData(const char *name, type_code type, int32 index,
								const void *data, ssize_t data_size);

		void		*operator new(size_t size);
		void		*operator new(size_t, void* p);
		void		operator delete(void *ptr, size_t size);

// Private, reserved, or obsolete ----------------------------------------------
		bool		HasRect(const char *, int32 n = 0) const;
		bool		HasPoint(const char *, int32 n = 0) const;
		bool		HasString(const char *, int32 n = 0) const;
		bool		HasInt8(const char *, int32 n = 0) const;
		bool		HasInt16(const char *, int32 n = 0) const;
		bool		HasInt32(const char *, int32 n = 0) const;
		bool		HasInt64(const char *, int32 n = 0) const;
		bool		HasBool(const char *, int32 n = 0) const;
		bool		HasFloat(const char *, int32 n = 0) const;
		bool		HasDouble(const char *, int32 n = 0) const;
		bool		HasPointer(const char *, int32 n = 0) const;
		bool		HasMessenger(const char *, int32 n = 0) const;
		bool		HasRef(const char *, int32 n = 0) const;
		bool		HasMessage(const char *, int32 n = 0) const;
		bool		HasFlat(const char *, const BFlattenable *) const;
		bool		HasFlat(const char *,int32 ,const BFlattenable *) const;
		bool		HasData(const char *, type_code , int32 n = 0) const;
		BRect		FindRect(const char *, int32 n = 0) const;
		BPoint		FindPoint(const char *, int32 n = 0) const;
		const char	*FindString(const char *, int32 n = 0) const;
		int8		FindInt8(const char *, int32 n = 0) const;
		int16		FindInt16(const char *, int32 n = 0) const;
		int32		FindInt32(const char *, int32 n = 0) const;
		int64		FindInt64(const char *, int32 n = 0) const;
		bool		FindBool(const char *, int32 n = 0) const;
		float		FindFloat(const char *, int32 n = 0) const;
		double		FindDouble(const char *, int32 n = 0) const;

		class Private;

private:
		class Header;

friend class	BMessageQueue;
friend class	BMessenger;
friend class	BApplication;
friend class	Header;
friend class	Private;

friend inline	void		_set_message_target_(BMessage *, int32, bool);
friend inline	void		_set_message_reply_(BMessage *, BMessenger);
friend inline	int32		_get_message_target_(BMessage *);
friend inline	bool		_use_preferred_target_(BMessage *);

					// deprecated
					BMessage(BMessage *a_message);
					
virtual	void		_ReservedMessage1();
virtual	void		_ReservedMessage2();
virtual	void		_ReservedMessage3();

		void		init_data();
		status_t	flatten_target_info(BDataIO *stream,
										ssize_t size,
										uchar flags) const;
		status_t	real_flatten(char *result,
								ssize_t size) const;
		status_t	real_flatten(BDataIO *stream) const;
		char		*stack_flatten(char *stack_ptr,
									ssize_t stack_size,
									bool incl_reply,
									ssize_t *size = NULL) const;

		ssize_t		calc_size(uchar flags) const;
		ssize_t		calc_hdr_size(uchar flags) const;
		status_t	nfind_data(	const char *name,
								type_code type,
								int32 index,
								const void **data,
								ssize_t *data_size) const;
		status_t	copy_data(	const char *name,
								type_code type,
								int32 index,
								void *data,
								ssize_t data_size) const;

static	void		_StaticInit();
static	void		_StaticCleanup();
static	void		_StaticCacheCleanup();

static	BBlockCache	*sMsgCache;

		struct dyn_array {
			int32		fLogicalBytes;
			int32		fPhysicalBytes;
			int32		fChunkSize;		
			int32		fCount;
			int32		fEntryHdrSize;	
		};

		struct entry_hdr  : public dyn_array {
			entry_hdr	*fNext;
			uint32		fType;
			uchar		fNameLength;	
			char		fName[1];
		};

		struct var_chunk {
			int32	fDataSize;				
			char	fData[1];
		};

		entry_hdr 	*entry_find(const char *name, uint32 type,status_t *result=NULL) const;
		void 		entry_remove(entry_hdr *entry);
		
		void		*da_create(int32 header_size, int32 chunk_size,
								bool fixed, int32 nchunks);
		status_t	da_add_data(dyn_array **da, const void *data, int32 size);
		void		*da_find_data(dyn_array *da, int32 index,
									int32 *size = NULL) const;
		status_t	da_delete_data(dyn_array **pda, int32 index);
		status_t	da_replace_data(dyn_array **pda, int32 index,
									const void *data, int32 dsize);
		int32		da_calc_size(int32 hdr_size, int32 chunksize,
								bool is_fixed, int32 nchunks) const;
		void		*da_grow(dyn_array **pda, int32 increase);
		void		da_dump(dyn_array *da);

		int32		da_chunk_hdr_size() const
						{ return sizeof(int32); }
		int32		da_chunk_size(var_chunk *v) const
						{ return (v->fDataSize + da_chunk_hdr_size() + 7) & ~7; }
		var_chunk	*da_first_chunk(dyn_array *da) const
						{ return (var_chunk *) da_start_of_data(da); }
		var_chunk	*da_next_chunk(var_chunk *v) const
						{ return (var_chunk *) (((char*) v) + da_chunk_size(v)); }
		var_chunk	*da_chunk_ptr(void *data) const
						{ return (var_chunk*) (((char *) data) - da_chunk_hdr_size()); }

		int32		da_pad_8(int32 val) const
						{ return (val + 7) & ~7; }
		int32		da_total_size(dyn_array *da) const
						{ return (int32)sizeof(dyn_array) + da->fEntryHdrSize +
											da->fPhysicalBytes; }
		int32		da_total_logical_size(dyn_array *da) const
						{ return (int32)sizeof(dyn_array) + da->fEntryHdrSize +
											da->fLogicalBytes; }
		char		*da_start_of_data(dyn_array *da) const
						{ return ((char *) da) + (sizeof(dyn_array) +
											da->fEntryHdrSize); }
		bool		da_is_mini_data(dyn_array *da) const
						{ return ((da->fLogicalBytes <= (int32) UCHAR_MAX) &&
											(da->fCount <= (int32) UCHAR_MAX));}
		void		da_swap_var_sized(dyn_array *da);
		void		da_swap_fixed_sized(dyn_array *da);

		BMessage			*link;
		int32				fTarget;	
		BMessage			*fOriginal;
		uint32				fChangeCount;
		int32				fCurSpecifier;
		uint32				fPtrOffset;

		// ejaesler: Stealing one for my whacky BMessageBody l33tness
		uint32				_reserved[2];
		BPrivate::BMessageBody*	fBody;

		BMessage::entry_hdr	*fEntries;

		struct reply_to_info {
			port_id				port;
			int32				target;
			team_id				team;
			bool				preferred;
		} fReplyTo;

		bool				fPreferred;
		bool				fReplyRequired;
		bool				fReplyDone;
		bool				fIsReply;
		bool				fWasDelivered;
		bool				fReadOnly;
		bool				fHasSpecifiers;	
};

//------------------------------------------------------------------------------

#endif	// _MESSAGE_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

