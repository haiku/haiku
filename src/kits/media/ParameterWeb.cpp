/* ParameterWeb - implements the following classes:
**		BParameterWeb, BParameterGroup, BParameter, BNullParameter,
**		BContinuousParameter, BDiscreteParameter
**
** Author: Zousar Shaker
**         Axel Dörfler, axeld@pinc-software.de
**         Marcus Overhagen
**
** This file may be used under the terms of the OpenBeOS License.
*/


#include <ParameterWeb.h>
#include <MediaNode.h>
#include <string.h>
#include "DataExchange.h"
#include "MediaMisc.h"


#include "debug.h"


/*
	The following is documentation on the flattened format
	of structures/classes in this module:
	
	//--------BEGIN-CORE-BPARAMETER-STRUCT---------------------
	?? (0x02040607): 4 bytes
	BParameter Struct Size (in bytes): 4 bytes
	ID: 4 bytes
	Name String Length: 1 byte (??)
		Name String: 'Name String Length' bytes
	Kind String Length: 1 byte (??)
		Kind String: 'Kind String Length' bytes
	Unit String Length: 1 byte (??)
		Unit String: 'Unit String Length' bytes
	Inputs Count: 4 bytes
		Inputs (pointers): ('Inputs Count')*4 bytes
	Outputs Count: 4 bytes
		Outputs (pointers): ('Outputs Count')*4 bytes
	Media Type: 4 bytes
	ChannelCount: 4 bytes
	Flags: 4 bytes
	//---------END-CORE-BPARAMETER-STRUCT-----------------------
	//--------BEGIN-BCONTINUOUSPARAMETER-STRUCT---------
	Min: 4 bytes (as float)
	Max: 4 bytes (as float)
	Stepping: 4 bytes (as float)
	Response: 4 bytes (as int or enum)
	Factor: 4 bytes (as float)
	Offset: 4 bytes (as float)
	//--------END-BCONTINUOUSPARAMETER-STRUCT-------------
	//--------BEGIN-BDISCRETEPARAMETER-STRUCT----------------
	NumItems: 4 bytes (as int)
		//for each item BEGIN
		Item Name String Length: 1 byte
			Item Name String: 'Item Name String Length' bytes
		Item Value: 4 bytes (as int)
		//for each item END
	//--------END-BDISCRETEPARAMETER-STRUCT-------------------
	
	//--------BEGIN-CORE-BPARAMETERGROUP-STRUCT-----------
	?? (0x03040507 OR 0x03040509 depending if the flags field is included or not???): 4 bytes
	(possible) Flags: 4 bytes
	Name String Length: 1 byte (??)
		Name String: 'Name String Length' bytes
	Param Count: 4 bytes
		//for each Param BEGIN
		Pointer: 4 bytes
		Parameter Type: 4 bytes
		Flattened Parameter Size: 4 bytes
		Flattened Parameter: 'Flattened Parameter Size' bytes
		//for each Param END
	Subgroup Count: 4 bytes
		//for each SubGroup BEGIN
		Pointer: 4 bytes
		MEDIA PARAMETER GROUP TYPE('BMCG' (opposite byte order in file)): 4 bytes
		Flattened Group Size: 4 bytes
		Flattened Group: 'Flattened Group Size' bytes
		//for each SubGroup END
	
	//---------END-CORE-BPARAMETERGROUP-STRUCT--------------
	
	//--------BEGIN-CORE-BPARAMETERWEB-STRUCT-----------
	?? 0x01030506: 4 bytes
	??: 4 bytes (is always 1)
	Group Count: 4 bytes
	Node (as media_node): 0x18 bytes (decimal 24 bytes)
		//for each Group BEGIN
		Flattened Group Size: 4 bytes
		Flattened Group: 'Flattened Group Size' bytes
		//for each Group END
		//for each Group BEGIN
		??: 4 bytes (never get written to (holds uninitialized value))
		//for each Group END
	//---------END-CORE-BPARAMETERWEB-STRUCT--------------
	
*/


const char * const B_GENERIC			 = "";
const char * const B_MASTER_GAIN		 = "Master";
const char * const B_GAIN				 = "Gain";
const char * const B_BALANCE			 = "Balance";
const char * const B_FREQUENCY			 = "Frequency";
const char * const B_LEVEL				 = "Level";
const char * const B_SHUTTLE_SPEED		 = "Speed";
const char * const B_CROSSFADE			 = "XFade";
const char * const B_EQUALIZATION		 = "EQ";
const char * const B_COMPRESSION		 = "Compression";
const char * const B_QUALITY			 = "Quality";
const char * const B_BITRATE			 = "Bitrate";
const char * const B_GOP_SIZE			 = "GOPSize";
const char * const B_MUTE				 = "Mute";
const char * const B_ENABLE				 = "Enable";
const char * const B_INPUT_MUX			 = "Input";
const char * const B_OUTPUT_MUX			 = "Output";
const char * const B_TUNER_CHANNEL		 = "Channel";
const char * const B_TRACK				 = "Track";
const char * const B_RECSTATE			 = "RecState";
const char * const B_SHUTTLE_MODE		 = "Shuttle";
const char * const B_RESOLUTION			 = "Resolution";
const char * const B_COLOR_SPACE		 = "Colorspace";
const char * const B_FRAME_RATE			 = "FrameRate";
const char * const B_VIDEO_FORMAT		 = "VideoFormat";
const char * const B_WEB_PHYSICAL_INPUT	 = "PhysInput";
const char * const B_WEB_PHYSICAL_OUTPUT = "PhysOutput";
const char * const B_WEB_ADC_CONVERTER	 = "ADC";
const char * const B_WEB_DAC_CONVERTER	 = "DAC";
const char * const B_WEB_LOGICAL_INPUT	 = "LogInput";
const char * const B_WEB_LOGICAL_OUTPUT	 = "LogOutput";
const char * const B_WEB_LOGICAL_BUS	 = "LogBus";
const char * const B_WEB_BUFFER_INPUT	 = "DataInput";
const char * const B_WEB_BUFFER_OUTPUT	 = "DataOutput";
const char * const B_SIMPLE_TRANSPORT	 = "SimpleTransport";

// Flattened data

static const int32 kCurrentParameterWebVersion = 1;
static const uint32 kParameterWebMagic = 0x01030506;
static const uint32 kBufferGroupMagic = 0x03040509;
static const uint32 kBufferGroupMagicNoFlags = 0x03040507;
static const uint32 kParameterMagic = 0x02040607;

static const ssize_t kAdditionalParameterGroupSize = 12;
static const ssize_t kAdditionalParameterSize = 35;

/* BContinuousParameter - FlattenedSize() fixed part
 *	Min: 4 bytes (as float)
 *	Max: 4 bytes (as float)
 *	Stepping: 4 bytes (as float)
 *	Response: 4 bytes (as int or enum)
 *	Factor: 4 bytes (as float)
 *	Offset: 4 bytes (as float)
 */
static const ssize_t kAdditionalContinuousParameterSize = 24;
static const ssize_t kAdditionalDiscreteParameterSize = sizeof(ssize_t);


// helper functions


template<class Type> Type
read_from_buffer(const void **_buffer)
{
	const Type *typedBuffer = static_cast<const Type *>(*_buffer);
	Type value = *typedBuffer;

	typedBuffer++;
	*_buffer = static_cast<const void *>(typedBuffer);

	return value;
}


static status_t
read_string_from_buffer(const void **_buffer, char **_string, ssize_t size)
{
	if (size < 1)
		return B_BAD_VALUE;
	
	const uint8 *buffer = static_cast<const uint8 *>(*_buffer);
	uint8 length = *buffer++;
	if (length > size - 1)
		return B_BAD_VALUE;

	char *string = (char *)malloc(length + 1);
	if (string == NULL)
		return B_NO_MEMORY;

	memcpy(string, buffer, length);
	string[length] = '\0';

	*_buffer = static_cast<const void *>(buffer + length);
	*_string = string;
	return B_OK;
}


// currently unused
#if 0
template<class Type> Type *
reserve_in_buffer(void **_buffer)
{
	Type *typedBuffer = static_cast<Type *>(*_buffer);

	*typedBuffer = 0;
	typedBuffer++;

	*_buffer = static_cast<void *>(typedBuffer);
}
#endif

template<class Type> void
write_to_buffer(void **_buffer, Type value)
{
	Type *typedBuffer = static_cast<Type *>(*_buffer);

	*typedBuffer = value;
	typedBuffer++;

	*_buffer = static_cast<void *>(typedBuffer);
}


void
write_string_to_buffer(void **_buffer, const char *string)
{
	uint8 *buffer = static_cast<uint8 *>(*_buffer);
	uint32 length = string ? strlen(string) : 0;
	if (length > 255)
		length = 255;

	*buffer++ = static_cast<uint8>(length);
	if (length) {
		memcpy(buffer, string, length);
		buffer += length;
	}

	*_buffer = static_cast<void *>(buffer);
}


