/***********************************************************************
 * AUTHOR: Marcus Overhagen
 *   FILE: MediaAddOn.cpp
 *  DESCR: 
 ***********************************************************************/
#include <MediaAddOn.h>
#include <string.h>
#include <malloc.h>
#include "debug.h"
#include "PortPool.h"
#include "../server/headers/ServerInterface.h"

/*
 * some little helper function
 */

static inline char *newstrdup(const char *str);
char *newstrdup(const char *str)
{
	if (str == NULL)
		return NULL;
	int len = strlen(str) + 1;
	char *p = new char[len];
	memcpy(p,str,len);
	return p;
}

#define FLATTEN_MAGIC 		'CODE'
#define FLATTEN_TYPECODE	'DFIT'

/*************************************************************
 * public dormant_node_info
 *************************************************************/

// final & verified
dormant_node_info::dormant_node_info()
	: addon(-1),
	flavor_id(-1)
{
	CALLED();
	name[0] = '\0';
}

// final
dormant_node_info::~dormant_node_info()
{
	CALLED();
}

/*************************************************************
 * private flavor_info
 *************************************************************/

/* DO NOT IMPLEMENT */
/*
flavor_info &flavor_info::operator=(const flavor_info &other)
*/

/*************************************************************
 * public dormant_flavor_info
 *************************************************************/

// final & verified
dormant_flavor_info::dormant_flavor_info()
{
	CALLED();
	name = 0;
	info = 0;
	kinds = 0;
	flavor_flags = 0;
	internal_id = 0;
	possible_count = 0;
	in_format_count = 0;
	in_format_flags = 0;
	in_formats = 0;
	out_format_count = 0;
	out_format_flags = 0;
	out_formats = 0;
}


/* virtual */ 
dormant_flavor_info::~dormant_flavor_info()
{
	CALLED();
	delete [] name;
	delete [] info;
	delete [] in_formats;
	delete [] out_formats;
}


dormant_flavor_info::dormant_flavor_info(const dormant_flavor_info &clone)
{
	*this = clone;
}


dormant_flavor_info &
dormant_flavor_info::operator=(const dormant_flavor_info &clone)
{
	CALLED();
	//*this = static_cast<const flavor_info>(clone);
	*this = (const flavor_info)clone;
	node_info = clone.node_info;
	return *this;
}


dormant_flavor_info &
dormant_flavor_info::operator=(const flavor_info &clone)
{
	CALLED();
	delete [] name;
	delete [] info;
	delete [] in_formats;
	delete [] out_formats;

	name = newstrdup(clone.name);
	info = newstrdup(clone.info);
	
	kinds = clone.kinds;
	flavor_flags = clone.flavor_flags;
	internal_id = clone.internal_id;
	possible_count = clone.possible_count;

	in_format_count = clone.in_format_count;
	in_format_flags = clone.in_format_flags;
	out_format_count = clone.out_format_count;
	out_format_flags = clone.out_format_flags;

	if (in_format_count > 0) {
		media_format *temp;
		temp = new media_format[in_format_count];
		for (int i = 0; i < in_format_count; i++)
			temp[i] = clone.in_formats[i];
		in_formats = temp;
	} else {
		in_formats = 0;
	}
		
	if (out_format_count > 0) {
		media_format *temp;
		temp = new media_format[out_format_count];
		for (int i = 0; i < out_format_count; i++)
			temp[i] = clone.out_formats[i];
		out_formats = temp;
	} else {
		out_formats = 0;
	}
	
	dormant_node_info temp;
	node_info = temp;
	
	return *this;
}


void
dormant_flavor_info::set_name(const char *in_name)
{
	CALLED();
	delete [] name;
	name = newstrdup(in_name);
}


void
dormant_flavor_info::set_info(const char *in_info)
{
	CALLED();
	delete [] info;
	info = newstrdup(in_info);
}


void
dormant_flavor_info::add_in_format(const media_format &in_format)
{
	CALLED();
	media_format *temp;
	temp = new media_format[in_format_count + 1];
	for (int i = 0; i < in_format_count; i++)
		temp[i] = in_formats[i];
	temp[in_format_count] = in_format;
	delete [] in_formats;
	in_format_count += 1;
	in_formats = temp;
}


