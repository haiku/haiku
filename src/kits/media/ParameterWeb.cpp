/***********************************************************************
 * AUTHOR: Zousar Shaker
 *   FILE: ParameterWeb.cpp
 *  DESCR: BParameterWeb, BParameterGroup, BParameter, BNullParameter, 
 *         BContinuousParameter, BDiscreteParameter
 ***********************************************************************/
#include <ParameterWeb.h>
#include <Debug.h>
#include "debug.h"

//--------BEGIN-ADDED-BY-ZS----------------------------------

	//Comment keywords:
	// FIXME: Something that should be fixed
	// QUESTION: Something that needs clarification
	// NOTICE: Explanation of what's going on
	
typedef unsigned char byte;

/* by Marcus Overhagen: Sorry Zousar, but I really want the program
 * to be stopped when something is wrong!
 */
#if 1 /* always use these */

#define ASSERT_WRETURN_VALUE(CheckExpr,RetVal) \
		ASSERT(CheckExpr)
	
#define ASSERT_RETURN(CheckExpr) \
		ASSERT(CheckExpr)

#else /* and disable the other macros */

/*************************************************************
 * Used to check assertions, and if the assertions fail, a
 * value is returned.  It is used only to check
 * potentially erronious conditions INTERNAL to the code (ie:
 * if I have forgotten to change a variable from NULL to some
 * usable value) it is NOT used to check erronious input from
 * the outside world (ie: if the user of a class passes NULL for
 * some parameter). (ZS)
 *************************************************************/
#define ASSERT_WRETURN_VALUE(CheckExpr,RetVal)\
	if(!(CheckExpr))\
	{\
		return (RetVal);\
	}
/*************************************************************
 * Used to check assertions, and if the assertions fail, the
 * method returns (no return value).  It is used only to check
 * potentially erronious conditions INTERNAL to the code (ie:
 * if I have forgotten to change a variable from NULL to some
 * usable value) it is NOT used to check erronious input from
 * the outside world (ie: if the user of a class passes NULL for
 * some parameter). (ZS)
 *************************************************************/
#define ASSERT_RETURN(CheckExpr)\
	if(!(CheckExpr))\
	{\
		return;\
	}

#endif /* end of disabled macros */

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

//--------END-ADDED-BY-ZS-------------------------------------
	
	

/*************************************************************
 * 
 *************************************************************/

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

/*************************************************************
 * public BParameterWeb
 *************************************************************/

BParameterWeb::BParameterWeb():mNode(media_node::null)
{
	mGroups = new BList();
	
	mOldRefs = new BList();
	mNewRefs = new BList();
}


BParameterWeb::~BParameterWeb()
{
	int i;
	
	if(mGroups != NULL)
	{
		for(i = 0;i < mGroups->CountItems(); i++)
		{
			BParameterGroup *CurrentGroup = static_cast<BParameterGroup *>(mGroups->ItemAt(i));
			
			if(CurrentGroup != NULL)
			{
				delete CurrentGroup;
			}
		}
		mGroups->MakeEmpty();
		delete mGroups;
		mGroups = NULL;
	}
	
	if(mOldRefs != NULL)
	{
		mOldRefs->MakeEmpty();
		delete mOldRefs;
	}
	
	if(mNewRefs != NULL)
	{
		mNewRefs->MakeEmpty();
		delete mNewRefs;
	}
	
}


media_node
BParameterWeb::Node()
{
	return mNode;
}


BParameterGroup *
BParameterWeb::MakeGroup(const char *name)
{
	ASSERT_WRETURN_VALUE(mGroups != NULL,NULL);
	
	BParameterGroup *NewGroup = new BParameterGroup(this,name);
	
	mGroups->AddItem(NewGroup);
	
	return NewGroup;
}


int32
BParameterWeb::CountGroups()
{
	ASSERT_WRETURN_VALUE(mGroups != NULL,0);
	
	return mGroups->CountItems();
}


BParameterGroup *
BParameterWeb::GroupAt(int32 index)
{
	ASSERT_WRETURN_VALUE(mGroups != NULL,NULL);
	
	return static_cast<BParameterGroup *>(mGroups->ItemAt(index));
}


int32
BParameterWeb::CountParameters()
{
	//iterative traversal of the parameter web
	//NOTE: This most definately should be tested and debugged.
	
	ASSERT_WRETURN_VALUE(mGroups != NULL,0);
	
	int32 RetVal = 0;
	
	int i;
	int Limit = mGroups->CountItems();
	for(i = 0; i < Limit; i++)
	{
		BList *GroupStack = new BList();
		BList *IterStack = new BList();
		BParameterGroup *CurrentGroup = static_cast<BParameterGroup *>(mGroups->ItemAt(i));
		int *CurrentIter = new int(0);
		
		while(1)
		{
			if(CurrentGroup != NULL)
			{
				if((*CurrentIter) == 0)
				//if this is the first time you're encountering this node, add the parameters within it.
				{
					RetVal += CurrentGroup->CountParameters();
				}
				
				if((*CurrentIter) < CurrentGroup->CountGroups())
				//if we've still got sub-groups within the current group to account for
				{
					IterStack->AddItem(CurrentIter);
					GroupStack->AddItem(CurrentGroup);
					
					//update the current group
					CurrentGroup = CurrentGroup->GroupAt(*CurrentIter);
					//increment the current iter
					(*CurrentIter)++;
					
					//create a new iter for the group you're descending into
					CurrentIter = new int(0);
				}
				else if(GroupStack->CountItems())
				//we've taken care of all the subgroups of the current group, and there's still something on the stack, clean up, and pop it
				{
					//toss out the iter associated with this group
					if(CurrentIter != NULL)
						delete CurrentIter;
						
					CurrentGroup = static_cast<BParameterGroup *>(GroupStack->RemoveItem(GroupStack->CountItems()-1));
					CurrentIter = static_cast<int *>(IterStack->RemoveItem(IterStack->CountItems()-1));
				}
				else
				//we've taken care of all the subgroups of the current group, and there's nothing on the stack, we're done.
				{
					break;
				}
				
			}
			else if(GroupStack->CountItems())
			{
				if(CurrentIter != NULL)
					delete CurrentIter;
					
				CurrentGroup = static_cast<BParameterGroup *>(GroupStack->RemoveItem(GroupStack->CountItems()-1));
				CurrentIter = static_cast<int *>(IterStack->RemoveItem(IterStack->CountItems()-1));
			}
			else
			{
				//NULL current group, and nothing on the stack, it's time to exit
				break;
			}
		}
		
		if(CurrentIter != NULL)
			delete CurrentIter;
		
		delete IterStack;
		delete GroupStack;
	}
	
	return RetVal;
}


BParameter *
BParameterWeb::ParameterAt(int32 index)
{
	//iterative traversal of the parameter web
	//NOTE: This most definately should be tested and debugged.
	
	ASSERT_WRETURN_VALUE(mGroups != NULL,0);
	
	
	int i;
	int Limit = mGroups->CountItems();
	for(i = 0; i < Limit; i++)
	{
		BList *GroupStack = new BList();
		BList *IterStack = new BList();
		BParameterGroup *CurrentGroup = static_cast<BParameterGroup *>(mGroups->ItemAt(i));
		int *CurrentIter = new int(0);
		
		while(1)
		{
			if(CurrentGroup != NULL)
			{
				
				if((*CurrentIter) == 0)
				//if this is the first time you're encountering this node, add the parameters within it.
				{
					if(index < CurrentGroup->CountParameters())
					{
						//delete the current iter (it is not on the stack)
						if(CurrentIter != NULL)
							delete CurrentIter;
						
						//delete items in the IterStack	
						for(int j = 0; j < IterStack->CountItems(); j++)
						{
							int *Temp = static_cast<int *>(IterStack->ItemAt(j));
							if(Temp != NULL)
								delete CurrentIter;
						}
						IterStack->MakeEmpty();
						delete IterStack;
						
						GroupStack->MakeEmpty();
						delete GroupStack;
						
						return CurrentGroup->ParameterAt(index);
					}
					else
					{
						index -= CurrentGroup->CountParameters();
					}
				}
				
				if((*CurrentIter) < CurrentGroup->CountGroups())
				//if we've still got sub-groups within the current group to account for
				{
					IterStack->AddItem(CurrentIter);
					GroupStack->AddItem(CurrentGroup);
					
					//update the current group
					CurrentGroup = CurrentGroup->GroupAt(*CurrentIter);
					//increment the current iter
					(*CurrentIter)++;
					
					//create a new iter for the group you're descending into
					CurrentIter = new int(0);
				}
				else if(GroupStack->CountItems())
				//we've taken care of all the subgroups of the current group, and there's still something on the stack, clean up, and pop it
				{
					//toss out the iter associated with this group
					if(CurrentIter != NULL)
						delete CurrentIter;
						
					CurrentGroup = static_cast<BParameterGroup *>(GroupStack->RemoveItem(GroupStack->CountItems()-1));
					CurrentIter = static_cast<int *>(IterStack->RemoveItem(IterStack->CountItems()-1));
				}
				else
				//we've taken care of all the subgroups of the current group, and there's nothing on the stack, we're done.
				{
					break;
				}
				
			}
			else if(GroupStack->CountItems())
			{
				if(CurrentIter != NULL)
					delete CurrentIter;
					
				CurrentGroup = static_cast<BParameterGroup *>(GroupStack->RemoveItem(GroupStack->CountItems()-1));
				CurrentIter = static_cast<int *>(IterStack->RemoveItem(IterStack->CountItems()-1));
			}
			else
			{
				//NULL current group, and nothing on the stack, it's time to exit
				break;
			}
		}
		
		if(CurrentIter != NULL)
			delete CurrentIter;
		
		delete IterStack;
		delete GroupStack;
	}
	
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
	ssize_t RetVal = sizeof(int32) + 2*sizeof(int32) + sizeof(media_node);
	
	int i;
	int limit;
	
	limit = mGroups->CountItems();
	for(i = 0; i < limit; i++)
	{
		BParameterGroup *CurrentGroup = static_cast<BParameterGroup *>(mGroups->ItemAt(i));
		if(CurrentGroup != NULL)
		{
			//overhead for each parameter flattened
			RetVal += 8; //4 bytes for the flattened size, and 4 in the 'mystery parameter'...
			RetVal += CurrentGroup->FlattenedSize();
		}
	}
	
	return RetVal;
}