static void
skip_in_buffer(const void **_buffer, uint32 bytes)
{
	const uint8 *buffer = static_cast<const uint8 *>(*_buffer);

	buffer += bytes;

	*_buffer = static_cast<const void *>(buffer);
}


static void inline
skip_in_buffer(void **_buffer, uint32 bytes)
{
	// This actually shouldn't be necessary, but I believe it's a
	// bug in Be's gcc - it complains about "adds cv-quals without intervening `const'"
	// when passing a "const void **" pointer as the first argument.
	// Maybe I am just stupid, though -- axeld.

	skip_in_buffer((const void **)_buffer, bytes);
}


template<class Type> Type
swap32(Type value, bool doSwap)
{
	STATIC_ASSERT(sizeof(Type) == 4);

	if (doSwap)
		return (Type)B_SWAP_INT32((int32)value);

	return value;
}


template<class Type> Type
read_from_buffer_swap32(const void **_buffer, bool doSwap)
{
	return swap32<Type>(read_from_buffer<Type>(_buffer), doSwap);
}


static inline ssize_t
size_left(ssize_t size, const void *bufferStart, const void *buffer)
{
	return size - static_cast<ssize_t>((const uint8 *)buffer - (const uint8 *)bufferStart);
}


/** Copies the source string into a new buffer with the specified
 *	maximum size. This always includes a trailing zero byte.
 *	Returns NULL if the source string is either NULL or empty, or
 *	if there wasn't enough memory to allocate the buffer.
 *	The buffer returned is to be freed by the caller using free().
 */

static char *
strndup(const char *source, size_t maxBufferSize)
{
	if (source == NULL || source[0] == '\0')
		return NULL;

	uint32 size = strlen(source) + 1;
	if (size > maxBufferSize)
		size = maxBufferSize;

	char *target = (char *)malloc(size);
	if (target == NULL)
		return NULL;
	
	memcpy(target, source, size - 1);
	target[size - 1] = '\0';

	return target;
}


//	#pragma mark -

/*************************************************************
 * public BParameterWeb
 *************************************************************/


BParameterWeb::BParameterWeb()
	:	mNode(media_node::null) // mNode is set in BControllable::SetParameterWeb()
{
	CALLED();

	mGroups = new BList();

	mOldRefs = new BList();
	mNewRefs = new BList();

	ASSERT(mGroups != NULL && mOldRefs != NULL && mNewRefs != NULL);
}


BParameterWeb::~BParameterWeb()
{
	CALLED();

	if (mGroups != NULL) {
		for(int32 i = mGroups->CountItems(); i-- > 0;) {
			delete static_cast<BParameterGroup *>(mGroups->ItemAt(i));
		}

		delete mGroups;
	}

	delete mOldRefs;
	delete mNewRefs;
}


media_node
BParameterWeb::Node()
{
	return mNode;
}


BParameterGroup *
BParameterWeb::MakeGroup(const char *name)
{
	CALLED();
	ASSERT(mGroups != NULL);

	BParameterGroup *group = new BParameterGroup(this, name);
	mGroups->AddItem(group);

	return group;
}


int32
BParameterWeb::CountGroups()
{
	ASSERT(mGroups != NULL);

	return mGroups->CountItems();
}


BParameterGroup *
BParameterWeb::GroupAt(int32 index)
{
	ASSERT(mGroups != NULL);

	return static_cast<BParameterGroup *>(mGroups->ItemAt(index));
}


int32
BParameterWeb::CountParameters()
{
	CALLED();
	ASSERT(mGroups != NULL);

	// Counts over all groups (and sub-groups) in the web.
	// The "groups" list is used as count stack

	BList groups(*mGroups);
	int32 count = 0;

	for (int32 i = 0; i < groups.CountItems(); i++) {
		BParameterGroup *group = static_cast<BParameterGroup *>(groups.ItemAt(i));

		count += group->CountParameters();

		if (group->mGroups != NULL)
			groups.AddList(group->mGroups);
	}

	return count;
}


BParameter *
BParameterWeb::ParameterAt(int32 index)
{
	CALLED();
	ASSERT(mGroups != NULL);
	
	// Iterates over all groups (and sub-groups) in the web.
	// The "groups" list is used as iteration stack (breadth search style)
	// Maintains the same order as the Be implementation

	BList groups(*mGroups);

	for (int32 i = 0; i < groups.CountItems(); i++) {
		BParameterGroup *group = static_cast<BParameterGroup *>(groups.ItemAt(i));
		int32 count = group->CountParameters();
		if (index < count)
			return group->ParameterAt(index);

		index -= count;
			// the index is always relative to the start of the current group

		if (group->mGroups != NULL)
			groups.AddList(group->mGroups);
	}

	TRACE("*** could not find parameter at %ld (count = %ld)\n", index, CountParameters());
	return NULL;
}


bool
BParameterWeb::IsFixedSize() const
{
	return false;
}


type_code
BParameterWeb::TypeCode() const
{
	return B_MEDIA_PARAMETER_WEB_TYPE;
}


ssize_t
BParameterWeb::FlattenedSize() const
{
	CALLED();

/*
	//--------BEGIN-CORE-BPARAMETERWEB-STRUCT-----------
	?? 0x01030506: 4 bytes
	??: 4 bytes (is always 1)
	Group Count: 4 bytes
	Node (as media_node): 0x18 bytes (decimal 24 bytes)
		//for each Group BEGIN
		Flattened Group Size: 4 bytes
		Flattened Group: 'Flattened Group Size' bytes
		//for each Group END
		//for each Group BEGIN
		??: 4 bytes (never get written to (holds uninitialized value))
		//for each Group END
	//---------END-CORE-BPARAMETERWEB-STRUCT--------------
*/
	//36 guaranteed bytes, variable after that.
	ssize_t size = sizeof(int32) + 2*sizeof(int32) + sizeof(media_node);

	for (int32 i = mGroups->CountItems(); i-- > 0;) {
		BParameterGroup *group = static_cast<BParameterGroup *>(mGroups->ItemAt(i));
		if (group != NULL) {
			size += 4 + group->FlattenedSize();
				// 4 bytes for the flattened size
		}
	}

	return size;
}


status_t
BParameterWeb::Flatten(void *buffer, ssize_t size) const
{
	CALLED();

	if (buffer == NULL)
		return B_NO_INIT;

	ssize_t actualSize = BParameterWeb::FlattenedSize();
	if (size < actualSize)
		return B_NO_MEMORY;

	void *bufferStart = buffer;

	write_to_buffer<int32>(&buffer, kParameterWebMagic);
	write_to_buffer<int32>(&buffer, kCurrentParameterWebVersion);

	// flatten all groups into this buffer

	int32 count = mGroups ? mGroups->CountItems() : 0;
	write_to_buffer<int32>(&buffer, count);

	write_to_buffer<media_node>(&buffer, mNode);

	for (int32 i = 0; i < count; i++) {
		BParameterGroup *group = static_cast<BParameterGroup *>(mGroups->ItemAt(i));
		if (group == NULL) {
			ERROR("BParameterWeb::Flatten(): group is NULL\n");
			continue;
		}

		ssize_t groupSize = group->FlattenedSize();
		if (groupSize > size_left(size, bufferStart, buffer)) {
			ERROR("BParameterWeb::Flatten(): buffer too small\n");
			return B_BAD_VALUE;
		}

		write_to_buffer<ssize_t>(&buffer, groupSize);

		// write the flattened sub group
		status_t status = group->Flatten(buffer, groupSize);
		if (status < B_OK)
			return status;

		skip_in_buffer(&buffer, groupSize);
	}

	return B_OK;
}


bool
BParameterWeb::AllowsTypeCode(type_code code) const
{
	return code == TypeCode();
}