void
dormant_flavor_info::add_out_format(const media_format &out_format)
{
	CALLED();
	media_format *temp;
	temp = new media_format[out_format_count + 1];
	for (int i = 0; i < out_format_count; i++)
		temp[i] = out_formats[i];
	temp[out_format_count] = out_format;
	delete [] out_formats;
	out_format_count += 1;
	out_formats = temp;
}


/* virtual */ bool
dormant_flavor_info::IsFixedSize() const
{
	CALLED();
	return false;
}


/* virtual */ type_code
dormant_flavor_info::TypeCode() const
{
	CALLED();
	return FLATTEN_TYPECODE;
}


/* virtual */ ssize_t
dormant_flavor_info::FlattenedSize() const
{
	ssize_t size = 0;
	// magic
	size += sizeof(int32);
	// size
	size += sizeof(int32);
	// struct flavor_info
	size += sizeof(int32) + strlen(name);
	size += sizeof(int32) + strlen(info);
	size += sizeof(kinds);
	size += sizeof(flavor_flags);
	size += sizeof(internal_id);
	size += sizeof(possible_count);
	size += sizeof(in_format_count);
	size += sizeof(in_format_flags);
	size += in_format_count * sizeof(media_format);
	size += sizeof(out_format_count);
	size += sizeof(out_format_flags);
	size += out_format_count * sizeof(media_format);
	// struct dormant_node_info	node_info
	size += sizeof(node_info);

	return size;
}


/* virtual */ status_t
dormant_flavor_info::Flatten(void *buffer,
							 ssize_t size) const
{
	CALLED();
	if (size < FlattenedSize())
		return B_ERROR;

	char *buf = (char *)buffer;	
	int32 namelen = name ? (int32)strlen(name) : -1;
	int32 infolen = info ? (int32)strlen(info) : -1;

	// magic
	*(int32*)buf = FLATTEN_MAGIC; buf += sizeof(int32);

	// size
	*(int32*)buf = FlattenedSize(); buf += sizeof(int32);

	// struct flavor_info
	*(int32*)buf = namelen; buf += sizeof(int32);
	if (namelen > 0) {
		memcpy(buf,name,namelen);
		buf += namelen;
	}
	*(int32*)buf = infolen; buf += sizeof(int32);
	if (infolen > 0) {
		memcpy(buf,info,infolen);
		buf += infolen;
	}

	*(uint64*)buf = kinds; buf += sizeof(uint64);
	*(uint32*)buf = flavor_flags; buf += sizeof(uint32);
	*(int32*)buf = internal_id; buf += sizeof(int32);
	*(int32*)buf = possible_count; buf += sizeof(int32);
	*(int32*)buf = in_format_count; buf += sizeof(int32);
	*(uint32*)buf = in_format_flags; buf += sizeof(uint32);

	// XXX FIXME! we should not!!! make flat copies	of media_format
	memcpy(buf,in_formats,in_format_count * sizeof(media_format)); buf += in_format_count * sizeof(media_format);

	*(int32*)buf = out_format_count; buf += sizeof(int32);
	*(uint32*)buf = out_format_flags; buf += sizeof(uint32);

	// XXX FIXME! we should not!!! make flat copies	of media_format
	memcpy(buf,out_formats,out_format_count * sizeof(media_format)); buf += out_format_count * sizeof(media_format);

	*(dormant_node_info*)buf = node_info; buf += sizeof(dormant_node_info);

	return B_OK;
}


