// MediaWriterAddOn.h
//
// Andrew Bachmann, 2002
//
// A MediaWriterAddOn is an add-on
// that can make MediaWriter nodes
//
// MediaWriter nodes write a multistream into a file

#if !defined(_MEDIA_WRITER_ADD_ON_H)
#define _MEDIA_WRITER_ADD_ON_H

#include <MediaDefs.h>
#include <MediaAddOn.h>
#include "../AbstractFileInterfaceAddOn.h"

class MediaWriterAddOn :
    public AbstractFileInterfaceAddOn
{
public:
	virtual ~MediaWriterAddOn(void);
	explicit MediaWriterAddOn(image_id image);

/**************************/
/* begin from BMediaAddOn */
public:
virtual	status_t GetFlavorAt(
				int32 n,
				const flavor_info ** out_info);
virtual	BMediaNode * InstantiateNodeFor(
				const flavor_info * info,
				BMessage * config,
				status_t * out_error);
virtual	status_t GetConfigurationFor(
				BMediaNode * your_node,
				BMessage * into_message);

/* only implement if you have a B_FILE_INTERFACE node */
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
				void * _reserved);

/* end from BMediaAddOn */
/************************/

private:

		MediaWriterAddOn(	/* private unimplemented */
				const MediaWriterAddOn & clone);
		MediaWriterAddOn & operator=(
				const MediaWriterAddOn & clone);				

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_MediaWriterAddOn_0(void *);
virtual		status_t _Reserved_MediaWriterAddOn_1(void *);
virtual		status_t _Reserved_MediaWriterAddOn_2(void *);
virtual		status_t _Reserved_MediaWriterAddOn_3(void *);
virtual		status_t _Reserved_MediaWriterAddOn_4(void *);
virtual		status_t _Reserved_MediaWriterAddOn_5(void *);
virtual		status_t _Reserved_MediaWriterAddOn_6(void *);
virtual		status_t _Reserved_MediaWriterAddOn_7(void *);
virtual		status_t _Reserved_MediaWriterAddOn_8(void *);
virtual		status_t _Reserved_MediaWriterAddOn_9(void *);
virtual		status_t _Reserved_MediaWriterAddOn_10(void *);
virtual		status_t _Reserved_MediaWriterAddOn_11(void *);
virtual		status_t _Reserved_MediaWriterAddOn_12(void *);
virtual		status_t _Reserved_MediaWriterAddOn_13(void *);
virtual		status_t _Reserved_MediaWriterAddOn_14(void *);
virtual		status_t _Reserved_MediaWriterAddOn_15(void *);

		uint32 _reserved_media_writer_add_on_[16];

};

#if BUILDING_MEDIA_WRITER__ADD_ON
extern "C" _EXPORT BMediaAddOn * make_media_writer_add_on(image_id you);
#endif

#endif /* _MEDIA_WRITER_ADD_ON_H */