status_t
BParameterWeb::Unflatten(type_code code, const void *buffer, ssize_t size)
{
	CALLED();

	if (!AllowsTypeCode(code)) {
		ERROR("BParameterWeb::Unflatten(): wrong type code\n");
		return B_BAD_TYPE;
	}

	if (buffer == NULL) {
		ERROR("BParameterWeb::Unflatten(): NULL buffer pointer\n");
		return B_NO_INIT;
	}

	// if the buffer is smaller than the size needed to read the
	// signature field, the mystery field, the group count, and the Node, then there is a problem
	if (size < static_cast<ssize_t>(sizeof(int32) + sizeof(int32) + sizeof(ssize_t) + sizeof(media_node)) )
	{
		ERROR("BParameterWeb::Unflatten(): size to small\n");
		return B_ERROR;
	}

	const void *bufferStart = buffer;

	uint32 magic = read_from_buffer<uint32>(&buffer);
	bool isSwapped = false;

	if (magic == B_SWAP_INT32(kParameterWebMagic)) {
		isSwapped = true;
		magic = B_SWAP_INT32(magic);
	}
	if (magic != kParameterWebMagic)
		return B_BAD_DATA;

	// Note, it's not completely sure that this field is the version
	// information - but it doesn't seem to have another purpose	
	int32 version = read_from_buffer_swap32<int32>(&buffer, isSwapped);
	if (version != kCurrentParameterWebVersion) {
		ERROR("BParameterWeb::Unflatten(): wrong version %ld (%lx)?!\n", version, version);
		return B_ERROR;
	}

#if 0	
	if(mGroups != NULL)
	{
		for (int32 i = 0; i < mGroups->CountItems(); i++)
		{
			BParameterGroup *CurrentItem = static_cast<BParameterGroup *>(mGroups->ItemAt(i));
			if(CurrentItem != NULL)
			{
				delete CurrentItem;
			}
		}
		mGroups->MakeEmpty();
	}
	else
	{
		mGroups = new BList();
	}
#endif

	int32 count = read_from_buffer_swap32<int32>(&buffer, isSwapped);

	mNode = read_from_buffer<media_node>(&buffer);
	if (isSwapped)
		swap_data(B_INT32_TYPE, &mNode, sizeof(media_node), B_SWAP_ALWAYS);

	for (int32 i = 0; i < count; i++) {
		ssize_t groupSize = read_from_buffer_swap32<ssize_t>(&buffer, isSwapped);
		if (groupSize > size_left(size, bufferStart, buffer)) {
			ERROR("BParameterWeb::Unflatten(): buffer too small\n");
			return B_BAD_DATA;
		}

		BParameterGroup *group = new BParameterGroup(this, "unnamed");
		status_t status = group->Unflatten(group->TypeCode(), buffer, groupSize);
		if (status < B_OK) {
			ERROR("BParameterWeb::Unflatten(): unflatten group failed\n");
			delete group;
			return status;
		}

		skip_in_buffer(&buffer, groupSize);

		mGroups->AddItem(group);
	}

	// fix all references (ParameterAt() style)

	if ((mOldRefs != NULL) && (mNewRefs != NULL)) {
		BList groups(*mGroups);
		
		for (int32 i = 0; i < groups.CountItems(); i++) {
			BParameterGroup *group = static_cast<BParameterGroup *>(groups.ItemAt(i));
			
			for (int32 index = group->CountParameters(); index-- > 0;) {
				BParameter *parameter = static_cast<BParameter *>(group->ParameterAt(index));

				parameter->FixRefs(*mOldRefs, *mNewRefs);
			}
			
			if (group->mGroups != NULL)
				groups.AddList(group->mGroups);
		}

		mOldRefs->MakeEmpty();
		mNewRefs->MakeEmpty();
	}

	return B_OK;
}

/*************************************************************
 * private BParameterWeb
 *************************************************************/

/*
unimplemented
BParameterWeb::BParameterWeb(const BParameterWeb &clone)
BParameterWeb &BParameterWeb::operator=(const BParameterWeb &clone)
*/


void
BParameterWeb::AddRefFix(void *oldItem, void *newItem)
{
	ASSERT(mOldRefs != NULL);
	ASSERT(mNewRefs != NULL);

	mOldRefs->AddItem(oldItem);
	mNewRefs->AddItem(newItem);
}


//	#pragma mark -

/*************************************************************
 * private BParameterGroup
 *************************************************************/


BParameterGroup::BParameterGroup(BParameterWeb *web, const char *name)
	:	mWeb(web),
	mFlags(0)
{
	CALLED();
	TRACE("BParameterGroup: web = %p, name = \"%s\"\n", web, name);

	mName = strndup(name, 256);

	mControls = new BList();
	mGroups = new BList();
}


BParameterGroup::~BParameterGroup()
{
	CALLED();

	if (mControls) {
		BParameter **items = reinterpret_cast<BParameter **>(mControls->Items());
		for (int i = mControls->CountItems(); i-- > 0;)
			delete items[i];

		delete mControls;
	}

	if (mGroups) {
		BParameterGroup **items = reinterpret_cast<BParameterGroup **>(mGroups->Items());
		for (int i = mGroups->CountItems(); i-- > 0;)
			delete items[i];

		delete mGroups;
	}

	free(mName);
}

/*************************************************************
 * public BParameterGroup
 *************************************************************/

BParameterWeb *
BParameterGroup::Web() const
{
	return mWeb;
}


const char *
BParameterGroup::Name() const
{
	return mName;
}


void
BParameterGroup::SetFlags(uint32 flags)
{
	mFlags = flags;
}


uint32
BParameterGroup::Flags() const
{
	return mFlags;
}


BNullParameter *
BParameterGroup::MakeNullParameter(int32 id, media_type mediaType, const char *name,
	const char *kind)
{
	CALLED();
	ASSERT(mControls != NULL);

	BNullParameter *parameter = new BNullParameter(id, mediaType, mWeb, name, kind);
	parameter->mGroup = this;
	mControls->AddItem(parameter);

	return parameter;
}


BTextParameter *
BParameterGroup::MakeTextParameter(int32 id, media_type mediaType, const char *name,
	const char *kind, size_t max_bytes)
{
	CALLED();
	ASSERT(mControls != NULL);

	BTextParameter *parameter = new BTextParameter(id, mediaType, mWeb, name, kind, max_bytes);
	parameter->mGroup = this;
	mControls->AddItem(parameter);

	return parameter;
}


BContinuousParameter *
BParameterGroup::MakeContinuousParameter(int32 id, media_type mediaType,
	const char *name, const char *kind, const char *unit,
	float minimum, float maximum, float stepping)
{
	CALLED();
	ASSERT(mControls != NULL);

	BContinuousParameter *parameter = new BContinuousParameter(id, mediaType, mWeb,
		name, kind, unit, minimum, maximum, stepping);

	parameter->mGroup = this;
	mControls->AddItem(parameter);

	return parameter;
}


BDiscreteParameter *
BParameterGroup::MakeDiscreteParameter(int32 id, media_type mediaType,
	const char *name, const char *kind)
{
	CALLED();
	ASSERT(mControls != NULL);

	BDiscreteParameter *parameter = new BDiscreteParameter(id, mediaType, mWeb, name, kind);
	if (parameter == NULL)
		return NULL;

	parameter->mGroup = this;
	mControls->AddItem(parameter);

	return parameter;
}


BParameterGroup *
BParameterGroup::MakeGroup(const char *name)
{
	CALLED();
	ASSERT(mGroups != NULL);

	BParameterGroup *group = new BParameterGroup(mWeb, name);
	if (group != NULL)
		mGroups->AddItem(group);

	return group;
}


int32
BParameterGroup::CountParameters()
{
	ASSERT(mControls != NULL);

	return mControls->CountItems();
}


BParameter *
BParameterGroup::ParameterAt(int32 index)
{
	ASSERT(mControls != NULL);

	return static_cast<BParameter *>(mControls->ItemAt(index));
}


int32
BParameterGroup::CountGroups()
{
	ASSERT(mGroups != NULL);

	return mGroups->CountItems();
}


BParameterGroup *
BParameterGroup::GroupAt(int32 index)
{
	ASSERT(mGroups != NULL);

	return static_cast<BParameterGroup *>(mGroups->ItemAt(index));
}


bool
BParameterGroup::IsFixedSize() const
{
	return false;
}


type_code
BParameterGroup::TypeCode() const
{
	return B_MEDIA_PARAMETER_GROUP_TYPE;
}


ssize_t
BParameterGroup::FlattenedSize() const
{
	CALLED();
	ASSERT(mControls != NULL);
	ASSERT(mGroups != NULL);

	/*
		//--------BEGIN-CORE-BPARAMETERGROUP-STRUCT-----------
		?? (0x03040507 OR 0x03040509 depending if the flags field is included or not???): 4 bytes
		(possible) Flags: 4 bytes
		Name String Length: 1 byte (??)
			Name String: 'Name String Length' bytes
		Param Count: 4 bytes
			//for each Param BEGIN
			Pointer: 4 bytes
			Parameter Type: 4 bytes
			Flattened Parameter Size: 4 bytes
			Flattened Parameter: 'Flattened Parameter Size' bytes
			//for each Param END
		Subgroup Count: 4 bytes
			//for each SubGroup BEGIN
			Pointer: 4 bytes
			MEDIA PARAMETER GROUP TYPE('BMCG' (opposite byte order in file)): 4 bytes
			Flattened Group Size: 4 bytes
			Flattened Group: 'Flattened Group Size' bytes
			//for each SubGroup END
		
		//---------END-CORE-BPARAMETERGROUP-STRUCT--------------
	*/
	//13 guaranteed bytes, variable after that.
	ssize_t size = 13;

	if(mFlags != 0)
	{
		size += 4;
	}
	
	if(mName != NULL)
	{
		size += min_c(strlen(mName),255);
	}
	
	int i;
	int limit;
	
	limit = mControls->CountItems();
	for(i = 0; i < limit; i++)
	{
		BParameter *CurrentParameter = static_cast<BParameter *>(mControls->ItemAt(i));
		if(CurrentParameter != NULL)
		{
			//overhead for each parameter flattened
			size += 16 + CurrentParameter->FlattenedSize();
		}
	}

	limit = mGroups->CountItems();
	for(i = 0; i < limit; i++)
	{
		BParameterGroup *CurrentGroup = static_cast<BParameterGroup *>(mGroups->ItemAt(i));
		if(CurrentGroup != NULL)
		{
			//overhead for each group flattened
			size += 16 + CurrentGroup->FlattenedSize();
		}
	}

	return size;
}


