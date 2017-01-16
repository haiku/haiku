// A MediaFileProducerAddOn is an add-on
// that can make MediaFileProducer nodes

#if !defined(_MEDIA_FILE_PRODUCER_ADD_ON_H)
#define _MEDIA_FILE_PRODUCER_ADD_ON_H

#include <MediaDefs.h>
#include <MediaAddOn.h>

class MediaFileProducerAddOn :
    public BMediaAddOn
{
public:
MediaFileProducerAddOn();
virtual ~MediaFileProducerAddOn();

/**************************/
/* begin from BMediaAddOn */
public:
virtual	~BMediaAddOn();

virtual	status_t InitCheck(
				const char ** out_failure_text);
virtual	int32 CountFlavors();
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
virtual	bool WantsAutoStart();
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
				void * _reserved);


/* end from BMediaAddOn */
/************************/

private:

		MediaFileProducerAddOn(	/* private unimplemented */
				const MediaFileProducerAddOn & clone);
		MediaFileProducerAddOn & operator=(
				const MediaFileProducerAddOn & clone);				

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_MediaFileProducerAddOn_0(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_1(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_2(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_3(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_4(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_5(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_6(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_7(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_8(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_9(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_10(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_11(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_12(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_13(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_14(void *);
virtual		status_t _Reserved_MediaFileProducerAddOn_15(void *);

		uint32 _reserved_media_file_node_[16];

};

#if BUILDING_MEDIA_FILE_PRODUCER__ADD_ON
extern "C" _EXPORT BMediaAddOn * make_media_file_producer_add_on(image_id you);
#endif

#endif /* _MEDIA_FILE_PRODUCER_ADD_ON_H */
