/*
 * Copyright 2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _MEDIA_ADD_ON_H
#define _MEDIA_ADD_ON_H


#include <image.h>

#include <MediaDefs.h>
#include <Flattenable.h>


class BMediaNode;
class BMimeType;
struct entry_ref;

struct dormant_node_info {
								dormant_node_info();
								~dormant_node_info();

			media_addon_id		addon;
			int32				flavor_id;
			char				name[B_MEDIA_NAME_LENGTH];

private:
			char				reserved[128];
};

// flavor_flags
enum {
	B_FLAVOR_IS_GLOBAL	= 0x100000L,		// force in media_addon_server,
											// only one instance
	B_FLAVOR_IS_LOCAL	= 0x200000L			// force in loading app, many
											// instances if none is set, could
											// go either way
};

struct flavor_info {
	char*				name;
	char*				info;
	uint64				kinds;				// node kind
	uint32				flavor_flags;
	int32				internal_id;		// For BMediaAddOn internal use
	int32				possible_count;		// 0 for "any number"

	int32				in_format_count;	// for BufferConsumer kinds
	uint32				in_format_flags;	// set to 0
	const media_format*	in_formats;

	int32				out_format_count;	// for BufferProducer kinds
	uint32				out_format_flags;	// set to 0
	const media_format*	out_formats;

	uint32				_reserved_[16];

private:
	flavor_info&		operator=(const flavor_info& other);
};

struct dormant_flavor_info : public flavor_info, public BFlattenable {
								dormant_flavor_info();
	virtual						~dormant_flavor_info();

								dormant_flavor_info(
									const dormant_flavor_info& other);
			dormant_flavor_info& operator=(const dormant_flavor_info& other);
			dormant_flavor_info& operator=(const flavor_info& other);

			dormant_node_info	node_info;

			void				set_name(const char* name);
			void				set_info(const char* info);
			void				add_in_format(const media_format& format);
			void				add_out_format(const media_format& format);

	virtual	bool				IsFixedSize() const;
	virtual	type_code			TypeCode() const;
	virtual	ssize_t				FlattenedSize() const;
	virtual	status_t			Flatten(void* buffer, ssize_t size) const;
	virtual	status_t			Unflatten(type_code type, const void* buffer,
									ssize_t size);
};


namespace BPrivate {
	namespace media {
		class DormantNodeManager;
	};
};


//! a MediaAddOn is something which can manufacture MediaNodes
class BMediaAddOn {
public:
	explicit					BMediaAddOn(image_id image);
	virtual						~BMediaAddOn();

	virtual	status_t			InitCheck(const char** _failureText);
	virtual	int32				CountFlavors();
	virtual	status_t			GetFlavorAt(int32 index,
									const flavor_info** _info);
	virtual	BMediaNode*			InstantiateNodeFor(const flavor_info* info,
									BMessage* config, status_t* _error);
	virtual	status_t			GetConfigurationFor(BMediaNode* yourNode,
									BMessage* intoMessage);
	virtual	bool				WantsAutoStart();
	virtual	status_t			AutoStart(int index, BMediaNode** _node,
									int32* _internalID, bool* _hasMore);

	// NOTE: Only implement if you have a B_FILE_INTERFACE node
	virtual	status_t			SniffRef(const entry_ref& file,
									BMimeType* ioMimeType, float* _quality,
									int32* _internalID);
	// NOTE: This is broken if you deal with producers and consumers both.
	// Implement SniffTypeKind instead. If you implement SniffTypeKind, this
	// doesn't get called.
	virtual	status_t			SniffType(const BMimeType& type,
									float* _quality, int32* _internalID);

	virtual	status_t			GetFileFormatList(int32 forNodeFlavorID,
									media_file_format* _writableFormats,
									int32 writableFormatsCount,
									int32* _writableFormatsTotalCount,
									media_file_format* _readableFormats,
									int32 readableFormatsCount,
									int32* _readableFormatsTotalCount,
									void* _reserved);

	// NOTE: Like SniffType, but for the specific kind(s)
	virtual	status_t			SniffTypeKind(const BMimeType& type,
									uint64 kinds, float* _quality,
									int32* _internalID, void* _reserved);

			image_id			ImageID();
			media_addon_id		AddonID();

protected:
	// Calling this will cause everyone to get notified, and also
	// cause the server to re-scan your flavor info. It is thread safe.
			status_t			NotifyFlavorChange();

	// TODO: Needs a Perform() virtual method!

private:
	// FBC padding and forbidden methods
								BMediaAddOn();
								BMediaAddOn(const BMediaAddOn& other);
			BMediaAddOn&		operator=(const BMediaAddOn& other);

			friend class BPrivate::media::DormantNodeManager;

	virtual	status_t			_Reserved_MediaAddOn_2(void*);
	virtual	status_t			_Reserved_MediaAddOn_3(void*);
	virtual	status_t			_Reserved_MediaAddOn_4(void*);
	virtual	status_t			_Reserved_MediaAddOn_5(void*);
	virtual	status_t			_Reserved_MediaAddOn_6(void*);
	virtual	status_t			_Reserved_MediaAddOn_7(void*);

			image_id 			fImage;
			media_addon_id		fAddon;

			uint32				_reserved_media_add_on_[7];
};


#if BUILDING_MEDIA_ADDON
	extern "C" _EXPORT BMediaAddOn* make_media_addon(image_id yourImage);
#endif


#endif // _MEDIA_ADD_ON_H