status_t
BParameterGroup::Flatten(void *buffer, ssize_t size) const
{
	CALLED();

	if (buffer == NULL) {
		ERROR("BParameterGroup::Flatten buffer is NULL\n");
		return B_NO_INIT;
	}

	// NOTICE: It is important that this value is the size returned by
	// BParameterGroup::FlattenedSize, not by a descendent's override of this method.
	ssize_t actualSize = BParameterGroup::FlattenedSize();
	if (size < actualSize) {
		ERROR("BParameterGroup::Flatten size to small\n");
		return B_NO_MEMORY;
	}

	if (mFlags != 0) {
		write_to_buffer<int32>(&buffer, kBufferGroupMagic);
		write_to_buffer<uint32>(&buffer, mFlags);
	} else
		write_to_buffer<int32>(&buffer, kBufferGroupMagicNoFlags);

	write_string_to_buffer(&buffer, mName);

	int32 count = mControls ? mControls->CountItems() : 0;
	write_to_buffer<int32>(&buffer, count);

	for (int32 i = 0; i < count; i++) {
		BParameter *parameter = static_cast<BParameter *>(mControls->ItemAt(i));
		if (parameter == NULL) {
			ERROR("BParameterGroup::Flatten(): NULL parameter\n");
			continue;
		}

		write_to_buffer<BParameter *>(&buffer, parameter);
		write_to_buffer<BParameter::media_parameter_type>(&buffer, parameter->Type());

		// flatten parameter into this buffer

		ssize_t parameterSize = parameter->FlattenedSize();
		write_to_buffer<ssize_t>(&buffer, parameterSize);

		status_t status = parameter->Flatten(buffer, parameterSize);
			// we have only that much bytes left to write in this buffer
		if (status < B_OK)
			return status;

		skip_in_buffer(&buffer, parameterSize);
	}

	count = mGroups ? mGroups->CountItems() : 0;
	write_to_buffer<int32>(&buffer, count);

	for (int32 i = 0; i < count; i++) {
		BParameterGroup *group = static_cast<BParameterGroup *>(mGroups->ItemAt(i));
		if (group == NULL) {
			ERROR("BParameterGroup::Flatten(): NULL group\n");
			continue;
		}

		write_to_buffer<BParameterGroup *>(&buffer, group);
		write_to_buffer<type_code>(&buffer, group->TypeCode());

		// flatten sub group into this buffer

		ssize_t groupSize = group->FlattenedSize();
		write_to_buffer<ssize_t>(&buffer, groupSize);

		status_t status = group->Flatten(buffer, groupSize);
			// we have only that much bytes left to write in this buffer
		if (status < B_OK)
			return status;

		skip_in_buffer(&buffer, groupSize);
	}

	return B_OK;
}


bool
BParameterGroup::AllowsTypeCode(type_code code) const
{
	return code == TypeCode();
}


status_t
BParameterGroup::Unflatten(type_code code, const void *buffer, ssize_t size)
{
	CALLED();

	if (!AllowsTypeCode(code)) {
		ERROR("BParameterGroup::Unflatten() wrong type code\n");
		return B_BAD_TYPE;
	}

	if (buffer == NULL) {
		ERROR("BParameterGroup::Unflatten() buffer is NULL\n");
		return B_NO_INIT;
	}

	// if the buffer is smaller than the size needed to read the
	// signature field, then there is a problem
	if (size < static_cast<ssize_t>(sizeof(int32))) {
		ERROR("BParameterGroup::Unflatten() size to small\n");
		return B_ERROR;
	}

	const void *bufferStart = buffer;
		// used to compute the rest length of the buffer when needed

	uint32 magic = read_from_buffer<uint32>(&buffer);
	bool isSwapped = false;

	if (magic == B_SWAP_INT32(kBufferGroupMagic)
		|| magic == B_SWAP_INT32(kBufferGroupMagicNoFlags)) {
		isSwapped = true;
		magic = B_SWAP_INT32(magic);
	}

	if (magic == kBufferGroupMagic)
		mFlags = read_from_buffer_swap32<int32>(&buffer, isSwapped);
	else if (magic == kBufferGroupMagicNoFlags)
		mFlags = 0;
	else
		return B_BAD_TYPE;

	if (read_string_from_buffer(&buffer, &mName,
			size - static_cast<ssize_t>((uint8 *)buffer - (uint8 *)bufferStart)) < B_OK)
		return B_BAD_VALUE;

	// Currently disabled since it's not really used -- axeld.
#if 0
	//Clear all existing parameters/subgroups
	int i;
	if(mControls != NULL)
	{
		for(i = 0; i < mControls->CountItems(); i++)
		{
			BParameter *CurrentItem = static_cast<BParameter *>(mControls->ItemAt(i));
			if(CurrentItem != NULL)
			{
				delete CurrentItem;
			}
		}
		mControls->MakeEmpty();
	}
	else
	{
		mControls = new BList();
	}
	
	if(mGroups != NULL)
	{
		for(i = 0; i < mGroups->CountItems(); i++)
		{
			BParameterGroup *CurrentItem = static_cast<BParameterGroup *>(mGroups->ItemAt(i));
			if(CurrentItem != NULL)
			{
				delete CurrentItem;
			}
		}
		mGroups->MakeEmpty();
	}
	else
	{
		mGroups = new BList();
	}
#endif

	// unflatten parameter list

	int32 count = read_from_buffer_swap32<int32>(&buffer, isSwapped);
	if (count < 0 || count * kAdditionalParameterSize > size_left(size, bufferStart, buffer))
		return B_BAD_VALUE;

	for (int32 i = 0; i < count; i++) {
		// make sure we can read as many bytes
		if (size_left(size, bufferStart, buffer) < 12)
			return B_BAD_VALUE;

		BParameter *oldPointer = read_from_buffer_swap32<BParameter *>(&buffer, isSwapped);
		BParameter::media_parameter_type mediaType =
			read_from_buffer_swap32<BParameter::media_parameter_type>(&buffer, isSwapped);

		ssize_t parameterSize = read_from_buffer_swap32<ssize_t>(&buffer, isSwapped);
		if (parameterSize > size_left(size, bufferStart, buffer))
			return B_BAD_VALUE;

		BParameter *parameter = MakeControl(mediaType);
		if (parameter == NULL) {
			ERROR("BParameterGroup::Unflatten(): MakeControl() failed\n");
			return B_ERROR;
		}

		status_t status = parameter->Unflatten(parameter->TypeCode(), buffer, parameterSize);
		if (status < B_OK) {
			ERROR("BParameterGroup::Unflatten(): parameter->Unflatten() failed\n");
			delete parameter;
			return status;
		}

		skip_in_buffer(&buffer, parameterSize);

		// add the item to the list
		parameter->mGroup = this;
		parameter->mWeb = mWeb;
		mControls->AddItem(parameter);

		// add it's old pointer value to the RefFix list kept by the owner web
		if (mWeb != NULL)
			mWeb->AddRefFix(oldPointer, parameter);
	}

	// unflatten sub groups

	count = read_from_buffer_swap32<int32>(&buffer, isSwapped);
	if (count < 0 || count * kAdditionalParameterGroupSize > size_left(size, bufferStart, buffer))
		return B_BAD_VALUE;

	for (int32 i = 0; i < count; i++) {
		// make sure we can read as many bytes
		if (size_left(size, bufferStart, buffer) < 12)
			return B_BAD_VALUE;

		BParameterGroup *oldPointer = read_from_buffer_swap32<BParameterGroup *>(&buffer, isSwapped);
		type_code type = read_from_buffer_swap32<type_code>(&buffer, isSwapped);

		ssize_t groupSize = read_from_buffer_swap32<ssize_t>(&buffer, isSwapped);
		if (groupSize > size_left(size, bufferStart, buffer))
			return B_BAD_VALUE;

		BParameterGroup *group = new BParameterGroup(mWeb, "sub-unnamed");
		if (group == NULL) {
			ERROR("BParameterGroup::Unflatten(): MakeGroup() failed\n");
			return B_ERROR;
		}

		status_t status = group->Unflatten(type, buffer, groupSize);
		if (status != B_OK) {
			ERROR("BParameterGroup::Unflatten(): group->Unflatten() failed\n");
			delete group;
			return status;
		}

		skip_in_buffer(&buffer, groupSize);

		mGroups->AddItem(group);

		// add it's old pointer value to the RefFix list kept by the owner web
		if (mWeb != NULL)
			mWeb->AddRefFix(oldPointer, group);
	}

	return B_OK;
}

