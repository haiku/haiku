// MediaDemultiplexerAddOn.h
//
// Andrew Bachmann, 2002
//
// MediaDemultiplexerAddOn is an add-on
// that can make instances of MediaDemultiplexerNode
//
// MediaDemultiplexerNode handles a file and a multistream

#if !defined(_MEDIA_DEMULTIPLEXER_ADD_ON_H)
#define _MEDIA_DEMULTIPLEXER_ADD_ON_H

#include <MediaDefs.h>
#include <MediaAddOn.h>

class MediaDemultiplexerAddOn :
    public BMediaAddOn
{
public:
	virtual ~MediaDemultiplexerAddOn(void);
	explicit MediaDemultiplexerAddOn(image_id image);

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
				status_t * out_error);
virtual	status_t GetConfigurationFor(
				BMediaNode * your_node,
				BMessage * into_message);
virtual	bool WantsAutoStart(void);
virtual	status_t AutoStart(
				int in_count,
				BMediaNode ** out_node,
				int32 * out_internal_id,
				bool * out_has_more);

/* end from BMediaAddOn */
/************************/

private:

		MediaDemultiplexerAddOn(	/* private unimplemented */
				const MediaDemultiplexerAddOn & clone);
		MediaDemultiplexerAddOn & operator=(
				const MediaDemultiplexerAddOn & clone);				

		int32 refCount;

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_MediaDemultiplexerAddOn_0(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_1(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_2(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_3(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_4(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_5(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_6(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_7(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_8(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_9(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_10(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_11(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_12(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_13(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_14(void *);
virtual		status_t _Reserved_MediaDemultiplexerAddOn_15(void *);

		uint32 _reserved_media_demultiplexer_add_on_[16];

};

#if BUILDING_MEDIA_DEMULTIPLEXER__ADD_ON
extern "C" _EXPORT BMediaAddOn * make_media_demultiplexer_add_on(image_id you);
#endif

#endif /* _MEDIA_DEMULTIPLEXER_ADD_ON_H */
