/*******************************************************************************
/
/	File:			MediaAddOn.h
/
/   Description:   A BMediaAddOn is created by Media Kit add-ons to instantiate and 
/	handle "latent" Nodes.
/
/	Copyright 1997-98, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_MEDIA_ADD_ON_H)
#define _MEDIA_ADD_ON_H

#include <image.h>

#include <MediaDefs.h>
#include <Flattenable.h>


struct dormant_node_info {
	dormant_node_info();
	~dormant_node_info();

	media_addon_id addon;
	int32 flavor_id;
	char name[B_MEDIA_NAME_LENGTH];
private:
	char reserved[128];
};



enum
{	//	flavor_flags
	B_FLAVOR_IS_GLOBAL = 0x100000L,	//	force in media_addon_server, only one instance
	B_FLAVOR_IS_LOCAL = 0x200000L	//	force in loading app, many instances
									//	if none is set, could go either way
};

struct flavor_info {
	char *				name;
	char *				info;
	uint64				kinds;			/* node_kind */
	uint32				flavor_flags;
	int32				internal_id;	/* For BMediaAddOn internal use */
	int32				possible_count;	/* 0 for "any number" */

	int32				in_format_count;	/* for BufferConsumer kinds */
	uint32				in_format_flags;	/* set to 0 */
	const media_format *	in_formats;

	int32				out_format_count;	/* for BufferProducer kinds */
	uint32				out_format_flags;	/* set to 0 */
	const media_format *	out_formats;

	uint32				_reserved_[16];

private:
	flavor_info & operator=(const flavor_info & other);
};

struct dormant_flavor_info : public flavor_info, public BFlattenable {

		dormant_flavor_info();
virtual	~dormant_flavor_info();
		dormant_flavor_info(const dormant_flavor_info &);
		dormant_flavor_info& operator=(const dormant_flavor_info &);
		dormant_flavor_info& operator=(const flavor_info &);

		dormant_node_info	node_info;

		void set_name(const char * in_name);
		void set_info(const char * in_info);
		void add_in_format(const media_format & in_format);
		void add_out_format(const media_format & out_format);

virtual	bool		IsFixedSize() const;
virtual	type_code	TypeCode() const;
virtual	ssize_t		FlattenedSize() const;
virtual	status_t	Flatten(void *buffer, ssize_t size) const;
virtual	status_t	Unflatten(type_code c, const void *buf, ssize_t size);
};



/* a MediaAddOn is something which can manufacture MediaNodes */
class BMediaAddOn
{
public:
explicit	BMediaAddOn(
				image_id image);
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

		image_id ImageID();
		media_addon_id AddonID();

protected:

		//	Calling this will cause everyone to get notified, and also
		//	cause the server to re-scan your flavor info. It is thread safe.
		status_t			NotifyFlavorChange();

private:
		BMediaAddOn();	/* private unimplemented */
		BMediaAddOn(const BMediaAddOn & clone);
		BMediaAddOn & operator=(const BMediaAddOn & clone);

		/* Mmmh, stuffing! */
virtual		status_t _Reserved_MediaAddOn_2(void *);
virtual		status_t _Reserved_MediaAddOn_3(void *);
virtual		status_t _Reserved_MediaAddOn_4(void *);
virtual		status_t _Reserved_MediaAddOn_5(void *);
virtual		status_t _Reserved_MediaAddOn_6(void *);
virtual		status_t _Reserved_MediaAddOn_7(void *);

	image_id 		_fImage;
	media_addon_id	_fAddon;
	uint32			_reserved_media_add_on_[7];
};


#if BUILDING_MEDIA_ADDON
extern "C" _EXPORT BMediaAddOn * make_media_addon(image_id you);
#endif


#endif /* _MEDIA_ADD_ON_H */