/*************************************************************
 * private BParameterGroup
 *************************************************************/

/*
// unimplemented
BParameterGroup::BParameterGroup()
BParameterGroup::BParameterGroup(const BParameterGroup &clone)
BParameterGroup &BParameterGroup::operator=(const BParameterGroup &clone)
*/


/** Creates an uninitialized parameter of the specified type.
 *	Unlike the BParameterGroup::MakeXXXParameter() type of methods, this
 *	method does not add the parameter to this group automatically.
 */

BParameter *
BParameterGroup::MakeControl(int32 type)
{
	CALLED();

	switch (type) {
		case BParameter::B_NULL_PARAMETER:
			return new BNullParameter(-1, B_MEDIA_NO_TYPE, NULL, NULL, NULL);

		case BParameter::B_DISCRETE_PARAMETER:
			return new BDiscreteParameter(-1, B_MEDIA_NO_TYPE, NULL, NULL, NULL);

		case BParameter::B_CONTINUOUS_PARAMETER:
			return new BContinuousParameter(-1, B_MEDIA_NO_TYPE, NULL, NULL, NULL, NULL, 0, 0, 0);

		case BParameter::B_TEXT_PARAMETER:
			return new BTextParameter(-1, B_MEDIA_NO_TYPE, NULL, NULL, NULL, NULL);

		default:
			ERROR("BParameterGroup::MakeControl unknown type %ld\n", type);
			return NULL;
	}
}


//	#pragma mark -

/*************************************************************
 * public BParameter
 *************************************************************/


BParameter::media_parameter_type
BParameter::Type() const
{
	return mType;
}


BParameterWeb *
BParameter::Web() const
{
	return mWeb;
}


BParameterGroup *
BParameter::Group() const
{
	return mGroup;
}


const char *
BParameter::Name() const
{
	return mName;
}


const char *
BParameter::Kind() const
{
	return mKind;
}


const char *
BParameter::Unit() const
{
	return mUnit;
}


int32
BParameter::ID() const
{
	return mID;
}


void
BParameter::SetFlags(uint32 flags)
{
	mFlags = flags;
}


uint32
BParameter::Flags() const
{
	return mFlags;
}


