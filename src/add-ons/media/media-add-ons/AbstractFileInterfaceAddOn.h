// AbstractFileInterfaceAddOn.h
//
// Andrew Bachmann, 2002
//
// AbstractFileInterfaceAddOn is an add-on
// that can make instances of AbstractFileInterfaceNode
//
// AbstractFileInterfaceNode handles a file and a multistream

#if !defined(_ABSTRACT_FILE_INTERFACE_ADD_ON_H)
#define _ABSTRACT_FILE_INTERFACE_ADD_ON_H

#include <MediaDefs.h>
#include <MediaAddOn.h>

class AbstractFileInterfaceAddOn :
    public BMediaAddOn
{
public:
	virtual ~AbstractFileInterfaceAddOn(void);
	explicit AbstractFileInterfaceAddOn(image_id image);

/**************************/
/* begin from BMediaAddOn */
public:
virtual	status_t InitCheck(
				const char ** out_failure_text);
virtual	int32 CountFlavors(void);
virtual	status_t GetFlavorAt(
				int32 n,
				const flavor_info ** out_info);
virtual	BMediaNode * InstantiateNodeFor(
				const flavor_info * info,
				BMessage * config,
				status_t * out_error) = 0;
virtual	status_t GetConfigurationFor(
				BMediaNode * your_node,
				BMessage * into_message);
virtual	bool WantsAutoStart(void);
virtual	status_t AutoStart(
				int in_count,
				BMediaNode ** out_node,
				int32 * out_internal_id,
				bool * out_has_more);

/* only implement if you have a B_FILE_INTERFACE node */
virtual	status_t SniffRef(
				const entry_ref & file,
				BMimeType * io_mime_type,
				float * out_quality,
				int32 * out_internal_id);
virtual	status_t SniffType(					//	This is broken if you deal with producers 
				const BMimeType & type,		//	and consumers both. Use SniffTypeKind instead.
				float * out_quality,		//	If you implement SniffTypeKind, this doesn't
				int32 * out_internal_id);	//	get called.
virtual	status_t GetFileFormatList(
				int32 flavor_id,			//	for this node flavor (if it matters)
				media_file_format * out_writable_formats, 	//	don't write here if NULL
				int32 in_write_items,		//	this many slots in out_writable_formats
				int32 * out_write_items,	//	set this to actual # available, even if bigger than in count
				media_file_format * out_readable_formats, 	//	don't write here if NULL
				int32 in_read_items,		//	this many slots in out_readable_formats
				int32 * out_read_items,		//	set this to actual # available, even if bigger than in count
				void * _reserved);			//	ignore until further notice
virtual	status_t SniffTypeKind(				//	Like SniffType, but for the specific kind(s)
				const BMimeType & type,
				uint64 in_kinds,
				float * out_quality,
				int32 * out_internal_id,
				void * _reserved) = 0;
virtual	status_t SniffTypeKind(				//	Like SniffType, but for the specific kind(s)
				const BMimeType & type,
				uint64 in_kinds,
				uint64 io_kind,
				float * out_quality,
				int32 * out_internal_id,
				void * _reserved);


/* end from BMediaAddOn */
/************************/

private:

		AbstractFileInterfaceAddOn(	/* private unimplemented */
				const AbstractFileInterfaceAddOn & clone);
		AbstractFileInterfaceAddOn & operator=(
				const AbstractFileInterfaceAddOn & clone);				

		int32 refCount;

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_0(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_1(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_2(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_3(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_4(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_5(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_6(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_7(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_8(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_9(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_10(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_11(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_12(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_13(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_14(void *);
virtual		status_t _Reserved_AbstractFileInterfaceAddOn_15(void *);

		uint32 _reserved_abstract_file_interface_add_on_[16];

};

#if BUILDING_ABSTRACT_FILE_INTERFACE__ADD_ON
extern "C" _EXPORT BMediaAddOn * make_abstract_file_interface_add_on(image_id you);
#endif

#endif /* _ABSTRACT_FILE_INTERFACE_ADD_ON_H */