status_t
BParameterWeb::Flatten(void *buffer,
					   ssize_t size) const
{
	if(buffer == NULL)
		return B_NO_INIT;
	
	//NOTICE: It is important that this value is the size returned by BParameterGroup::FlattenedSize,
	//		  not by a descendent's override of this method.
	ssize_t ActualFSize = BParameterWeb::FlattenedSize();
	
	if(size < ActualFSize)
		return B_NO_MEMORY;
	
	byte *CurrentPos = static_cast<byte *>(buffer);
	
	//QUESTION: I have no idea where this magic number came from, and i'm not sure that it's
	//being written in the correct byte order.
	*(reinterpret_cast<int32 *>(CurrentPos)) = 0x01030506;
	CurrentPos += sizeof(int32);

	//QUESTION: Another unknown constant.  This one is different in style than the others though.
	*(reinterpret_cast<int32 *>(CurrentPos)) = 1;
	CurrentPos += sizeof(int32);
	

	int i;
	int NumItems;
	void **Items;

	
	ssize_t *TotalWrittenSubGroupsCount = reinterpret_cast<ssize_t *>(CurrentPos);
	(*TotalWrittenSubGroupsCount) = 0;
	CurrentPos += sizeof(ssize_t);
	
	if(mGroups != NULL)
	{
		NumItems = mGroups->CountItems();
		Items = static_cast<void **>(mGroups->Items());
		
		for(i = 0; i < NumItems; i++)
		{
			BParameterGroup *CurrentSubGroup = static_cast<BParameterGroup *>(Items[i]);
			if(CurrentSubGroup != NULL)
			{
				ssize_t FlattenedSubGroupSize = CurrentSubGroup->FlattenedSize();
				
				//write the flattened size value
				*(reinterpret_cast<ssize_t *>(CurrentPos)) = FlattenedSubGroupSize;
				CurrentPos += sizeof(ssize_t);
				
				//write the flattened sub group
				status_t SubGroupFlattenStatus = CurrentSubGroup->Flatten(CurrentPos,FlattenedSubGroupSize);
				
				if(SubGroupFlattenStatus != B_OK)
				{
					return SubGroupFlattenStatus;
				}
				
				CurrentPos += FlattenedSubGroupSize;
				
				(*TotalWrittenSubGroupsCount)++;
			}
		}
	}

	return B_OK;
}


bool
BParameterWeb::AllowsTypeCode(type_code code) const
{
	return (code == this->TypeCode());
}


status_t
BParameterWeb::Unflatten(type_code c,
						 const void *buf,
						 ssize_t size)
{	
	if(!this->AllowsTypeCode(c))
		return B_BAD_TYPE;
	
	if(buf == NULL)
		return B_NO_INIT;
	
	//if the buffer is smaller than the size needed to read the
	//signature field, the mystery field, the group count, and the Node, then there is a problem
	if(size < static_cast<ssize_t>(sizeof(int32) + sizeof(int32) + sizeof(ssize_t) + sizeof(media_node)) )
	{
		return B_ERROR;
	}

	const byte *CurrentPos = static_cast<const byte *>(buf);
	
	//QUESTION: I have no idea where this magic number came from, and i'm not sure that it's
	//being read in the correct byte order.
	if( *(reinterpret_cast<const int32 *>(CurrentPos)) != 0x010300506)
	{
		return B_BAD_TYPE;
	}
	CurrentPos += sizeof(int32);
	
	//QUESTION: I have no idea where this magic number came from, and i'm not sure that it's
	//being read in the correct byte order.  THIS SURELY HAS SOME MEANING I'M NOT AWARE OF.
	if( *(reinterpret_cast<const int32 *>(CurrentPos)) != 1)
	{
		return B_ERROR;
	}
	CurrentPos += sizeof(int32);
	
	
	//this variable is used to cap lengths/sizes read from the flattened buffer
	//to maximum reasonable sizes to ensure that we don't run off the buffer.
	int MaxByteLength;
	int i;
	
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
	
	
	
	//read NumGroups
	int NumItems = *(reinterpret_cast<const ssize_t *>(CurrentPos));
	CurrentPos += sizeof(ssize_t);
	
	//MaxByteLength = size - ((offset into struct) + (minimum bytes REQUIRED AFTER name string))
	//In this case, the fields REQUIRED after the group list are:
	//(none)
	//TOTAL: 0 bytes
	MaxByteLength = size - (((CurrentPos + sizeof(media_node)) - static_cast<byte *>(buf)) + 0);
	
	ssize_t MinFlattenedItemSize = 8;
	
	//each item occupies a minimum of 8 bytes, so make sure that there is enough
	//space remaining in the buffer for the specified NumItems (assuming each is minimum size)
	NumItems = min_c(NumItems,MaxByteLength/MinFlattenedItemSize);
	
	
	
	//read Node
	mNode = *(reinterpret_cast<const media_node *>(CurrentPos));
	CurrentPos += sizeof(media_node);
	
	
	status_t RetVal = B_OK;
	
	for(i = 0; i < NumItems; i++)
	{
		//read the flattened size of this item
		ssize_t SubGroupSize = *(reinterpret_cast<const ssize_t *>(CurrentPos));
		CurrentPos += sizeof(ssize_t);
		
		//MaxByteLength = size - ((current offset into struct) + (minimum bytes REQUIRED AFTER name string))
		//In this case, the fields REQUIRED after the group list are:
		//NumGroups*(4 bytes)
		//TOTAL: NumGroups*(4 bytes) bytes
		MaxByteLength = size - ((CurrentPos - static_cast<byte *>(buf)) + (NumItems*4));
		
		//make sure that the SubGroupSize cannot overflow the buffer we are reading out of
		SubGroupSize = min_c(SubGroupSize,MaxByteLength);
		
		
		BParameterGroup *NewSubGroup = new BParameterGroup(this,"New_UnNamed_SubGroup");
		
		RetVal = NewSubGroup->Unflatten(NewSubGroup->TypeCode(),CurrentPos,SubGroupSize);
		
		if(RetVal != B_OK)
		{
			delete NewSubGroup;
			//don't return, because we should still fix references...
			break;
		}
		
		CurrentPos += SubGroupSize;
		
		//add the item to the list
		mGroups->AddItem(NewSubGroup);
		
	}
	
	
	//fix all references
	if((mOldRefs != NULL) && (mNewRefs != NULL))
	{
		int limit = this->CountParameters();
		for(i = 0; i < limit; i++)
		{
			BParameter *CurrentParam = this->ParameterAt(i);
			if(CurrentParam != NULL)
			{
				CurrentParam->FixRefs(*mOldRefs,*mNewRefs);
			}
		}
	
		this->mOldRefs->MakeEmpty();
		this->mNewRefs->MakeEmpty();
	}
	

	return RetVal;
}

/*************************************************************
 * private BParameterWeb
 *************************************************************/

/*
unimplemented
BParameterWeb::BParameterWeb(const BParameterWeb &clone)
BParameterWeb &BParameterWeb::operator=(const BParameterWeb &clone)
*/

