// MediaReaderAddOn.h
//
// Andrew Bachmann, 2002
//
// A MediaReaderAddOn is an add-on
// that can make MediaReader nodes
//
// MediaReader nodes read a file into a multistream

#if !defined(_MEDIA_READER_ADD_ON_H)
#define _MEDIA_READER_ADD_ON_H

#include <MediaDefs.h>
#include <MediaAddOn.h>
#include "../AbstractFileInterfaceAddOn.h"

class MediaReaderAddOn :
    public AbstractFileInterfaceAddOn
{
public:
	virtual ~MediaReaderAddOn(void);
	explicit MediaReaderAddOn(image_id image);

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

		MediaReaderAddOn(	/* private unimplemented */
				const MediaReaderAddOn & clone);
		MediaReaderAddOn & operator=(
				const MediaReaderAddOn & clone);				

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_MediaReaderAddOn_0(void *);
virtual		status_t _Reserved_MediaReaderAddOn_1(void *);
virtual		status_t _Reserved_MediaReaderAddOn_2(void *);
virtual		status_t _Reserved_MediaReaderAddOn_3(void *);
virtual		status_t _Reserved_MediaReaderAddOn_4(void *);
virtual		status_t _Reserved_MediaReaderAddOn_5(void *);
virtual		status_t _Reserved_MediaReaderAddOn_6(void *);
virtual		status_t _Reserved_MediaReaderAddOn_7(void *);
virtual		status_t _Reserved_MediaReaderAddOn_8(void *);
virtual		status_t _Reserved_MediaReaderAddOn_9(void *);
virtual		status_t _Reserved_MediaReaderAddOn_10(void *);
virtual		status_t _Reserved_MediaReaderAddOn_11(void *);
virtual		status_t _Reserved_MediaReaderAddOn_12(void *);
virtual		status_t _Reserved_MediaReaderAddOn_13(void *);
virtual		status_t _Reserved_MediaReaderAddOn_14(void *);
virtual		status_t _Reserved_MediaReaderAddOn_15(void *);

		uint32 _reserved_media_reader_add_on_[16];

};

#if BUILDING_MEDIA_READER__ADD_ON
extern "C" _EXPORT BMediaAddOn * make_media_reader_add_on(image_id you);
#endif

#endif /* _MEDIA_READER_ADD_ON_H */