/* virtual */ status_t
dormant_flavor_info::Unflatten(type_code c,
							   const void *buffer,
							   ssize_t size)
{
	CALLED();
	if (c != FLATTEN_TYPECODE)
		return B_ERROR;
	if (size < 8)
		return B_ERROR;

	const char *buf = (const char *)buffer;	
	int32 namelen;
	int32 infolen;

	// magic
	if (*(int32*)buf != FLATTEN_MAGIC)
		return B_ERROR;
	buf += sizeof(int32);

	// size
	if (*(int32*)buf > size)
		return B_ERROR;
	buf += sizeof(int32);


	delete [] name;
	delete [] info;
	delete [] in_formats;
	delete [] out_formats;
	name = 0;
	info = 0;
	in_formats = 0;
	out_formats = 0;


	// struct flavor_info
	namelen = *(int32*)buf; buf += sizeof(int32);
	if (namelen >= 0) { // if namelen is -1, we leave name = 0
		name = new char [namelen + 1];
		memcpy(name,buf,namelen);
		name[namelen] = 0;
		buf += namelen;
	}

	infolen = *(int32*)buf; buf += sizeof(int32);
	if (infolen >= 0) { // if infolen is -1, we leave info = 0
		info = new char [infolen + 1];
		memcpy(info,buf,infolen);
		info[infolen] = 0;
		buf += infolen;
	}
	
	kinds = *(uint64*)buf; buf += sizeof(uint64);
	flavor_flags = *(uint32*)buf; buf += sizeof(uint32);
	internal_id = *(int32*)buf; buf += sizeof(int32);
	possible_count = *(int32*)buf; buf += sizeof(int32);
	in_format_count = *(int32*)buf; buf += sizeof(int32);
	in_format_flags = *(uint32*)buf; buf += sizeof(uint32);

	// XXX FIXME! we should not!!! make flat copies	of media_format
	if (in_format_count > 0) {
		in_formats = new media_format[in_format_count];
		memcpy((media_format *)in_formats,buf,in_format_count * sizeof(media_format)); 
		buf += in_format_count * sizeof(media_format);
	}
	
	out_format_count = *(int32*)buf; buf += sizeof(int32);
	out_format_flags = *(uint32*)buf; buf += sizeof(uint32);

	// XXX FIXME! we should not!!! make flat copies	of media_format
	if (out_format_count > 0) {
		out_formats = new media_format[out_format_count];
		memcpy((media_format *)out_formats,buf,out_format_count * sizeof(media_format)); 
		buf += out_format_count * sizeof(media_format);
	}

	node_info = *(dormant_node_info*)buf; buf += sizeof(dormant_node_info);
	
	return B_OK;
}

/*************************************************************
 * public BMediaAddOn
 *************************************************************/

/* explicit */ 
BMediaAddOn::BMediaAddOn(image_id image) :
	_fImage(image),
	_fAddon(0)
{
	CALLED();
	xfer_server_register_mediaaddon msg;
	xfer_server_register_mediaaddon_reply reply;
	port_id port;
	status_t rv;
	int32 code;
	port = find_port("media_server port");
	if (port <= B_OK)
			return;
	msg.reply_port = _PortPool->GetPort();
	rv = write_port(port, SERVER_REGISTER_MEDIAADDON, &msg, sizeof(msg));
	if (rv != B_OK) {
		_PortPool->PutPort(msg.reply_port);
		return;
	}
	rv = read_port(msg.reply_port, &code, &reply, sizeof(reply));
	_PortPool->PutPort(msg.reply_port);
	if (rv < B_OK)
		return;
	_fAddon = reply.addonid;
}


/* virtual */ 
BMediaAddOn::~BMediaAddOn()
{
	CALLED();
	if (_fAddon != 0) {
		xfer_server_unregister_mediaaddon msg;
		port_id port;
		port = find_port("media_server port");
		if (port <= B_OK)
			return;
		msg.addonid = _fAddon;
		write_port(port, SERVER_UNREGISTER_MEDIAADDON, &msg, sizeof(msg));
	}
}


/* virtual */ status_t
BMediaAddOn::InitCheck(const char **out_failure_text)
{
	CALLED();
	// only to be implemented by derived classes
	*out_failure_text = "no error";
	return B_OK;
}


/* virtual */ int32
BMediaAddOn::CountFlavors()
{
	CALLED();
	// only to be implemented by derived classes
	return 0;
}


/* virtual */ status_t
BMediaAddOn::GetFlavorAt(int32 n,
						 const flavor_info **out_info)
{
	CALLED();
	// only to be implemented by derived classes
	return B_ERROR;
}


/* virtual */ BMediaNode *
BMediaAddOn::InstantiateNodeFor(const flavor_info *info,
								BMessage *config,
								status_t *out_error)
{
	CALLED();
	// only to be implemented by derived classes
	return NULL;
}


/* virtual */ status_t
BMediaAddOn::GetConfigurationFor(BMediaNode *your_node,
								 BMessage *into_message)
{
	CALLED();
	// only to be implemented by derived classes
	return B_ERROR;
}