status_t BParameterWeb::_Reserved_ControlWeb_0(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_1(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_2(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_3(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_4(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_5(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_6(void *) { return B_ERROR; }
status_t BParameterWeb::_Reserved_ControlWeb_7(void *) { return B_ERROR; }

void
BParameterWeb::AddRefFix(void *oldItem,
						 void *newItem)
{
	ASSERT_RETURN(mOldRefs != NULL);
	ASSERT_RETURN(mNewRefs != NULL);
	
	mOldRefs->AddItem(oldItem);
	mNewRefs->AddItem(newItem);
}

/*************************************************************
 * private BParameterGroup
 *************************************************************/

BParameterGroup::BParameterGroup(BParameterWeb *web,
								 const char *name):mWeb(web)
{
	mControls = new BList();
	mGroups = new BList();
	
	int NameLength = 0;
	if(name != NULL)
	{
		NameLength = strlen(name);
	}
	mName = new char[NameLength + 1];
	memcpy(mName,name,NameLength);
	mName[NameLength] = 0;
	
	mFlags = 0;
}


BParameterGroup::~BParameterGroup()
{
	int i;

	int NumItems;
	void **Items;

	if(mControls != NULL)
	{
		NumItems = mControls->CountItems();
		Items = static_cast<void **>(mControls->Items());

		for(i = 0; i < NumItems; i++)
		{
			if(Items[i] != NULL)
			{
				delete Items[i];
				Items[i] = NULL;
			}
		}
	}
	
	if(mGroups != NULL)
	{
		NumItems = mGroups->CountItems();
		Items = static_cast<void **>(mControls->Items());
		
		for(i = 0; i < NumItems; i++)
		{
			if(Items[i] != NULL)
			{
				delete Items[i];
				Items[i] = NULL;
			}
		}
	}

	if(mName != NULL)
	{
		delete[] mName;
		mName = NULL;
	}
	
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
BParameterGroup::MakeNullParameter(int32 id,
								   media_type m_type,
								   const char *name,
								   const char *kind)
{
	ASSERT_WRETURN_VALUE(mControls != NULL,NULL);
	
	BNullParameter *NewParam = new BNullParameter(id,m_type,mWeb,name,kind);
	
	NewParam->mGroup = this;
	
	mControls->AddItem(NewParam);
	
	return NewParam;
}


BContinuousParameter *
BParameterGroup::MakeContinuousParameter(int32 id,
										 media_type m_type,
										 const char *name,
										 const char *kind,
										 const char *unit,
										 float minimum,
										 float maximum,
										 float stepping)
{
	ASSERT_WRETURN_VALUE(mControls != NULL,NULL);
	
	BContinuousParameter *NewParam = new BContinuousParameter(id,m_type,mWeb,name,kind,unit,minimum,maximum,stepping);
	
	NewParam->mGroup = this;
	
	mControls->AddItem(NewParam);
	
	return NewParam;
}


BDiscreteParameter *
BParameterGroup::MakeDiscreteParameter(int32 id,
									   media_type m_type,
									   const char *name,
									   const char *kind)
{
	ASSERT_WRETURN_VALUE(mControls != NULL,NULL);
	
	BDiscreteParameter *NewParam = new BDiscreteParameter(id,m_type,mWeb,name,kind);
	
	NewParam->mGroup = this;
	
	mControls->AddItem(NewParam);
	
	return NewParam;
}


BParameterGroup *
BParameterGroup::MakeGroup(const char *name)
{
	ASSERT_WRETURN_VALUE(mGroups != NULL,NULL);
	
	BParameterGroup *NewGroup = new BParameterGroup(mWeb,name);
	
	mGroups->AddItem(NewGroup);
	
	return NewGroup;
}


int32
BParameterGroup::CountParameters()
{
	ASSERT_WRETURN_VALUE(mControls != NULL,0);
	
	return mControls->CountItems();
}


BParameter *
BParameterGroup::ParameterAt(int32 index)
{
	ASSERT_WRETURN_VALUE(mControls != NULL,NULL);
	
	return static_cast<BParameter *>(mControls->ItemAt(index));
}


int32
BParameterGroup::CountGroups()
{
	ASSERT_WRETURN_VALUE(mGroups != NULL,0);
	
	return mGroups->CountItems();
}


BParameterGroup *
BParameterGroup::GroupAt(int32 index)
{
	ASSERT_WRETURN_VALUE(mGroups != NULL,NULL);
	
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
	ASSERT_WRETURN_VALUE(mControls != NULL,0);
	ASSERT_WRETURN_VALUE(mGroups != NULL,0);
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
	ssize_t RetVal = 13;
	
	if(mFlags != 0)
	{
		RetVal += 4;
	}
	
	if(mName != NULL)
	{
		RetVal += min_c(strlen(mName),255);
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
			RetVal += 16;
			RetVal += CurrentParameter->FlattenedSize();
		}
	}
	
	limit = mGroups->CountItems();
	for(i = 0; i < limit; i++)
	{
		BParameterGroup *CurrentGroup = static_cast<BParameterGroup *>(mGroups->ItemAt(i));
		if(CurrentGroup != NULL)
		{
			//overhead for each group flattened
			RetVal += 16;
			RetVal += CurrentGroup->FlattenedSize();
		}
	}
	

	return RetVal;
}


status_t
BParameterGroup::Flatten(void *buffer,
						 ssize_t size) const
{
	if(buffer == NULL)
		return B_NO_INIT;
	
	//NOTICE: It is important that this value is the size returned by BParameterGroup::FlattenedSize,
	//		  not by a descendent's override of this method.
	ssize_t ActualFSize = BParameterGroup::FlattenedSize();
	
	if(size < ActualFSize)
		return B_NO_MEMORY;
	
	byte *CurrentPos = static_cast<byte *>(buffer);
	
	//QUESTION: I have no idea where this magic number came from, and i'm not sure that it's
	//being written in the correct byte order.
	if(mFlags == 0)
	{
		*(reinterpret_cast<int32 *>(CurrentPos)) = 0x03040507;
		CurrentPos += sizeof(int32);
	}
	else
	{
		*(reinterpret_cast<int32 *>(CurrentPos)) = 0x03040509;
		CurrentPos += sizeof(int32);
		
		*(reinterpret_cast<uint32 *>(CurrentPos)) = mFlags;
		CurrentPos += sizeof(uint32);
	}
	
	//flatten and write the name string
	byte NameStringLength = 0;
	if(mName != NULL)
	{
		NameStringLength = min_c(strlen(mName),255);
	}
	*(reinterpret_cast<byte *>(CurrentPos)) = NameStringLength;
	CurrentPos += sizeof(byte);
	
	memcpy(CurrentPos,mName,NameStringLength);
	CurrentPos += NameStringLength;
	

	int i;
	int NumItems;
	void **Items;

	ssize_t *TotalWrittenParamsCount = reinterpret_cast<ssize_t *>(CurrentPos);
	(*TotalWrittenParamsCount) = 0;
	CurrentPos += sizeof(ssize_t);
	
	if(mControls != NULL)
	{
		NumItems = mControls->CountItems();
		Items = static_cast<void **>(mControls->Items());
		
		for(i = 0; i < NumItems; i++)
		{
			BParameter *CurrentParam = static_cast<BParameter *>(Items[i]);
			if(CurrentParam != NULL)
			{
				//write the pointer value
				*(reinterpret_cast<BParameter **>(CurrentPos)) = CurrentParam;
				CurrentPos += sizeof(BParameter *);
				
				//write the type value
				*(reinterpret_cast<BParameter::media_parameter_type *>(CurrentPos)) = CurrentParam->Type();
				CurrentPos += sizeof(BParameter::media_parameter_type);
				
				ssize_t FlattenedParamSize = CurrentParam->FlattenedSize();
				
				//write the flattened size value
				*(reinterpret_cast<ssize_t *>(CurrentPos)) = FlattenedParamSize;
				CurrentPos += sizeof(ssize_t);
				
				//write the flattened parameter
				status_t ParamFlattenStatus = CurrentParam->Flatten(CurrentPos,FlattenedParamSize);
				
				if(ParamFlattenStatus != B_OK)
				{
					return ParamFlattenStatus;
				}
				
				CurrentPos += FlattenedParamSize;
				
				(*TotalWrittenParamsCount)++;
			}
		}
	}
	
	ssize_t *TotalWrittenSubGroupsCount = reinterpret_cast<ssize_t *>(CurrentPos);
	(*TotalWrittenSubGroupsCount) = 0;
	CurrentPos += sizeof(ssize_t);
	
	if(mGroups != NULL)
	{
		NumItems = mGroups->CountItems();
		Items = static_cast<void **>(mGroups->Items());
		
		for(i = 0; i < NumItems; i++)
		{
			BParameterGroup *CurrentSubGroup = static_cast<BParameterGroup *>(Items[i]);
			if(CurrentSubGroup != NULL)
			{
				//write the pointer value
				*(reinterpret_cast<BParameterGroup **>(CurrentPos)) = CurrentSubGroup;
				CurrentPos += sizeof(BParameterGroup *);
				
				//write the type code value
				*(reinterpret_cast<type_code *>(CurrentPos)) = CurrentSubGroup->TypeCode();
				CurrentPos += sizeof(type_code);
				
				ssize_t FlattenedSubGroupSize = CurrentSubGroup->FlattenedSize();
				
				//write the flattened size value
				*(reinterpret_cast<ssize_t *>(CurrentPos)) = FlattenedSubGroupSize;
				CurrentPos += sizeof(ssize_t);
				
				//write the flattened sub group
				status_t SubGroupFlattenStatus = CurrentSubGroup->Flatten(CurrentPos,FlattenedSubGroupSize);
				
				if(SubGroupFlattenStatus != B_OK)
				{
					return SubGroupFlattenStatus;
				}
				
				CurrentPos += FlattenedSubGroupSize;
				
				(*TotalWrittenSubGroupsCount)++;
			}
		}
	}

	return B_OK;
}


bool
BParameterGroup::AllowsTypeCode(type_code code) const
{
	return (code == this->TypeCode());
}


status_t
BParameterGroup::Unflatten(type_code c,
						   const void *buf,
						   ssize_t size)
{
	if(!this->AllowsTypeCode(c))
		return B_BAD_TYPE;
	
	if(buf == NULL)
		return B_NO_INIT;
	
	//if the buffer is smaller than the size needed to read the
	//signature field, then there is a problem
	if(size < static_cast<ssize_t>(sizeof(int32)) )
	{
		return B_ERROR;
	}

	const byte *CurrentPos = static_cast<const byte *>(buf);
	
	uint32 Flags = 0;
	
	//QUESTION: I have no idea where this magic number came from, and i'm not sure that it's
	//being read in the correct byte order.
	if( *(reinterpret_cast<const int32 *>(CurrentPos)) == 0x03040507)
	{
		CurrentPos += sizeof(int32);
	}
	else if( *(reinterpret_cast<const int32 *>(CurrentPos)) == 0x03040509)
	{
		CurrentPos += sizeof(int32);
		
		//check to make sure we've got room to read the flags field
		if(size < static_cast<ssize_t>(sizeof(int32) + sizeof(uint32)))
		{
			return B_ERROR;
		}
		
		Flags = *(reinterpret_cast<const uint32 *>(CurrentPos));
		CurrentPos += sizeof(uint32);
	}
	else
	{
		return B_BAD_TYPE;
	}
	
	this->mFlags = Flags;
	
	
	
	
	//this variable is used to cap lengths/sizes read from the flattened buffer
	//to maximum reasonable sizes to ensure that we don't run off the buffer.
	int32 MaxByteLength;
	
	//read the name string
	byte NameStringLength = *(reinterpret_cast<const byte *>(CurrentPos));
	CurrentPos += sizeof(byte);
	
	//MaxByteLength = size - ((current offset into struct) + (minimum bytes REQUIRED AFTER name string))
	//In this case, the fields REQUIRED after the name string are:
	//Param Count (4 bytes)
	//Subgroup Count (4 bytes)
	//TOTAL: 8 bytes
	MaxByteLength = size - ((CurrentPos - static_cast<byte *>(buf)) + 8);
	
	NameStringLength = min_c(NameStringLength,MaxByteLength);
	
	if(mName != NULL)
	{
		delete[] mName;
		mName = NULL;
	}
	mName = new char[NameStringLength + 1];
	memcpy(mName,CurrentPos,NameStringLength);
	mName[NameStringLength] = 0;
	CurrentPos += NameStringLength;
	
	
	
	
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
	
	
	
	//read NumParameters
	ssize_t NumItems = *(reinterpret_cast<const ssize_t *>(CurrentPos));
	CurrentPos += sizeof(ssize_t);
	
	//MaxByteLength = size - ((current offset into struct) + (minimum bytes REQUIRED AFTER name string))
	//In this case, the fields REQUIRED after the name string are:
	//Subgroup Count (4 bytes)
	//TOTAL: 4 bytes
	MaxByteLength = size - ((CurrentPos - static_cast<byte *>(buf)) + 4);
	
	ssize_t MinFlattenedItemSize(12);
	
	//each item occupies a minimum of 12 bytes, so make sure that there is enough
	//space remaining in the buffer for the specified NumItems (assuming each is minimum size)
	NumItems = min_c(NumItems,MaxByteLength/MinFlattenedItemSize);
	
	
	for(i = 0; i < NumItems; i++)
	{
		//read the old pointer value of this item
		BParameter *OldPointerVal = *(reinterpret_cast<BParameter *const*>(CurrentPos));
		CurrentPos += sizeof(BParameter *);
		
		//read the media_parameter_type of this item
		BParameter::media_parameter_type ParamType = *(reinterpret_cast<const BParameter::media_parameter_type *>(CurrentPos));
		CurrentPos += sizeof(BParameter::media_parameter_type);
		
		//read the flattened size of this item
		ssize_t ParamSize = *(reinterpret_cast<const ssize_t *>(CurrentPos));
		CurrentPos += sizeof(ssize_t);
		
		//MaxByteLength = size - ((current offset into struct) + (minimum bytes REQUIRED AFTER name string))
		//In this case, the fields REQUIRED after the name string are:
		//Subgroup Count (4 bytes)
		//TOTAL: 4 bytes
		MaxByteLength = size - ((CurrentPos - static_cast<byte *>(buf)) + 4);
		
		//make sure that the ParamSize cannot overflow the buffer we are reading out of
		ParamSize = min_c(ParamSize,MaxByteLength);
		
		BParameter *NewParam = this->MakeControl(ParamType);
		
		//need to be careful because ParamType could be invalid
		if(NewParam == NULL)
		{
			return B_ERROR;
		}
		
		status_t RetVal = NewParam->Unflatten(NewParam->TypeCode(),CurrentPos,ParamSize);
		
		if(RetVal != B_OK)
		{
			delete NewParam;
			return RetVal;
		}
		
		CurrentPos += ParamSize;
		
		//add the item to the list
		mControls->AddItem(NewParam);
		
		//add it's old pointer value to the RefFix list kept by the owner web
		if(mWeb != NULL)
		{
			mWeb->AddRefFix(OldPointerVal,NewParam);
		}
	}

	
	
	
	
	//read NumSubGroups
	NumItems = *(reinterpret_cast<const ssize_t *>(CurrentPos));
	CurrentPos += sizeof(ssize_t);
	
	//MaxByteLength = size - ((current offset into struct) + (minimum bytes REQUIRED AFTER name string))
	//In this case, the fields REQUIRED after the name string are:
	//(none)
	//TOTAL: 0 bytes
	MaxByteLength = size - ((CurrentPos - static_cast<byte *>(buf)) + 0);
	
	MinFlattenedItemSize = 12;
	
	//each item occupies a minimum of 12 bytes, so make sure that there is enough
	//space remaining in the buffer for the specified NumItems (assuming each is minimum size)
	NumItems = min_c(NumItems,MaxByteLength/MinFlattenedItemSize);
	
	
	for(i = 0; i < NumItems; i++)
	{
		//read the old pointer value of this item
		BParameter *OldPointerVal = *(reinterpret_cast<BParameter *const*>(CurrentPos));
		CurrentPos += sizeof(BParameter *);
		
		//read the type_code of this item
		type_code BufTypeCode = *(reinterpret_cast<const type_code *>(CurrentPos));
		CurrentPos += sizeof(type_code);
		
		//read the flattened size of this item
		ssize_t SubGroupSize = *(reinterpret_cast<const ssize_t *>(CurrentPos));
		CurrentPos += sizeof(ssize_t);
		
		//MaxByteLength = size - ((current offset into struct) + (minimum bytes REQUIRED AFTER name string))
		//In this case, the fields REQUIRED after the name string are:
		//(none)
		//TOTAL: 0 bytes
		MaxByteLength = size - ((CurrentPos - static_cast<byte *>(buf)) + 0);
		
		//make sure that the SubGroupSize cannot overflow the buffer we are reading out of
		SubGroupSize = min_c(SubGroupSize,MaxByteLength);
		
		
		BParameterGroup *NewSubGroup = new BParameterGroup(mWeb,"New_UnNamed_SubGroup");
		
		status_t RetVal = NewSubGroup->Unflatten(BufTypeCode,CurrentPos,SubGroupSize);
		
		if(RetVal != B_OK)
		{
			delete NewSubGroup;
			return RetVal;
		}
		
		CurrentPos += SubGroupSize;
		
		//add the item to the list
		mGroups->AddItem(NewSubGroup);
		
		//add it's old pointer value to the RefFix list kept by the owner web
		if(mWeb != NULL)
		{
			mWeb->AddRefFix(OldPointerVal,NewSubGroup);
		}
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

status_t BParameterGroup::_Reserved_ControlGroup_0(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_1(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_2(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_3(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_4(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_5(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_6(void *) { return B_ERROR; }
status_t BParameterGroup::_Reserved_ControlGroup_7(void *) { return B_ERROR; }


BParameter *
BParameterGroup::MakeControl(int32 type)
{
	/*NOTE:
	  Creates a new parameter for addition within this with a type defined by the passed 'type' parameter,
	  BUT DOES NOT ADD THE CREATED PARAMETER TO THE INTERNAL LIST OF PARAMETERS
	*/
	switch(type)
	{
		case(BParameter::B_NULL_PARAMETER):
		{
			return new BNullParameter(-1,B_MEDIA_UNKNOWN_TYPE,mWeb,"New_UnNamed_NullParameter",B_GENERIC);
		}
		break;
		case(BParameter::B_DISCRETE_PARAMETER):
		{
			return new BDiscreteParameter(-1,B_MEDIA_UNKNOWN_TYPE,mWeb,"New_UnNamed_DiscreteParameter",B_GENERIC);
		}
		break;
		case(BParameter::B_CONTINUOUS_PARAMETER):
		{
			return new BContinuousParameter(-1,B_MEDIA_UNKNOWN_TYPE,mWeb,"New_UnNamed_ContinuousParameter",B_GENERIC,"",0,100,1);
		}
		break;
		default:
		{
			return NULL;
		}
		break;
	}
	return NULL;
}

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
BParameter::GetValue(void *buffer,
					 size_t *ioSize,
					 bigtime_t *when)
{
	UNIMPLEMENTED();
	/*
	 * XXX FIXME! call BControllable::GetControlValue() here.
	 */
	return B_BAD_VALUE;
}


status_t
BParameter::SetValue(const void *buffer,
					 size_t size,
					 bigtime_t when)
{
	UNIMPLEMENTED();
	/*
	 * XXX FIXME! call BControllable::SetControlValue() here.
	 */
	return B_BAD_VALUE;
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
	ASSERT_WRETURN_VALUE(mInputs != NULL,0);

	return mInputs->CountItems();
}


BParameter *
BParameter::InputAt(int32 index)
{
	ASSERT_WRETURN_VALUE(mInputs != NULL,NULL);
	
	return static_cast<BParameter *>(mInputs->ItemAt(index));
}


void
BParameter::AddInput(BParameter *input)
{
	// BeBook has this method returning a status value,
	// but it should be updated
	if(input == NULL)
	{
		return;
	}
	
	ASSERT_RETURN(mInputs != NULL);
	
	if(mInputs->HasItem(input))
	{
		//if already in input list, don't duplicate.
		return;
	}
	
	mInputs->AddItem(input);
	input->AddOutput(this);
}


int32
BParameter::CountOutputs()
{
	ASSERT_WRETURN_VALUE(mOutputs != NULL,0);

	return mOutputs->CountItems();
}


BParameter *
BParameter::OutputAt(int32 index)
{
	ASSERT_WRETURN_VALUE(mOutputs != NULL,NULL);
	
	return static_cast<BParameter *>(mOutputs->ItemAt(index));
}


void
BParameter::AddOutput(BParameter *output)
{
	// BeBook has this method returning a status value,
	// but it should be updated
	if(output == NULL)
	{
		return;
	}
	
	ASSERT_RETURN(mOutputs != NULL);
	
	if(mOutputs->HasItem(output))
	{
		//if already in output list, don't duplicate.
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
	ssize_t RetVal = 35;
	
	if(mName != NULL) RetVal += strlen(mName);
	
	if(mKind != NULL) RetVal += strlen(mKind);
	
	if(mUnit != NULL) RetVal += strlen(mUnit);
	
	if(mInputs != NULL) RetVal += mInputs->CountItems()*sizeof(BParameter *);
	
	if(mOutputs != NULL) RetVal += mOutputs->CountItems()*sizeof(BParameter *);

	return RetVal;
}


status_t
BParameter::Flatten(void *buffer,
					ssize_t size) const
{
	if(buffer == NULL)
		return B_NO_INIT;
	
	//NOTICE: It is important that this value is the size returned by BParameter::FlattenedSize,
	//		  not by a descendent's override of this method.
	ssize_t ActualFSize = BParameter::FlattenedSize();
	
	if(size < ActualFSize)
		return B_NO_MEMORY;
	
	byte *CurrentPos = static_cast<byte *>(buffer);
	
	//QUESTION: I have no idea where this magic number came from, and i'm not sure that it's
	//being written in the correct byte order.
	*(reinterpret_cast<int32 *>(CurrentPos)) = 0x02040607;
	CurrentPos += sizeof(int32);
	
	//flatten and write the struct size
	*(reinterpret_cast<ssize_t *>(CurrentPos)) = ActualFSize;
	CurrentPos += sizeof(ssize_t);
	
	//flatten and write the ID
	*(reinterpret_cast<int32 *>(CurrentPos)) = mID;
	CurrentPos += sizeof(int32);
	
	//flatten and write the name string
	byte NameStringLength = 0;
	if(mName != NULL)
	{
		NameStringLength = min_c(strlen(mName),255);
	}
	*(reinterpret_cast<byte *>(CurrentPos)) = NameStringLength;
	CurrentPos += sizeof(byte);
	
	memcpy(CurrentPos,mName,NameStringLength);
	CurrentPos += NameStringLength;
	
	//flatten and write the kind string
	byte KindStringLength = 0;
	if(mKind != NULL)
	{
		KindStringLength = min_c(strlen(mKind),255);
	}
	*(reinterpret_cast<byte *>(CurrentPos)) = KindStringLength;
	CurrentPos += sizeof(byte);
	
	memcpy(CurrentPos,mKind,KindStringLength);
	CurrentPos += KindStringLength;
	
	//flatten and write the unit string
	byte UnitStringLength = 0;
	if(mUnit != NULL)
	{
		UnitStringLength = min_c(strlen(mUnit),255);
	}
	*(reinterpret_cast<byte *>(CurrentPos)) = UnitStringLength;
	CurrentPos += sizeof(byte);
	
	memcpy(CurrentPos,mUnit,UnitStringLength);
	CurrentPos += UnitStringLength;
	
	
	//flatten and write the list of inputs
	ssize_t NumInputs = 0;
	if(mInputs != NULL)
	{
		NumInputs = mInputs->CountItems();
	}
	*(reinterpret_cast<ssize_t *>(CurrentPos)) = NumInputs;
	CurrentPos += sizeof(ssize_t);
	
	memcpy(CurrentPos,mInputs->Items(),sizeof(BParameter *)*NumInputs);
	
	
	//flatten and write the list of outputs
	ssize_t NumOutputs = 0;
	if(mOutputs != NULL)
	{
		NumOutputs = mOutputs->CountItems();
	}
	*(reinterpret_cast<ssize_t *>(CurrentPos)) = NumOutputs;
	CurrentPos += sizeof(ssize_t);
	
	memcpy(CurrentPos,mOutputs->Items(),sizeof(BParameter *)*NumOutputs);
	

	//flatten and write the media type
	*(reinterpret_cast<media_type *>(CurrentPos)) = mMediaType;
	CurrentPos += sizeof(media_type);
	
	//flatten and write the channel count
	*(reinterpret_cast<int32 *>(CurrentPos)) = mChannels;
	CurrentPos += sizeof(int32);
	
	//flatten and write the flags
	*(reinterpret_cast<uint32 *>(CurrentPos)) = mFlags;
	CurrentPos += sizeof(uint32);
	

	return B_OK;
}


bool
BParameter::AllowsTypeCode(type_code code) const
{
	return (code == this->TypeCode());
}


status_t
BParameter::Unflatten(type_code c,
					  const void *buf,
					  ssize_t size)
{

	if(!this->AllowsTypeCode(c))
		return B_BAD_TYPE;
	
	if(buf == NULL)
		return B_NO_INIT;
	
	//if the buffer is smaller than the size needed to read the
	//signature and struct size fields, then there is a problem
	if(size < static_cast<ssize_t>(sizeof(int32) + sizeof(ssize_t)))
	{
		return B_ERROR;
	}

	const byte *CurrentPos = static_cast<const byte *>(buf);
	
	
	//QUESTION: I have no idea where this magic number came from, and i'm not sure that it's
	//being read in the correct byte order.
	if( *(reinterpret_cast<const int32 *>(CurrentPos)) != 0x02040607)
	{
		return B_BAD_TYPE;
	}
	CurrentPos += sizeof(int32);
	
	//read the struct size
	ssize_t ParamStructSize = *(reinterpret_cast<const ssize_t *>(CurrentPos));
	CurrentPos += sizeof(ssize_t);
	
	if(ParamStructSize > size)
	{
		//if the struct size is larger than the size of the buffer we were given,
		//there's a problem
		return B_MISMATCHED_VALUES;
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
	if(ParamStructSize < MinFlattenedParamSize)
	{
		return B_ERROR;
	}

	//read the ID
	this->mID = *(reinterpret_cast<const int32 *>(CurrentPos));
	CurrentPos += sizeof(int32);
	
	//this variable is used to cap lengths/sizes read from the flattened buffer
	//to maximum reasonable sizes to ensure that we don't run off the buffer.
	int32 MaxByteLength;
	
	//read the name string
	byte NameStringLength = *(reinterpret_cast<const byte *>(CurrentPos));
	CurrentPos += sizeof(byte);
	
	//MaxByteLength = ParamStructSize - ((current offset into struct) + (minimum bytes REQUIRED AFTER name string))
	//In this case, the fields REQUIRED after the name string are:
	//Kind String Length (1 byte)
	//Unit String Length (1 byte)
	//Inputs Count (4 bytes)
	//Outputs Count (4 bytes)
	//Media Type (4 bytes)
	//Channel Count (4 bytes)
	//Flags (4 bytes)
	//TOTAL: 22 bytes
	MaxByteLength = ParamStructSize - ((CurrentPos - static_cast<byte *>(buf)) + 22);
	
	NameStringLength = min_c(NameStringLength,MaxByteLength);
	
	if(mName != NULL)
	{
		delete[] mName;
		mName = NULL;
	}
	mName = new char[NameStringLength + 1];
	memcpy(mName,CurrentPos,NameStringLength);
	mName[NameStringLength] = 0;
	CurrentPos += NameStringLength;
	
	//read the kind string
	byte KindStringLength = *(reinterpret_cast<const byte *>(CurrentPos));
	CurrentPos += sizeof(byte);
	
	//MaxByteLength = ParamStructSize - ((current offset into struct) + (minimum bytes REQUIRED AFTER kind string))
	//In this case, the fields REQUIRED after the kind string are:
	//Unit String Length (1 byte)
	//Inputs Count (4 bytes)
	//Outputs Count (4 bytes)
	//Media Type (4 bytes)
	//Channel Count (4 bytes)
	//Flags (4 bytes)
	//TOTAL: 21 bytes
	MaxByteLength = ParamStructSize - ((CurrentPos - static_cast<byte *>(buf)) + 21);
	
	KindStringLength = min_c(KindStringLength,MaxByteLength);
	
	if(mKind != NULL)
	{
		delete[] mKind;
		mKind = NULL;
	}
	mKind = new char[KindStringLength + 1];
	memcpy(mKind,CurrentPos,KindStringLength);
	mKind[KindStringLength] = 0;
	CurrentPos += KindStringLength;
	
	//read the unit string
	byte UnitStringLength = *(reinterpret_cast<const byte *>(CurrentPos));
	CurrentPos += sizeof(byte);
	
	//MaxByteLength = ParamStructSize - ((current offset into struct) + (minimum bytes REQUIRED AFTER unit string))
	//In this case, the fields REQUIRED after the unit string are:
	//Inputs Count (4 bytes)
	//Outputs Count (4 bytes)
	//Media Type (4 bytes)
	//Channel Count (4 bytes)
	//Flags (4 bytes)
	//TOTAL: 20 bytes
	MaxByteLength = ParamStructSize - ((CurrentPos - static_cast<byte *>(buf)) + 20);
	
	UnitStringLength = min_c(UnitStringLength,MaxByteLength);
	
	if(mUnit != NULL)
	{
		delete[] mUnit;
		mUnit = NULL;
	}
	mUnit = new char[UnitStringLength + 1];
	memcpy(mUnit,CurrentPos,UnitStringLength);
	mUnit[UnitStringLength] = 0;
	CurrentPos += UnitStringLength;
	
	//Set this flag to false to indicate that the pointer values in this parameter, have
	//not been swapped.
	mSwapDetected = false;
	
	//read the list of inputs
	int i,j;
	
	ssize_t NumInputs = *(reinterpret_cast<const ssize_t *>(CurrentPos));
	CurrentPos += sizeof(ssize_t);
	
	
	//MaxByteLength = ParamStructSize - ((current offset into struct) + (minimum bytes REQUIRED AFTER inputs list))
	//In this case, the fields REQUIRED after the inputs list are:
	//Outputs Count (4 bytes)
	//Media Type (4 bytes)
	//Channel Count (4 bytes)
	//Flags (4 bytes)
	//TOTAL: 16 bytes
	MaxByteLength = ParamStructSize - ((CurrentPos - static_cast<byte *>(buf)) + 16);
	
	NumInputs = min_c(NumInputs,static_cast<ssize_t>(MaxByteLength/sizeof(BParameter *)));
	
	
	if(this->mInputs == NULL)
	{
		this->mInputs = new BList();
	}
	else
	{
		//if this object has an existing list of (valid) inputs, go to each one, removing this object
		//as an output for each of the objects in the input list, then clear the list
		if(mSwapDetected)
		{
			ssize_t OldInputCount = mInputs->CountItems();
			for(i = 0; i < OldInputCount; i++)
			{
				BParameter *CurrentParam = static_cast<BParameter *>(this->mInputs->ItemAt(i));
				if((CurrentParam != NULL) && (CurrentParam->mOutputs != NULL))
				{
					//Remove ALL instances of this parameter from the other parameter's
					//output list
					j = 0;
					ssize_t CurrentParamsOutputCount = CurrentParam->mOutputs->CountItems();
					while(j < CurrentParamsOutputCount)
					{
						if(CurrentParam->mOutputs->ItemAt(j) == this)
						{
							//remove this item, update the CurrentParamsOutputCount,
							//and DON'T increment j
							CurrentParam->mOutputs->RemoveItem(j);
							CurrentParamsOutputCount--;
						}
						else
						{
							//move on to the next one
							j++;
						}
					}
				}
			}
		}
		this->mInputs->MakeEmpty();
	}
	
	for(i = 0; i < NumInputs; i++)
	{
		this->AddInput(*(reinterpret_cast<BParameter * const *>(CurrentPos)));
		CurrentPos += sizeof(BParameter *);
	}
	
	
	//read the list of outputs
	ssize_t NumOutputs = *(reinterpret_cast<const ssize_t *>(CurrentPos));
	CurrentPos += sizeof(ssize_t);
	
	
	//MaxByteLength = ParamStructSize - ((current offset into struct) + (minimum bytes REQUIRED AFTER outputs list))
	//In this case, the fields REQUIRED after the outputs list are:
	//Media Type (4 bytes)
	//Channel Count (4 bytes)
	//Flags (4 bytes)
	//TOTAL: 12 bytes
	MaxByteLength = ParamStructSize - ((CurrentPos - static_cast<byte *>(buf)) + 12);
	
	NumOutputs = min_c(NumOutputs,static_cast<ssize_t>(MaxByteLength/sizeof(BParameter *)));
	
	
	if(this->mOutputs == NULL)
	{
		this->mOutputs = new BList();
	}
	else
	{
		//if this object has an existing list of (valid) outputs, go to each one, removing this object
		//as an input for each of the objects in the output list, then clear the list
		if(mSwapDetected)
		{
			ssize_t OldOutputCount = mOutputs->CountItems();
			for(i = 0; i < OldOutputCount; i++)
			{
				BParameter *CurrentParam = static_cast<BParameter *>(this->mOutputs->ItemAt(i));
				if((CurrentParam != NULL) && (CurrentParam->mInputs != NULL))
				{
					//Remove ALL instances of this parameter from the other parameter's
					//input list
					j = 0;
					ssize_t CurrentParamsInputCount = CurrentParam->mInputs->CountItems();
					while(j < CurrentParamsInputCount)
					{
						if(CurrentParam->mInputs->ItemAt(j) == this)
						{
							//remove this item, update the CurrentParamsInputCount,
							//and DON'T increment j
							CurrentParam->mInputs->RemoveItem(j);
							CurrentParamsInputCount--;
						}
						else
						{
							//move on to the next one
							j++;
						}
					}
				}
			}
		}
		this->mOutputs->MakeEmpty();
	}
	
	for(i = 0; i < NumOutputs; i++)
	{
		this->AddOutput(*(reinterpret_cast<BParameter * const *>(CurrentPos)));
		CurrentPos += sizeof(BParameter *);
	}
	
	//read the media type
	this->mMediaType = *(reinterpret_cast<const media_type *>(CurrentPos));
	CurrentPos += sizeof(media_type);
	
	//read the channel count
	this->mChannels = *(reinterpret_cast<const int32 *>(CurrentPos));
	CurrentPos += sizeof(int32);

	//read the flags
	this->mFlags = *(reinterpret_cast<const uint32 *>(CurrentPos));
	CurrentPos += sizeof(uint32);

	return B_OK;
}

/*************************************************************
 * private BParameter
 *************************************************************/


status_t BParameter::_Reserved_Control_0(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_1(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_2(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_3(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_4(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_5(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_6(void *) { return B_ERROR; }
status_t BParameter::_Reserved_Control_7(void *) { return B_ERROR; }


BParameter::BParameter(int32 id,
					   media_type m_type,
					   media_parameter_type type,
					   BParameterWeb *web,
					   const char *name,
					   const char *kind,
					   const char *unit):mID(id),mType(type),mWeb(web),
					   					 mGroup(NULL),mSwapDetected(true),mMediaType(m_type),mChannels(1),mFlags(0)
{
	mGroup = NULL;
	
	//copy the name string
	if(name == NULL)
	{
		mName = new char[1];
		mName[0] = 0;
	}
	else
	{
		ssize_t NewNameLength = strlen(name);
		mName = new char[NewNameLength + 1];
		mName[NewNameLength] = 0;
	}
	//copy the kind string
	if(kind == NULL)
	{
		mKind = new char[1];
		mKind[0] = 0;
	}
	else
	{
		ssize_t NewKindLength = strlen(kind);
		mKind = new char[NewKindLength + 1];
		mKind[NewKindLength] = 0;
	}
	//copy the unit string
	if(unit == NULL)
	{
		mUnit = new char[1];
		mUnit[0] = 0;
	}
	else
	{
		ssize_t NewUnitLength = strlen(unit);
		mUnit = new char[NewUnitLength + 1];
		mUnit[NewUnitLength] = 0;
	}
	
	//create an empty input list
	mInputs = new BList();
	
	//create an empty output list
	mOutputs = new BList();
	
}


BParameter::~BParameter()
{
	//don't worry about the mWeb/mGroup properties, you don't need
	//to remove yourself from a web/group since the only way in which
	//a parameter is destroyed is when the owner web/group destroys it
	
	if(mName != NULL)
	{
		delete[] mName;
		mName = NULL;
	}
	if(mKind != NULL)
	{
		delete[] mKind;
		mKind = NULL;
	}
	if(mUnit != NULL)
	{
		delete[] mUnit;
		mUnit = NULL;
	}
	
	int i,j;
	
	//clean up the inputs list
	if(this->mInputs != NULL)
	{
		//if this object has an existing list of (valid)inputs, go to each one, removing this object
		//as an output for each of the objects in the input list, then destroy the list
		if(mSwapDetected)
		{
			ssize_t OldInputCount = mInputs->CountItems();
			for(i = 0; i < OldInputCount; i++)
			{
				BParameter *CurrentParam = static_cast<BParameter *>(this->mInputs->ItemAt(i));
				if((CurrentParam != NULL) && (CurrentParam->mOutputs != NULL))
				{
					//Remove ALL instances of this parameter from the other parameter's
					//output list
					j = 0;
					ssize_t CurrentParamsOutputCount = CurrentParam->mOutputs->CountItems();
					while(j < CurrentParamsOutputCount)
					{
						if(CurrentParam->mOutputs->ItemAt(j) == this)
						{
							//remove this item, update the CurrentParamsOutputCount,
							//and DON'T increment j
							CurrentParam->mOutputs->RemoveItem(j);
							CurrentParamsOutputCount--;
						}
						else
						{
							//move on to the next one
							j++;
						}
					}
				}
			}
		}
		this->mInputs->MakeEmpty();
		delete mInputs;
		mInputs = NULL;
	}
	
	//clean up the outputs list
	if(this->mOutputs != NULL)
	{
		//if this object has an existing list of (valid) outputs, go to each one, removing this object
		//as an input for each of the objects in the output list, then clear the list
		if(mSwapDetected)
		{
			ssize_t OldOutputCount = mOutputs->CountItems();
			for(i = 0; i < OldOutputCount; i++)
			{
				BParameter *CurrentParam = static_cast<BParameter *>(this->mOutputs->ItemAt(i));
				if((CurrentParam != NULL) && (CurrentParam->mInputs != NULL))
				{
					//Remove ALL instances of this parameter from the other parameter's
					//input list
					j = 0;
					ssize_t CurrentParamsInputCount = CurrentParam->mInputs->CountItems();
					while(j < CurrentParamsInputCount)
					{
						if(CurrentParam->mInputs->ItemAt(j) == this)
						{
							//remove this item, update the CurrentParamsInputCount,
							//and DON'T increment j
							CurrentParam->mInputs->RemoveItem(j);
							CurrentParamsInputCount--;
						}
						else
						{
							//move on to the next one
							j++;
						}
					}
				}
			}
		}
		this->mOutputs->MakeEmpty();
		delete mOutputs;
		mOutputs = NULL;
	}
	
}


void
BParameter::FixRefs(BList &old,
					BList &updated)
{
	//Replaces references to (ie: pointers) items in the old list, with the
	//coresponding items in the updated list.
	//References are replaced in the mInputs and mOutputs lists.

	if(!mSwapDetected)
	{
		ASSERT_RETURN(mInputs);
		ASSERT_RETURN(mOutputs);
		
		int i;
		void **Items = static_cast<void **>(mInputs->Items());
		int NumItems = mInputs->CountItems();
		
		
		for(i = 0; i < NumItems; i++)
		{
			void *CurrentItem = Items[i];
			
			int32 Index = old.IndexOf(CurrentItem);
			
			if(Index >= 0)
			{
				Items[i] = updated.ItemAt(Index);
			}
		}
		
		Items = static_cast<void **>(mOutputs->Items());
		NumItems = mOutputs->CountItems();
		
		for(i = 0; i < NumItems; i++)
		{
			void *CurrentItem = Items[i];
			
			int32 Index = old.IndexOf(CurrentItem);
			
			if(Index >= 0)
			{
				Items[i] = updated.ItemAt(Index);
			}
		}
		
		mSwapDetected = true;
	}
	
}

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
BContinuousParameter::SetResponse(int resp,
								  float factor,
								  float offset)
{
	mResponse = static_cast<response>(resp);
	mFactor = factor;
	mOffset = offset;
}


void
BContinuousParameter::GetResponse(int *resp,
								  float *factor,
								  float *offset)
{
	if(resp != NULL) *resp = mResponse;
	if(factor != NULL) *factor = mFactor;
	if(offset != NULL) *offset = mOffset;
}


ssize_t
BContinuousParameter::FlattenedSize() const
{
	ssize_t RetVal = BParameter::FlattenedSize();
	
	/*
		Min: 4 bytes (as float)
		Max: 4 bytes (as float)
		Stepping: 4 bytes (as float)
		Response: 4 bytes (as int or enum)
		Factor: 4 bytes (as float)
		Offset: 4 bytes (as float)
	*/
	
	RetVal += 24;

	return RetVal;
}


status_t
BContinuousParameter::Flatten(void *buffer,
							  ssize_t size) const
{
	if(buffer == NULL)
		return B_NO_INIT;

	ssize_t TotalBParameterFlatSize = BParameter::FlattenedSize();
	//see BContinuousParameter::FlattenedSize() for a description of this value
	const ssize_t AdditionalBContParamFlatSize = 24;
	
	if(size < (TotalBParameterFlatSize + AdditionalBContParamFlatSize))
	{
		return B_NO_MEMORY;
	}

	status_t RetVal = BParameter::Flatten(buffer,size);
	
	if(RetVal != B_OK)
	{
		return RetVal;
	}
	
	byte *CurrentPos = static_cast<byte *>(buffer);
	CurrentPos += TotalBParameterFlatSize;
	
	//write out the mMinimum property
	*(reinterpret_cast<float *>(CurrentPos)) = mMinimum;
	CurrentPos += sizeof(float);
	//write out the mMaximum property
	*(reinterpret_cast<float *>(CurrentPos)) = mMaximum;
	CurrentPos += sizeof(float);
	//write out the mStepping property
	*(reinterpret_cast<float *>(CurrentPos)) = mStepping;
	CurrentPos += sizeof(float);
	//write out the mResponse property
	*(reinterpret_cast<response *>(CurrentPos)) = mResponse;
	CurrentPos += sizeof(response);
	//write out the mFactor property
	*(reinterpret_cast<float *>(CurrentPos)) = mFactor;
	CurrentPos += sizeof(float);
	//write out the mOffset property
	*(reinterpret_cast<float *>(CurrentPos)) = mOffset;
	CurrentPos += sizeof(float);
	

	return B_OK;
}


status_t
BContinuousParameter::Unflatten(type_code c,
								const void *buf,
								ssize_t size)
{

	/*
	 * NOTICE: This method tries to avoid corrupting an existing parameter
	 *		   by only reading values out of the buffer after it has verified
	 *		   that the size of the buffer is sufficient to read all needed
	 *		   fields.  In this way, we avoid having to exit this method after
	 *         the 'this' object has been modified by reading a part of the buffer.
	 */

	if(!this->AllowsTypeCode(c))
		return B_BAD_TYPE;
	
	if(buf == NULL)
		return B_NO_INIT;
	
	//if the buffer is smaller than the size needed to read the
	//signature and struct size fields, then there is a problem
	if(size < static_cast<ssize_t>(sizeof(int32) + sizeof(ssize_t)))
	{
		return B_ERROR;
	}

	const byte *CurrentPos = static_cast<const byte *>(buf);
	
	
	//QUESTION: I have no idea where this magic number came from, and i'm not sure that it's
	//being read in the correct byte order.
	if( *(reinterpret_cast<const int32 *>(CurrentPos)) != 0x02040607)
	{
		return B_BAD_TYPE;
	}
	CurrentPos += sizeof(int32);
	
	//read the struct size
	ssize_t ParamStructSize = *(reinterpret_cast<const ssize_t *>(CurrentPos));
	CurrentPos += sizeof(ssize_t);
	
	//see BContinuousParameter::FlattenedSize() for a description of this value
	const ssize_t AdditionalBContParamFlatSize = 24;
	
	if((ParamStructSize + AdditionalBContParamFlatSize) > size)
	{
		//if the struct size is larger than the size of the buffer we were given,
		//there's a problem
		return B_ERROR;
	}
	
	//read the base BParameter
	status_t RetVal = BParameter::Unflatten(c,buf,size);
	
	if(RetVal != B_OK)
	{
		return RetVal;
	}
	
	CurrentPos = static_cast<const byte *>(buf);
	CurrentPos += ParamStructSize;
	
	
	//read the mMinimum property
	mMinimum = *(reinterpret_cast<const float *>(CurrentPos));
	CurrentPos += sizeof(float);
	//read the mMaximum property
	mMaximum = *(reinterpret_cast<const float *>(CurrentPos));
	CurrentPos += sizeof(float);
	//read the mStepping property
	mStepping = *(reinterpret_cast<const float *>(CurrentPos));
	CurrentPos += sizeof(float);
	//read the mResponse property
	mResponse = *(reinterpret_cast<const response *>(CurrentPos));
	CurrentPos += sizeof(response);
	//read the mFactor property
	mFactor = *(reinterpret_cast<const float *>(CurrentPos));
	CurrentPos += sizeof(float);
	//read the mOffset property
	mOffset = *(reinterpret_cast<const float *>(CurrentPos));
	CurrentPos += sizeof(float);
	

	return B_OK;
}

/*************************************************************
 * private BContinuousParameter
 *************************************************************/

status_t BContinuousParameter::_Reserved_ContinuousParameter_0(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_1(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_2(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_3(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_4(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_5(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_6(void *) { return B_ERROR; }
status_t BContinuousParameter::_Reserved_ContinuousParameter_7(void *) { return B_ERROR; }


BContinuousParameter::BContinuousParameter(int32 id,
										   media_type m_type,
										   BParameterWeb *web,
										   const char *name,
										   const char *kind,
										   const char *unit,
										   float minimum,
										   float maximum,
										   float stepping)
	: BParameter(id,m_type,B_CONTINUOUS_PARAMETER,web,name,kind,unit),mMinimum(minimum),mMaximum(maximum),mStepping(stepping),
	  mResponse(B_LINEAR),mFactor(1.0),mOffset(0.0)
{
}


BContinuousParameter::~BContinuousParameter()
{
}

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
	ASSERT_WRETURN_VALUE(mValues != NULL,0);

	return mValues->CountItems();
}


const char *
BDiscreteParameter::ItemNameAt(int32 index)
{
	ASSERT_WRETURN_VALUE(mSelections != NULL,NULL);
	
	return reinterpret_cast<const char *>(mSelections->ItemAt(index));
}


int32
BDiscreteParameter::ItemValueAt(int32 index)
{
	ASSERT_WRETURN_VALUE(mValues != NULL,0);
	
	//check for out of range
	if((index < 0) || (index >= mValues->CountItems()))
	{
		return 0;
	}
	
	int32 *Item = static_cast<int32 *>(mValues->ItemAt(index));
	if(Item == NULL)
	{
		return 0;
	}
	
	return *Item;
}


status_t
BDiscreteParameter::AddItem(int32 value,
							const char *name)
{
	ASSERT_WRETURN_VALUE(mValues != NULL,B_ERROR);
	ASSERT_WRETURN_VALUE(mSelections != NULL,B_ERROR);
	
	int32 *NewVal = new int32(value);
	char *NewSel = NULL;
	if(name != NULL)
	{
		ssize_t NameLength = strlen(name);
		NewSel = new char[NameLength + 1];
		memcpy(NewSel,name,NameLength);
		NewSel[NameLength] = 0;
	}
	
	//QUESTION: How do we watch for the B_NO_MEMORY case (Be Book refers to this)?
	mValues->AddItem(NewVal);
	mSelections->AddItem(NewSel);

	return B_OK;
}


status_t
BDiscreteParameter::MakeItemsFromInputs()
{
	ASSERT_WRETURN_VALUE(mValues != NULL,B_ERROR);
	ASSERT_WRETURN_VALUE(mSelections != NULL,B_ERROR);
	ASSERT_WRETURN_VALUE(mInputs != NULL,B_ERROR);
	
	int32 i;
	ssize_t NumInputs = mInputs->CountItems();
	for(i = 0; i < NumInputs; i++)
	{
		BParameter *CurrentParam = static_cast<BParameter *>(mInputs->ItemAt(i));
		this->AddItem(i,CurrentParam->Name());
	}

	return B_OK;
}


status_t
BDiscreteParameter::MakeItemsFromOutputs()
{
	ASSERT_WRETURN_VALUE(mValues != NULL,B_ERROR);
	ASSERT_WRETURN_VALUE(mSelections != NULL,B_ERROR);
	ASSERT_WRETURN_VALUE(mOutputs != NULL,B_ERROR);
	
	int32 i;
	ssize_t NumOutputs = mOutputs->CountItems();
	for(i = 0; i < NumOutputs; i++)
	{
		BParameter *CurrentParam = static_cast<BParameter *>(mOutputs->ItemAt(i));
		this->AddItem(i,CurrentParam->Name());
	}

	return B_OK;
}


void
BDiscreteParameter::MakeEmpty()
{
	ASSERT_RETURN(mValues != NULL);
	ASSERT_RETURN(mSelections != NULL);
	
	int32 i;
	ssize_t ListSize = mValues->CountItems();
	for(i = 0; i < ListSize; i++)
	{
		int32 *CurrentValue = static_cast<int32 *>(mValues->ItemAt(i));
		if(CurrentValue != NULL)
		{
			delete CurrentValue;
		}
	}
	mValues->MakeEmpty();
	
	ListSize = mSelections->CountItems();
	for(i = 0; i < ListSize; i++)
	{
		char *CurrentSelection = static_cast<char *>(mSelections->ItemAt(i));
		if(CurrentSelection != NULL)
		{
			delete[] CurrentSelection;
		}
	}
	mSelections->MakeEmpty();
	
}


ssize_t
BDiscreteParameter::FlattenedSize() const
{
	ssize_t RetVal = BParameter::FlattenedSize();
	/*
		//--------BEGIN-BDISCRETEPARAMETER-STRUCT----------------
		NumItems: 4 bytes (as int)
			//for each item BEGIN
			Item Name String Length: 1 byte
				Item Name String: 'Item Name String Length' bytes
			Item Value: 4 bytes (as int)
			//for each item END
		//--------END-BDISCRETEPARAMETER-STRUCT-------------------
	*/
	RetVal += sizeof(ssize_t);
	
	ssize_t NumItems = mValues->CountItems();
	int32 i;
	for(i = 0; i < NumItems; i++)
	{
		char *CurrentSel = static_cast<char *>(mSelections->ItemAt(i));
		
		if(CurrentSel != NULL)
		{
			RetVal += min_c(strlen(CurrentSel),255);
		}
		
		//regardless of string size, there is a cost of 5 bytes for each item (string length + value)
		RetVal += 5;
	}

	return RetVal;
}


status_t
BDiscreteParameter::Flatten(void *buffer,
							ssize_t size) const
{
	if(buffer == NULL)
		return B_NO_INIT;

	ssize_t TotalBParameterFlatSize = BParameter::FlattenedSize();
	
	//see BDiscreteParameter::FlattenedSize() for a description of this value
	ssize_t AdditionalBDiscParamFlatSize = sizeof(ssize_t);
	
	ssize_t NumItems = mValues->CountItems();
	int32 i;
	for(i = 0; i < NumItems; i++)
	{
		char *CurrentSel = static_cast<char *>(mSelections->ItemAt(i));
		
		if(CurrentSel != NULL)
		{
			AdditionalBDiscParamFlatSize += min_c(strlen(CurrentSel),255);
		}
		
		//regardless of string size, there is a cost of 5 bytes for each item (string length + value)
		AdditionalBDiscParamFlatSize += 5;
	}
	
	
	if(size < (TotalBParameterFlatSize + AdditionalBDiscParamFlatSize))
	{
		return B_NO_MEMORY;
	}

	status_t RetVal = BParameter::Flatten(buffer,size);
	
	if(RetVal != B_OK)
	{
		return RetVal;
	}
	
	byte *CurrentPos = static_cast<byte *>(buffer);
	CurrentPos += TotalBParameterFlatSize;
	
	//write out the number of value/name pairs
	*(reinterpret_cast<ssize_t *>(CurrentPos)) = NumItems;
	CurrentPos += sizeof(ssize_t);
	
	//write out all value/name pairs themselves
	for(i = 0; i < NumItems; i++)
	{
		const char *CurrentSel = static_cast<char *>(mSelections->ItemAt(i));
		const int32 *CurrentVal = static_cast<int32 *>(mValues->ItemAt(i));
		
		if(CurrentSel != NULL)
		{
			byte NameLength = min_c(strlen(CurrentSel),255);
			
			//write out the name length
			*(reinterpret_cast<byte *>(CurrentPos)) = NameLength;
			CurrentPos += sizeof(byte);
			
			memcpy(CurrentPos,CurrentSel,NameLength);
			CurrentPos += NameLength;
		}
		else
		{
			//write out a zero name length
			*(reinterpret_cast<byte *>(CurrentPos)) = 0;
			CurrentPos += sizeof(byte);
		}
		
		if(CurrentVal != NULL)
		{
			//write out the value
			*(reinterpret_cast<int32 *>(CurrentPos)) = *CurrentVal;
			CurrentPos += sizeof(int32);
		}
		else
		{
			//write out a zero value
			*(reinterpret_cast<int32 *>(CurrentPos)) = 0;
			CurrentPos += sizeof(int32);
		}
	}
	

	return B_OK;
}


status_t
BDiscreteParameter::Unflatten(type_code c,
							  const void *buf,
							  ssize_t size)
{
	/*
	 * NOTICE: This method tries to avoid corrupting an existing parameter
	 *		   by only reading values out of the buffer after it has verified
	 *		   that the size of the buffer is sufficient to read all needed
	 *		   fields.  In this way, we avoid having to exit this method after
	 *         the 'this' object has been modified by reading a part of the buffer.
	 */

	if(!this->AllowsTypeCode(c))
		return B_BAD_TYPE;
	
	if(buf == NULL)
		return B_NO_INIT;
	
	//if the buffer is smaller than the size needed to read the
	//signature and struct size fields, then there is a problem
	if(size < static_cast<ssize_t>(sizeof(int32) + sizeof(ssize_t)))
	{
		return B_ERROR;
	}

	const byte *CurrentPos = static_cast<const byte *>(buf);
	
	
	//QUESTION: I have no idea where this magic number came from, and i'm not sure that it's
	//being read in the correct byte order.
	if( *(reinterpret_cast<const int32 *>(CurrentPos)) != 0x02040607)
	{
		return B_BAD_TYPE;
	}
	CurrentPos += sizeof(int32);
	
	//read the struct size
	ssize_t ParamStructSize = *(reinterpret_cast<const ssize_t *>(CurrentPos));
	CurrentPos += sizeof(ssize_t);
	
	//this is the minimum REQUIRED additional space for a BDiscreteParameter in a flattened buffer
	const ssize_t AdditionalBDiscParamFlatSize = 1;
	
	if((ParamStructSize + AdditionalBDiscParamFlatSize) > size)
	{
		//if the struct size is larger than the size of the buffer we were given,
		//there's a problem
		return B_ERROR;
	}
	
	//read the base BParameter
	status_t RetVal = BParameter::Unflatten(c,buf,size);
	
	if(RetVal != B_OK)
	{
		return RetVal;
	}
	
	CurrentPos = static_cast<const byte *>(buf);
	CurrentPos += ParamStructSize;
	
	//read NumItems
	ssize_t NumItems = *(reinterpret_cast<const ssize_t *>(CurrentPos));
	CurrentPos += sizeof(ssize_t);
	
	ssize_t MaxByteLength = size - (CurrentPos - static_cast<byte *>(buf));
	
	const ssize_t MinFlattenedItemSize(5);
	
	//each item occupies a minimum of 5 bytes, so make sure that there is enough
	//space remaining in the buffer for the specified NumItems (assuming each is minimum size)
	NumItems = min_c(NumItems,MaxByteLength/MinFlattenedItemSize);
	
	//clear any existing name/value pairs
	this->MakeEmpty();
	
	int i;
	for(i = 0; i < NumItems; i++)
	{
		//read the string length for the name associated with this item
		byte NameStringLength = *(reinterpret_cast<const byte *>(CurrentPos));
		CurrentPos += sizeof(byte);
		
		//ensure that we don't read so much that we don't have enough buffer left for the minimum remainder
		//of this item (4 byte int32 value), or for the minimum size of the remaining items (5bytes*(num remaining items))
		MaxByteLength = size - ((CurrentPos - static_cast<byte *>(buf)) + (NumItems - (i+1))*MinFlattenedItemSize + sizeof(int32));
		
		NameStringLength = min_c(NameStringLength,MaxByteLength);
		
		//read the name string
		char *ItemName = new char[NameStringLength + 1];
		memcpy(ItemName,CurrentPos,NameStringLength);
		ItemName[NameStringLength] = 0;
		CurrentPos += NameStringLength;
		
		
		//read the value of this item
		int32 ItemValue = *(reinterpret_cast<const int32 *>(CurrentPos));
		CurrentPos += sizeof(int32);
		
		//add the item and name to the list
		this->AddItem(ItemValue,ItemName);
	}
	

	return B_OK;
}

/*************************************************************
 * private BDiscreteParameter
 *************************************************************/

status_t BDiscreteParameter::_Reserved_DiscreteParameter_0(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_1(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_2(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_3(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_4(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_5(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_6(void *) { return B_ERROR; }
status_t BDiscreteParameter::_Reserved_DiscreteParameter_7(void *) { return B_ERROR; }


BDiscreteParameter::BDiscreteParameter(int32 id,
									   media_type m_type,
									   BParameterWeb *web,
									   const char *name,
									   const char *kind)
: BParameter(id,m_type,B_DISCRETE_PARAMETER,web,name,kind,"")
{
	this->mSelections = new BList();
	this->mValues = new BList();
}


BDiscreteParameter::~BDiscreteParameter()
{
	this->MakeEmpty();
	
	if(this->mSelections != NULL)
	{
		delete this->mSelections;
	}
	if(this->mValues != NULL)
	{
		delete this->mValues;
	}
}

/*************************************************************
 * public BNullParameter
 *************************************************************/

type_code
BNullParameter::ValueType()
{
	//NULL parameters have no value type
	return 0;
}


ssize_t
BNullParameter::FlattenedSize() const
{
	return BParameter::FlattenedSize();
}


status_t
BNullParameter::Flatten(void *buffer,
						ssize_t size) const
{
	return BParameter::Flatten(buffer,size);
}


status_t
BNullParameter::Unflatten(type_code c,
						  const void *buf,
						  ssize_t size)
{
	return BParameter::Unflatten(c,buf,size);
}

/*************************************************************
 * private BNullParameter
 *************************************************************/

status_t BNullParameter::_Reserved_NullParameter_0(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_1(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_2(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_3(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_4(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_5(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_6(void *) { return B_ERROR; }
status_t BNullParameter::_Reserved_NullParameter_7(void *) { return B_ERROR; }


BNullParameter::BNullParameter(int32 id,
							   media_type m_type,
							   BParameterWeb *web,
							   const char *name,
							   const char *kind)
	: BParameter(id,m_type,B_NULL_PARAMETER,web,name,kind,"")
{
}


BNullParameter::~BNullParameter()
{
}