status_t
BParameter::GetValue(void *buffer, size_t *_ioSize, bigtime_t *_when)
{
	CALLED();

	if (buffer == NULL || _ioSize == NULL)
		return B_BAD_VALUE;

	size_t ioSize = *_ioSize;
	if (ioSize <= 0)
		return B_NO_MEMORY;

	if (mWeb == NULL) {
		ERROR("BParameter::GetValue: no parent BParameterWeb\n");
		return B_NO_INIT;
	}

	media_node node = mWeb->Node();
	if (IS_INVALID_NODE(node)) {
		ERROR("BParameter::GetValue: the parent BParameterWeb is not assigned to a BMediaNode\n");
		return B_NO_INIT;
	}

	controllable_get_parameter_data_request request;
	controllable_get_parameter_data_reply reply;

	area_id area;
	void *data;
	if (ioSize > MAX_PARAMETER_DATA) {
		// create an area if large data needs to be transfered
		area = create_area("get parameter data", &data, B_ANY_ADDRESS, ROUND_UP_TO_PAGE(ioSize),
			B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (area < B_OK) {
			ERROR("BParameter::GetValue can't create area of %ld bytes\n", ioSize);
			return B_NO_MEMORY;
		}
	} else {
		area = -1;
		data = reply.rawdata;
	}

	request.parameter_id = mID;
	request.requestsize = ioSize;
	request.area = area;

	status_t status = QueryPort(node.port, CONTROLLABLE_GET_PARAMETER_DATA, &request,
		sizeof(request), &reply, sizeof(reply));
	if (status == B_OK) {
		// we don't want to copy more than the buffer provides
		if (reply.size < ioSize)
			ioSize = reply.size;

		memcpy(buffer, data, ioSize);

		// store reported values

		*_ioSize = reply.size;
		if (_when != NULL)
			*_when = reply.last_change;
	} else
		ERROR("BParameter::GetValue parameter '%s' querying node %d, port %d failed: %s\n", 
			mName, node.node, node.port, strerror(status));

	if (area >= B_OK)
		delete_area(area);

	return status;
}


status_t
BParameter::SetValue(const void *buffer, size_t size, bigtime_t when)
{
	CALLED();

	controllable_set_parameter_data_request request;
	controllable_set_parameter_data_reply reply;
	media_node node;
	area_id area;
	status_t rv;
	void *data;

	if (buffer == 0)
		return B_BAD_VALUE;
	if (size <= 0)
		return B_NO_MEMORY;
		
	if (mWeb == 0) {
		ERROR("BParameter::SetValue: no parent BParameterWeb\n");
		return B_NO_INIT;
	}
	
	node = mWeb->Node();
	if (IS_INVALID_NODE(node)) {
		ERROR("BParameter::SetValue: the parent BParameterWeb is not assigned to a BMediaNode\n");
		return B_NO_INIT;
	}
	
	if (size > MAX_PARAMETER_DATA) {
		// create an area if large data needs to be transfered
		area = create_area("set parameter data", &data, B_ANY_ADDRESS, ROUND_UP_TO_PAGE(size), B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
		if (area < B_OK) {
			ERROR("BParameter::SetValue can't create area of %ld bytes\n", size);
			return B_NO_MEMORY;
		}
	} else {
		area = -1;
		data = request.rawdata;
	}

	memcpy(data, buffer, size);
	request.parameter_id = mID;
	request.when = when;
	request.area = area;
	request.size = size;

	rv = QueryPort(node.port, CONTROLLABLE_SET_PARAMETER_DATA, &request, sizeof(request), &reply, sizeof(reply));
	if (rv != B_OK)
		ERROR("BParameter::SetValue querying node failed\n");

	if (area != -1)
		delete_area(area);
	
	return rv;
}


int32
BParameter::CountChannels()
{
	return mChannels;
}


void
BParameter::SetChannelCount(int32 channel_count)
{
	mChannels = channel_count;
}


media_type
BParameter::MediaType()
{
	return mMediaType;
}


void
BParameter::SetMediaType(media_type m_type)
{
	mMediaType = m_type;
}


int32
BParameter::CountInputs()
{
	ASSERT(mInputs != NULL);

	return mInputs->CountItems();
}


BParameter *
BParameter::InputAt(int32 index)
{
	ASSERT(mInputs != NULL);
	
	return static_cast<BParameter *>(mInputs->ItemAt(index));
}


void
BParameter::AddInput(BParameter *input)
{
	CALLED();

	// BeBook has this method returning a status value,
	// but it should be updated
	if (input == NULL)
		return;

	ASSERT(mInputs != NULL);

	if (mInputs->HasItem(input)) {
		// if already in input list, don't duplicate.
		return;
	}

	mInputs->AddItem(input);
	input->AddOutput(this);
}


int32
BParameter::CountOutputs()
{
	ASSERT(mOutputs != NULL);

	return mOutputs->CountItems();
}


BParameter *
BParameter::OutputAt(int32 index)
{
	ASSERT(mOutputs != NULL);

	return static_cast<BParameter *>(mOutputs->ItemAt(index));
}


void
BParameter::AddOutput(BParameter *output)
{
	CALLED();

	// BeBook has this method returning a status value,
	// but it should be updated
	if (output == NULL)
		return;

	ASSERT(mOutputs != NULL);

	if (mOutputs->HasItem(output)) {
		// if already in output list, don't duplicate.
		return;
	}

	mOutputs->AddItem(output);
	output->AddInput(this);
}


bool
BParameter::IsFixedSize() const
{
	return false;
}


type_code
BParameter::TypeCode() const
{
	return B_MEDIA_PARAMETER_TYPE;
}


ssize_t
BParameter::FlattenedSize() const
{
	CALLED();
	/*
		?? (0x02040607): 4 bytes
		BParameter Struct Size (in bytes): 4 bytes
		ID: 4 bytes
		Name String Length: 1 byte (??)
			Name String: 'Name String Length' bytes
		Kind String Length: 1 byte (??)
			Kind String: 'Kind String Length' bytes
		Unit String Length: 1 byte (??)
			Unit String: 'Unit String Length' bytes
		Inputs Count: 4 bytes
			Inputs (pointers): ('Inputs Count')*4 bytes
		Outputs Count: 4 bytes
			Outputs (pointers): ('Outputs Count')*4 bytes
		Media Type: 4 bytes
		ChannelCount: 4 bytes
		Flags: 4 bytes
	*/
	//35 bytes are guaranteed, after that, add the variable length parts.
	ssize_t size = 35;

	if (mName != NULL)
		size += strlen(mName);
	if (mKind != NULL)
		size += strlen(mKind);
	if (mUnit != NULL)
		size += strlen(mUnit);

	if (mInputs != NULL)
		size += mInputs->CountItems() * sizeof(BParameter *);

	if (mOutputs != NULL)
		size += mOutputs->CountItems() * sizeof(BParameter *);

	return size;
}


status_t
BParameter::Flatten(void *buffer, ssize_t size) const
{
	CALLED();

	if (buffer == NULL) {
		ERROR("BParameter::Flatten buffer is NULL\n");
		return B_NO_INIT;
	}

	// NOTICE: It is important that this value is the size returned by
	// BParameter::FlattenedSize(), not by a descendent's override of this method.
	ssize_t actualSize = BParameter::FlattenedSize();
	if (size < actualSize) {
		ERROR("BParameter::Flatten(): size too small\n");
		return B_NO_MEMORY;
	}

	write_to_buffer<uint32>(&buffer, kParameterMagic);
	write_to_buffer<ssize_t>(&buffer, actualSize);
	write_to_buffer<int32>(&buffer, mID);

	write_string_to_buffer(&buffer, mName);
	write_string_to_buffer(&buffer, mKind);
	write_string_to_buffer(&buffer, mUnit);

	// flatten and write the list of inputs
	ssize_t count = mInputs ? mInputs->CountItems() : 0;
	write_to_buffer<ssize_t>(&buffer, count);

	if (count > 0) {
		memcpy(buffer, mInputs->Items(), sizeof(BParameter *) * count);
		skip_in_buffer(&buffer, sizeof(BParameter *) * count);
	}

	// flatten and write the list of outputs
	count = mOutputs ? mOutputs->CountItems() : 0;
	write_to_buffer<ssize_t>(&buffer, count);

	if (count > 0) {
		memcpy(buffer, mOutputs->Items(), sizeof(BParameter *) * count);
		skip_in_buffer(&buffer, sizeof(BParameter *) * count);
	}

	write_to_buffer<media_type>(&buffer, mMediaType);
	write_to_buffer<int32>(&buffer, mChannels);
	write_to_buffer<uint32>(&buffer, mFlags);

	return B_OK;
}


bool
BParameter::AllowsTypeCode(type_code code) const
{
	return (code == TypeCode());
}


status_t
BParameter::Unflatten(type_code code, const void *buffer, ssize_t size)
{
	CALLED();

	if (!AllowsTypeCode(code)) {
		ERROR("BParameter::Unflatten(): wrong type code\n");
		return B_BAD_TYPE;
	}

	if (buffer == NULL) {
		ERROR("BParameter::Unflatten(): buffer is NULL\n");
		return B_NO_INIT;
	}

	// if the buffer is smaller than the size needed to read the
	// signature and struct size fields, then there is a problem
	if (size < static_cast<ssize_t>(sizeof(int32) + sizeof(ssize_t))) {
		ERROR("BParameter::Unflatten() size too small\n");
		return B_BAD_VALUE;
	}

	const void *bufferStart = buffer;

	// check magic

	uint32 magic = read_from_buffer<uint32>(&buffer);
	if (magic == B_SWAP_INT32(kParameterMagic))
		mSwapDetected = true;
	else if (magic == kParameterMagic)
		mSwapDetected = false;
	else {
		ERROR("BParameter::Unflatten(): bad magic\n");
		return B_BAD_TYPE;
	}

	ssize_t parameterSize = read_from_buffer_swap32<ssize_t>(&buffer, mSwapDetected);
	if (parameterSize > size) {
		ERROR("BParameter::Unflatten(): buffer too small (%ld > %ld)\n", parameterSize, size);
		return B_BAD_VALUE;
	}

	//if the struct doesn't meet the minimum size for
	//a flattened BParameter, then return an error.
	//MinFlattenedParamSize = 
	//ID (4 bytes)
	//Name String Length (1 byte)
	//Kind String Length (1 byte)
	//Unit String Length (1 byte)
	//Inputs Count (4 bytes)
	//Outputs Count (4 bytes)
	//Media Type (4 bytes)
	//Channel Count (4 bytes)
	//Flags (4 bytes)
	//TOTAL: 27 bytes
	const ssize_t MinFlattenedParamSize(27);
	if (parameterSize < MinFlattenedParamSize) {
		ERROR("BParameter::Unflatten out of memory (2)\n");
		return B_ERROR;
	}

	mID = read_from_buffer_swap32<int32>(&buffer, mSwapDetected);

	if (read_string_from_buffer(&buffer, &mName, size_left(size, bufferStart, buffer)) < B_OK
		|| read_string_from_buffer(&buffer, &mKind, size_left(size, bufferStart, buffer)) < B_OK	
		|| read_string_from_buffer(&buffer, &mUnit, size_left(size, bufferStart, buffer)) < B_OK)	
		return B_NO_MEMORY;

	// read the list of inputs

	// it will directly add the pointers in the flattened message to the list;
	// these will be fixed to point to the real inputs/outputs later in FixRefs()

	int32 count = read_from_buffer_swap32<int32>(&buffer, mSwapDetected);	

	if (mInputs == NULL)
		mInputs = new BList();
	else
		mInputs->MakeEmpty();

	for (int32 i = 0; i < count; i++) {
		mInputs->AddItem(read_from_buffer_swap32<BParameter * const>(&buffer, mSwapDetected));
	}

	// read the list of outputs

	count = read_from_buffer_swap32<int32>(&buffer, mSwapDetected);

	if (mOutputs == NULL)
		mOutputs = new BList();
	else
		mOutputs->MakeEmpty();

	for (int32 i = 0; i < count; i++) {
		mOutputs->AddItem(read_from_buffer_swap32<BParameter * const>(&buffer, mSwapDetected));
	}

	mMediaType = read_from_buffer_swap32<media_type>(&buffer, mSwapDetected);
	mChannels = read_from_buffer_swap32<int32>(&buffer, mSwapDetected);
	mFlags = read_from_buffer_swap32<uint32>(&buffer, mSwapDetected);

	return B_OK;
}


/*************************************************************
 * private BParameter
 *************************************************************/


BParameter::BParameter(int32 id, media_type mediaType, media_parameter_type type,
	BParameterWeb *web, const char *name, const char *kind, const char *unit)
	:
	mID(id),
	mType(type),
	mWeb(web),
	mGroup(NULL),
	mSwapDetected(true),
	mMediaType(mediaType),
	mChannels(1),
	mFlags(0)
{
	CALLED();

	mName = strndup(name, 256);
	mKind = strndup(kind, 256);
	mUnit = strndup(unit, 256);

	// create empty input/output lists
	mInputs = new BList();
	mOutputs = new BList();
}


BParameter::~BParameter()
{
	CALLED();

	// don't worry about the mWeb/mGroup properties, you don't need
	// to remove yourself from a web/group since the only way in which
	// a parameter is destroyed is when the owner web/group destroys it

	free(mName);
	free(mKind);
	free(mUnit);

	delete mInputs;
	delete mOutputs;
	
	mName = NULL; mKind = NULL; mUnit = NULL; mInputs = NULL; mOutputs = NULL;
}


/** Replaces references to items in the old list with the corresponding
 *	items in the updated list. The references are replaced in the input
 *	and output lists.
 *	This is called by BParameterWeb::Unflatten().
 */

void
BParameter::FixRefs(BList &old, BList &updated)
{
	CALLED();
	ASSERT(mInputs != NULL);
	ASSERT(mOutputs != NULL);

	// update inputs

	void **items = static_cast<void **>(mInputs->Items());
	int32 count = mInputs->CountItems();

	for (int32 i = 0; i < count; i++) {
		int32 index = old.IndexOf(items[i]);
		if (index >= 0)
			items[i] = updated.ItemAt(index);
		else {
			ERROR("BParameter::FixRefs(): No mapping found for input");
			items[i] = NULL;
		}
	}

	// remove all NULL inputs (those which couldn't be mapped)

	for (int32 i = count; i-- > 0;) {
		if (items[i] == NULL)
			mInputs->RemoveItem(i);
	}

	// update outputs

	items = static_cast<void **>(mOutputs->Items());
	count = mOutputs->CountItems();

	for (int32 i = 0; i < count; i++) {
		int32 index = old.IndexOf(items[i]);
		if (index >= 0)
			items[i] = updated.ItemAt(index);
		else {
			ERROR("BParameter::FixRefs(): No mapping found for output");
			items[i] = NULL;
		}
	}

	// remove all NULL outputs (those which couldn't be mapped)
	
	for (int32 i = count; i-- > 0;) {
		if (items[i] == NULL)
			mOutputs->RemoveItem(i);
	}
}


//	#pragma mark -

/*************************************************************
 * public BContinuousParameter
 *************************************************************/


type_code
BContinuousParameter::ValueType()
{
	return B_FLOAT_TYPE;
}


float
BContinuousParameter::MinValue()
{
	return mMinimum;
}


float
BContinuousParameter::MaxValue()
{
	return mMaximum;
}


float
BContinuousParameter::ValueStep()
{
	return mStepping;
}


void
BContinuousParameter::SetResponse(int resp, float factor, float offset)
{
	mResponse = static_cast<response>(resp);
	mFactor = factor;
	mOffset = offset;
}


void
BContinuousParameter::GetResponse(int *resp, float *factor, float *offset)
{
	if (resp != NULL)
		*resp = mResponse;
	if (factor != NULL)
		*factor = mFactor;
	if (offset != NULL)
		*offset = mOffset;
}


ssize_t
BContinuousParameter::FlattenedSize() const
{
	CALLED();

	// only adds a fixed amount of bytes
	return BParameter::FlattenedSize() + kAdditionalContinuousParameterSize;
}


status_t
BContinuousParameter::Flatten(void *buffer, ssize_t size) const
{
	CALLED();

	if (buffer == NULL) {
		ERROR("BContinuousParameter::Flatten(): buffer is NULL\n");
		return B_NO_INIT;
	}

	ssize_t parameterSize = BParameter::FlattenedSize();
	if (size < (parameterSize + kAdditionalContinuousParameterSize)) {
		ERROR("BContinuousParameter::Flatten(): size to small\n");
		return B_NO_MEMORY;
	}

	status_t status = BParameter::Flatten(buffer, size);
	if (status != B_OK) {
		ERROR("BContinuousParameter::Flatten(): BParameter::Flatten() failed\n");
		return status;
	}

	// add our data to the general flattened BParameter

	skip_in_buffer(&buffer, parameterSize);

	write_to_buffer<float>(&buffer, mMinimum);
	write_to_buffer<float>(&buffer, mMaximum);
	write_to_buffer<float>(&buffer, mStepping);
	write_to_buffer<response>(&buffer, mResponse);
	write_to_buffer<float>(&buffer, mFactor);
	write_to_buffer<float>(&buffer, mOffset);

	return B_OK;
}


status_t
BContinuousParameter::Unflatten(type_code code, const void *buffer, ssize_t size)
{
	CALLED();

	// we try to check if the buffer size is long enough to hold an object
	// as early as possible.

	if (!AllowsTypeCode(code)) {
		ERROR("BContinuousParameter::Unflatten wrong type code\n");
		return B_BAD_TYPE;
	}

	if (buffer == NULL) {
		ERROR("BContinuousParameter::Unflatten buffer is NULL\n");
		return B_NO_INIT;
	}

	// if the buffer is smaller than the size needed to read the
	// signature and struct size fields, then there is a problem
	if (size < static_cast<ssize_t>(sizeof(int32) + sizeof(ssize_t))) {
		ERROR("BContinuousParameter::Unflatten size too small\n");
		return B_ERROR;
	}

	status_t status = BParameter::Unflatten(code, buffer, size);
	if (status != B_OK) {
		ERROR("BContinuousParameter::Unflatten(): BParameter::Unflatten failed\n");
		return status;
	}

	ssize_t parameterSize = BParameter::FlattenedSize();
	skip_in_buffer(&buffer, parameterSize);

	if (size < (parameterSize + kAdditionalContinuousParameterSize)) {
		ERROR("BContinuousParameter::Unflatten(): buffer too small\n");
		return B_BAD_VALUE;
	}

	mMinimum = read_from_buffer_swap32<float>(&buffer, SwapOnUnflatten());
	mMaximum = read_from_buffer_swap32<float>(&buffer, SwapOnUnflatten());
	mStepping = read_from_buffer_swap32<float>(&buffer, SwapOnUnflatten());
	mResponse = read_from_buffer_swap32<response>(&buffer, SwapOnUnflatten());
	mFactor = read_from_buffer_swap32<float>(&buffer, SwapOnUnflatten());
	mOffset = read_from_buffer_swap32<float>(&buffer, SwapOnUnflatten());

	return B_OK;
}


/*************************************************************
 * private BContinuousParameter
 *************************************************************/


BContinuousParameter::BContinuousParameter(int32 id, media_type m_type,
	BParameterWeb *web, const char *name, const char *kind, const char *unit,
	float minimum, float maximum, float stepping)
	:	BParameter(id, m_type, B_CONTINUOUS_PARAMETER, web, name, kind, unit),
	mMinimum(minimum), mMaximum(maximum), mStepping(stepping),
	mResponse(B_LINEAR), mFactor(1.0), mOffset(0.0)
{
	CALLED();
}


BContinuousParameter::~BContinuousParameter()
{
	CALLED();
}


//	#pragma mark -

/*************************************************************
 * public BDiscreteParameter
 *************************************************************/


type_code
BDiscreteParameter::ValueType()
{
	return B_INT32_TYPE;
}


int32
BDiscreteParameter::CountItems()
{
	ASSERT(mValues != NULL);

	return mValues->CountItems();
}


const char *
BDiscreteParameter::ItemNameAt(int32 index)
{
	ASSERT(mSelections != NULL);

	return reinterpret_cast<const char *>(mSelections->ItemAt(index));
}


int32
BDiscreteParameter::ItemValueAt(int32 index)
{
	ASSERT(mValues != NULL);

	int32 *item = static_cast<int32 *>(mValues->ItemAt(index));
	if (item == NULL)
		return 0;

	return *item;
}


status_t
BDiscreteParameter::AddItem(int32 value, const char *name)
{
	CALLED();
	//TRACE("\tthis = %p, value = %ld, name = \"%s\"\n", this, value, name);
	ASSERT(mValues != NULL);
	ASSERT(mSelections != NULL);

	int32 *valueCopy = new int32(value);
	char *nameCopy = strndup(name, 256);
	if (name != NULL && nameCopy == NULL)
		return B_NO_MEMORY;

	if (!mValues->AddItem(valueCopy)
		|| !mSelections->AddItem(nameCopy))
		return B_NO_MEMORY;

	return B_OK;
}


status_t
BDiscreteParameter::MakeItemsFromInputs()
{
	CALLED();
	ASSERT(mValues != NULL);
	ASSERT(mSelections != NULL);
	ASSERT(mInputs != NULL);

	int32 count = mInputs->CountItems();
	for(int32 i = 0; i < count; i++) {
		BParameter *parameter = static_cast<BParameter *>(mInputs->ItemAt(i));
		AddItem(i, parameter->Name());
	}

	return B_OK;
}


status_t
BDiscreteParameter::MakeItemsFromOutputs()
{
	CALLED();
	ASSERT(mValues != NULL);
	ASSERT(mSelections != NULL);
	ASSERT(mOutputs != NULL);

	int32 count = mOutputs->CountItems();
	for(int32 i = 0; i < count; i++) {
		BParameter *parameter = static_cast<BParameter *>(mOutputs->ItemAt(i));
		AddItem(i, parameter->Name());
	}

	return B_OK;
}


void
BDiscreteParameter::MakeEmpty()
{
	CALLED();
	ASSERT(mValues != NULL);
	ASSERT(mSelections != NULL);

	for (int32 i = mValues->CountItems(); i-- > 0;) {
		delete static_cast<int32 *>(mValues->ItemAt(i));
	}
	mValues->MakeEmpty();

	for(int32 i = mSelections->CountItems(); i-- > 0;) {
		free(static_cast<char *>(mSelections->ItemAt(i)));
	}
	mSelections->MakeEmpty();
}


ssize_t
BDiscreteParameter::FlattenedSize() const
{
	CALLED();

	ssize_t size = BParameter::FlattenedSize() + kAdditionalDiscreteParameterSize;

	int32 count = mValues ? mValues->CountItems() : 0;
	for (int32 i = 0; i < count; i++) {
		char *selection = static_cast<char *>(mSelections->ItemAt(i));

		if (selection != NULL)
			size += min_c(strlen(selection), 255);

		size += 5;
			// string length + value
	}

	return size;
}


status_t
BDiscreteParameter::Flatten(void *buffer, ssize_t size) const
{
	CALLED();

	if (buffer == NULL) {
		ERROR("BDiscreteParameter::Flatten(): buffer is NULL\n");
		return B_NO_INIT;
	}

	ssize_t parameterSize = BParameter::FlattenedSize();

	if (size < FlattenedSize()) {
		ERROR("BDiscreteParameter::Flatten(): size too small\n");
		return B_NO_MEMORY;
	}

	status_t status = BParameter::Flatten(buffer, size);
	if (status != B_OK) {
		ERROR("BDiscreteParameter::Flatten(): BParameter::Flatten failed\n");
		return status;
	}

	skip_in_buffer(&buffer, parameterSize);

	int32 count = mValues ? mValues->CountItems() : 0;
	write_to_buffer<int32>(&buffer, count);

	// write out all value/name pairs
	for (int32 i = 0; i < count; i++) {
		const char *selection = static_cast<char *>(mSelections->ItemAt(i));
		const int32 *value = static_cast<int32 *>(mValues->ItemAt(i));

		write_string_to_buffer(&buffer, selection);
		write_to_buffer<int32>(&buffer, value ? *value : 0);
	}

	return B_OK;
}


status_t
BDiscreteParameter::Unflatten(type_code code, const void *buffer, ssize_t size)
{
	CALLED();

	if (!AllowsTypeCode(code)) {
		ERROR("BDiscreteParameter::Unflatten(): bad type code\n");
		return B_BAD_TYPE;
	}

	if (buffer == NULL) {
		ERROR("BDiscreteParameter::Unflatten(): buffer is NULL\n");
		return B_NO_INIT;
	}

	// if the buffer is smaller than the size needed to read the
	// signature and struct size fields, then there is a problem
	if (size < static_cast<ssize_t>(sizeof(int32) + sizeof(ssize_t))) {
		ERROR("BDiscreteParameter::Unflatten(): size too small\n");
		return B_ERROR;
	}

	const void *bufferStart = buffer;

	status_t status = BParameter::Unflatten(code, buffer, size);
	if (status != B_OK) {
		ERROR("BDiscreteParameter::Unflatten(): BParameter::Unflatten failed\n");
		return status;
	}

	ssize_t parameterSize = BParameter::FlattenedSize();
	skip_in_buffer(&buffer, parameterSize);

	if (size < (parameterSize + kAdditionalDiscreteParameterSize)) {
		ERROR("BDiscreteParameter::Unflatten(): buffer too small\n");
		return B_BAD_VALUE;
	}

	int32 count = read_from_buffer_swap32<int32>(&buffer, SwapOnUnflatten());

	// clear any existing name/value pairs
	MakeEmpty();

	for (int32 i = 0; i < count; i++) {
		char *name;
		if (read_string_from_buffer(&buffer, &name, size_left(size, bufferStart, buffer)) < B_OK)
			return B_BAD_DATA;

		if (size_left(size, bufferStart, buffer) < (int)sizeof(int32))
			return B_BAD_DATA;

		int32 value = read_from_buffer_swap32<int32>(&buffer, SwapOnUnflatten());

		AddItem(value, name);
	}

	return B_OK;
}


/*************************************************************
 * private BDiscreteParameter
 *************************************************************/


BDiscreteParameter::BDiscreteParameter(int32 id, media_type mediaType,
	BParameterWeb *web, const char *name, const char *kind)
	:	BParameter(id, mediaType, B_DISCRETE_PARAMETER, web, name, kind, NULL)
{
	CALLED();

	mSelections = new BList();
	mValues = new BList();
}


BDiscreteParameter::~BDiscreteParameter()
{
	CALLED();

	MakeEmpty();

	delete mSelections;
	delete mValues;

	mSelections = NULL; mValues = NULL;
}


//	#pragma mark -

/*************************************************************
 * public BNullParameter
 *************************************************************/


type_code
BNullParameter::ValueType()
{
	// NULL parameters have no value type
	return 0;
}


ssize_t
BNullParameter::FlattenedSize() const
{
	return BParameter::FlattenedSize();
}


status_t
BNullParameter::Flatten(void *buffer, ssize_t size) const
{
	return BParameter::Flatten(buffer, size);
}


status_t
BNullParameter::Unflatten(type_code code, const void *buffer, ssize_t size)
{
	return BParameter::Unflatten(code, buffer, size);
}


/*************************************************************
 * private BNullParameter
 *************************************************************/


BNullParameter::BNullParameter(int32 id, media_type mediaType, BParameterWeb *web,
	const char *name, const char *kind)
	: BParameter(id, mediaType, B_NULL_PARAMETER, web, name, kind, NULL)
{
}


BNullParameter::~BNullParameter()
{
}


//	#pragma mark -

/*************************************************************
 * public BTextParameter
 *************************************************************/


size_t
BTextParameter::MaxBytes() const
{
	// NULL parameters have no value type
	return mMaxBytes;
}


type_code
BTextParameter::ValueType()
{
	// NULL parameters have no value type
	return 0;
}


ssize_t
BTextParameter::FlattenedSize() const
{
	return BParameter::FlattenedSize() + sizeof(mMaxBytes);
}


status_t
BTextParameter::Flatten(void *buffer, ssize_t size) const
{
	if (buffer == NULL) {
		ERROR("BTextParameter::Flatten(): buffer is NULL\n");
		return B_NO_INIT;
	}

	ssize_t parameterSize = BParameter::FlattenedSize();
	if (size < static_cast<ssize_t>(parameterSize + sizeof(mMaxBytes))) {
		ERROR("BContinuousParameter::Flatten(): size to small\n");
		return B_NO_MEMORY;
	}

	status_t status = BParameter::Flatten(buffer, size);
	if (status != B_OK) {
		ERROR("BTextParameter::Flatten(): BParameter::Flatten() failed\n");
		return status;
	}

	// add our data to the general flattened BParameter

	skip_in_buffer(&buffer, parameterSize);

	write_to_buffer<uint32>(&buffer, mMaxBytes);

	return B_OK;
}


status_t
BTextParameter::Unflatten(type_code code, const void *buffer, ssize_t size)
{
	// we try to check if the buffer size is long enough to hold an object
	// as early as possible.

	if (!AllowsTypeCode(code)) {
		ERROR("BTextParameter::Unflatten wrong type code\n");
		return B_BAD_TYPE;
	}

	if (buffer == NULL) {
		ERROR("BTextParameter::Unflatten buffer is NULL\n");
		return B_NO_INIT;
	}

	if (size < static_cast<ssize_t>(sizeof(mMaxBytes))) {
		ERROR("BTextParameter::Unflatten size too small\n");
		return B_ERROR;
	}

	status_t status = BParameter::Unflatten(code, buffer, size);
	if (status != B_OK) {
		ERROR("BTextParameter::Unflatten(): BParameter::Unflatten failed\n");
		return status;
	}

	ssize_t parameterSize = BParameter::FlattenedSize();
	skip_in_buffer(&buffer, parameterSize);

	if (size < static_cast<ssize_t>(parameterSize + sizeof(mMaxBytes))) {
		ERROR("BTextParameter::Unflatten(): buffer too small\n");
		return B_BAD_VALUE;
	}

	mMaxBytes = read_from_buffer_swap32<uint32>(&buffer, SwapOnUnflatten());

	return B_OK;
}


/*************************************************************
 * private BTextParameter
 *************************************************************/


BTextParameter::BTextParameter(int32 id, media_type mediaType, BParameterWeb *web,
	const char *name, const char *kind, size_t max_bytes)
	: BParameter(id, mediaType, B_NULL_PARAMETER, web, name, kind, NULL)
{
	mMaxBytes = max_bytes;
}


BTextParameter::~BTextParameter()
{
}


//	#pragma mark -
//	reserved functions


status_t BParameterWeb::_Reserved_ControlWeb_0(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_1(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_2(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_3(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_4(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_5(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_6(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_7(void *) { return B_ERROR; }

status_t BParameterGroup::_Reserved_ControlGroup_0(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_1(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_2(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_3(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_4(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_5(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_6(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_7(void *) { return B_ERROR; }

status_t BParameter::_Reserved_Control_0(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_1(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_2(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_3(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_4(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_5(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_6(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_7(void *) { return B_ERROR; }

status_t BContinuousParameter::_Reserved_ContinuousParameter_0(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_1(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_2(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_3(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_4(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_5(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_6(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_7(void *) { return B_ERROR; }

status_t BDiscreteParameter::_Reserved_DiscreteParameter_0(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_1(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_2(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_3(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_4(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_5(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_6(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_7(void *) { return B_ERROR; }

status_t BNullParameter::_Reserved_NullParameter_0(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_1(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_2(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_3(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_4(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_5(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_6(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_7(void *) { return B_ERROR; }

status_t BTextParameter::_Reserved_TextParameter_0(void *) { return B_ERROR; }
status_t BTextParameter::_Reserved_TextParameter_1(void *) { return B_ERROR; }
status_t BTextParameter::_Reserved_TextParameter_2(void *) { return B_ERROR; }
status_t BTextParameter::_Reserved_TextParameter_3(void *) { return B_ERROR; }
status_t BTextParameter::_Reserved_TextParameter_4(void *) { return B_ERROR; }
status_t BTextParameter::_Reserved_TextParameter_5(void *) { return B_ERROR; }
status_t BTextParameter::_Reserved_TextParameter_6(void *) { return B_ERROR; }
status_t BTextParameter::_Reserved_TextParameter_7(void *) { return B_ERROR; }