/* virtual */ bool
BMediaAddOn::WantsAutoStart()
{
	CALLED();
	// only to be implemented by derived classes
	return false;
}


/* virtual */ status_t
BMediaAddOn::AutoStart(int in_count,
					   BMediaNode **out_node,
					   int32 *out_internal_id,
					   bool *out_has_more)
{
	CALLED();
	// only to be implemented by derived classes
	return B_ERROR;
}


/* virtual */ status_t
BMediaAddOn::SniffRef(const entry_ref &file,
					  BMimeType *io_mime_type,
					  float *out_quality,
					  int32 *out_internal_id)
{
	CALLED();
	// only to be implemented by BFileInterface derived classes
	return B_ERROR;
}


/* virtual */ status_t
BMediaAddOn::SniffType(const BMimeType &type,
					   float *out_quality,
					   int32 *out_internal_id)
{
	CALLED();
	// only to be implemented by BFileInterface derived classes
	return B_ERROR;
}


/* virtual */ status_t
BMediaAddOn::GetFileFormatList(int32 flavor_id,
							   media_file_format *out_writable_formats,
							   int32 in_write_items,
							   int32 *out_write_items,
							   media_file_format *out_readable_formats,
							   int32 in_read_items,
							   int32 *out_read_items,
							   void *_reserved)
{
	CALLED();
	// only to be implemented by BFileInterface derived classes
	return B_ERROR;
}


/* virtual */ status_t
BMediaAddOn::SniffTypeKind(const BMimeType &type,
						   uint64 in_kinds,
						   float *out_quality,
						   int32 *out_internal_id,
						   void *_reserved)
{
	CALLED();
	// only to be implemented by BFileInterface derived classes
	return B_ERROR;
}


image_id
BMediaAddOn::ImageID()
{
	return _fImage;
}


media_addon_id
BMediaAddOn::AddonID()
{
	return _fAddon;
}

/*************************************************************
 * protected BMediaAddOn
 *************************************************************/

status_t
BMediaAddOn::NotifyFlavorChange()
{
	if (_fAddon == 0)
		return B_ERROR;
	
	port_id port;
	port = find_port("media_addon_server port");
	if (port <= B_OK)
		return B_ERROR;

	xfer_addonserver_rescan_mediaaddon_flavors msg;
	msg.addonid = _fAddon;
	return write_port(port, ADDONSERVER_RESCAN_MEDIAADDON_FLAVORS, &msg, sizeof(msg));
}

/*************************************************************
 * private BMediaAddOn
 *************************************************************/

/*
unimplemented:
BMediaAddOn::BMediaAddOn()
BMediaAddOn::BMediaAddOn(const BMediaAddOn &clone)
BMediaAddOn & BMediaAddOn::operator=(const BMediaAddOn &clone)
*/


/*************************************************************
 * private BMediaAddOn
 *************************************************************/

extern "C" {
	// declared here to remove them from the class header file
	status_t _Reserved_MediaAddOn_0__11BMediaAddOnPv(void *, void *); /* now used for BMediaAddOn::GetFileFormatList */
	status_t _Reserved_MediaAddOn_1__11BMediaAddOnPv(void *, void *); /* now used for BMediaAddOn::SniffTypeKind */
	status_t _Reserved_MediaAddOn_0__11BMediaAddOnPv(void *, void *) { return B_ERROR; }
	status_t _Reserved_MediaAddOn_1__11BMediaAddOnPv(void *, void *) { return B_ERROR; }
};

status_t BMediaAddOn::_Reserved_MediaAddOn_2(void *) { return B_ERROR; }
status_t BMediaAddOn::_Reserved_MediaAddOn_3(void *) { return B_ERROR; }
status_t BMediaAddOn::_Reserved_MediaAddOn_4(void *) { return B_ERROR; }
status_t BMediaAddOn::_Reserved_MediaAddOn_5(void *) { return B_ERROR; }
status_t BMediaAddOn::_Reserved_MediaAddOn_6(void *) { return B_ERROR; }
status_t BMediaAddOn::_Reserved_MediaAddOn_7(void *) { return B_ERROR; }

