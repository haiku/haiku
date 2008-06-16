/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2005 Steve Lhomme.  All rights reserved.
**
** This library is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
** 
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
** 
** You should have received a copy of the GNU Lesser General Public
** License along with this library; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
**
** See http://www.matroska.org/license/lgpl/ for LGPL licensing information.**
** Contact license@matroska.org if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

/*!
	\file
	\todo add a PureBlock class to group functionalities between Block and BlockVirtual
	\version \$Id: KaxBlock.h,v 1.24 2004/04/14 23:26:17 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Julien Coloos    <suiryc @ users.sf.net>
*/
#ifndef LIBMATROSKA_BLOCK_H
#define LIBMATROSKA_BLOCK_H

#include <vector>

#include "matroska/KaxTypes.h"
#include "ebml/EbmlBinary.h"
#include "ebml/EbmlMaster.h"
#include "matroska/KaxTracks.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

class KaxCluster;
class KaxReferenceBlock;
class KaxInternalBlock;
class KaxBlockBlob;

class MATROSKA_DLL_API DataBuffer {
	protected:
		binary * myBuffer;
		uint32   mySize;
		bool     bValidValue;
		bool     (*myFreeBuffer)(const DataBuffer & aBuffer); // method to free the internal buffer

	public:
		DataBuffer(binary * aBuffer, uint32 aSize, bool (*aFreeBuffer)(const DataBuffer & aBuffer) = NULL)
			:myBuffer(aBuffer)
			,mySize(aSize)
			,bValidValue(true)	
      ,myFreeBuffer(aFreeBuffer)
		{}
		virtual ~DataBuffer() {}
		virtual binary * Buffer() {return myBuffer;}
		virtual uint32   & Size() {return mySize;};
		virtual const binary * Buffer() const {return myBuffer;}
		virtual const uint32   Size()   const {return mySize;};
		bool    FreeBuffer(const DataBuffer & aBuffer) {
			bool bResult = true;
			if (myBuffer != NULL && myFreeBuffer != NULL && bValidValue) {
				bResult = myFreeBuffer(aBuffer);
				myBuffer = NULL;
				bValidValue = false;
			}
			return bResult;
		}

		virtual DataBuffer * Clone();
};

class MATROSKA_DLL_API SimpleDataBuffer : public DataBuffer {
	public:
		SimpleDataBuffer(binary * aBuffer, uint32 aSize, uint32 aOffset, bool (*aFreeBuffer)(const DataBuffer & aBuffer) = myFreeBuffer)
			:DataBuffer(aBuffer + aOffset, aSize, aFreeBuffer)
			,Offset(aOffset)
			,BaseBuffer(aBuffer)
		{}
		virtual ~SimpleDataBuffer() {}

		DataBuffer * Clone() {return new SimpleDataBuffer(*this);}

	protected:
		uint32 Offset;
		binary * BaseBuffer;

		static bool myFreeBuffer(const DataBuffer & aBuffer)
		{
			binary *_Buffer = static_cast<const SimpleDataBuffer*>(&aBuffer)->BaseBuffer;
			if (_Buffer != NULL)
				free(_Buffer);
			return true;
		}

		SimpleDataBuffer(const SimpleDataBuffer & ToClone);
};

/*!
	\note the data is copied locally, it can be freed right away
* /
class MATROSKA_DLL_API NotSoSimpleDataBuffer : public SimpleDataBuffer {
	public:
		NotSoSimpleDataBuffer(binary * aBuffer, uint32 aSize, uint32 aOffset)
			:SimpleDataBuffer(new binary[aSize - aOffset], aSize, 0)
		{
			memcpy(BaseBuffer, aBuffer + aOffset, aSize - aOffset);
		}
};
*/

class MATROSKA_DLL_API KaxBlockGroup : public EbmlMaster {
	public:
		KaxBlockGroup();
		KaxBlockGroup(const KaxBlockGroup & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxBlockGroup);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxBlockGroup(*this);}

		~KaxBlockGroup();

		/*!
			\brief Addition of a frame without references
		*/
		bool AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, LacingType lacing = LACING_AUTO);
		/*!
			\brief Addition of a frame with a backward reference (P frame)
		*/
		bool AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, const KaxBlockGroup & PastBlock, LacingType lacing = LACING_AUTO);

		/*!
			\brief Addition of a frame with a backward+forward reference (B frame)
		*/
		bool AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, const KaxBlockGroup & PastBlock, const KaxBlockGroup & ForwBlock, LacingType lacing = LACING_AUTO);
		bool AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, const KaxBlockBlob * PastBlock, const KaxBlockBlob * ForwBlock, LacingType lacing = LACING_AUTO);

		void SetParent(KaxCluster & aParentCluster);

		void SetParentTrack(const KaxTrackEntry & aParentTrack) {
			ParentTrack = &aParentTrack;
		}

		/*!
			\brief Set the duration of the contained frame(s) (for the total number of frames)
		*/
		void SetBlockDuration(uint64 TimeLength);
		bool GetBlockDuration(uint64 &TheTimecode) const;

		/*!
			\return the global timecode of this Block (not just the delta to the Cluster)
		*/
		uint64 GlobalTimecode() const;
		uint64 GlobalTimecodeScale() const {
			assert(ParentTrack != NULL);
			return ParentTrack->GlobalTimecodeScale();
		}

		uint16 TrackNumber() const;

		uint64 ClusterPosition() const;
		
		/*!
			\return the number of references to other frames
		*/
		unsigned int ReferenceCount() const;
		const KaxReferenceBlock & Reference(unsigned int Index) const;

		/*!
			\brief release all the frames of all Blocks
		*/
		void ReleaseFrames();

		operator KaxInternalBlock &();

		const KaxCluster *GetParentCluster() const { return ParentCluster; }

	protected:
		KaxCluster * ParentCluster;
		const KaxTrackEntry * ParentTrack;
};

class KaxInternalBlock : public EbmlBinary {
	public:
		KaxInternalBlock( bool bSimple ) :bLocalTimecodeUsed(false), mLacing(LACING_AUTO), mInvisible(false)
			,ParentCluster(NULL), bIsSimple(bSimple), bIsKeyframe(true), bIsDiscardable(false)
		{}
		KaxInternalBlock(const KaxInternalBlock & ElementToClone);
		~KaxInternalBlock();
		bool ValidateSize() const;

		uint16 TrackNum() const {return TrackNumber;}
		/*!
			\todo !!!! This method needs to be changes !
		*/
		uint64 GlobalTimecode() const {return Timecode;}

		/*!
			\note override this function to generate the Data/Size on the fly, unlike the usual binary elements
		*/
		uint64 UpdateSize(bool bSaveDefault = false, bool bForceRender = false);
		uint64 ReadData(IOCallback & input, ScopeMode ReadFully = SCOPE_ALL_DATA);
		
		/*!
			\brief Only read the head of the Block (not internal data)
			\note convenient when you are parsing the file quickly
		*/
		uint64 ReadInternalHead(IOCallback & input);
		
		unsigned int NumberFrames() const { return SizeList.size();}
		DataBuffer & GetBuffer(unsigned int iIndex) {return *myBuffers[iIndex];}

		bool AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, LacingType lacing = LACING_AUTO, bool invisible = false);

		/*!
			\brief release all the frames of all Blocks
		*/
		void ReleaseFrames();

		void SetParent(KaxCluster & aParentCluster);

		/*!
			\return Returns the lacing type that produces the smallest footprint.
		*/
		LacingType GetBestLacingType() const;

		/*!
			\param FrameNumber 0 for the first frame
			\return the position in the stream for a given frame
			\note return -1 if the position doesn't exist
		*/
		int64 GetDataPosition(size_t FrameNumber = 0);

		/*!
			\param FrameNumber 0 for the first frame
			\return the size of a given frame
			\note return -1 if the position doesn't exist
		*/
		int64 GetFrameSize(size_t FrameNumber = 0);
		
		bool IsInvisible() const { return mInvisible; }

		uint64 ClusterPosition() const;

	protected:
		std::vector<DataBuffer *> myBuffers;
		std::vector<int32>        SizeList;
		uint64     Timecode; // temporary timecode of the first frame, non scaled
		int16      LocalTimecode;
		bool       bLocalTimecodeUsed;
		uint16     TrackNumber;
		LacingType mLacing;
		bool       mInvisible;
		uint64     FirstFrameLocation;

		uint32 RenderData(IOCallback & output, bool bForceRender, bool bSaveDefault = false);

		KaxCluster * ParentCluster;
		bool       bIsSimple;
		bool       bIsKeyframe;
		bool       bIsDiscardable;
};

class MATROSKA_DLL_API KaxBlock : public KaxInternalBlock {
	public:
		KaxBlock() :KaxInternalBlock(false) {}
		static EbmlElement & Create() {return *(new KaxBlock);}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		EbmlElement * Clone() const {return new KaxBlock(*this);}
};

#if MATROSKA_VERSION >= 2
class MATROSKA_DLL_API KaxSimpleBlock : public KaxInternalBlock {
	public:
		KaxSimpleBlock() :KaxInternalBlock(true) {}
		static EbmlElement & Create() {return *(new KaxSimpleBlock);}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		EbmlElement * Clone() const {return new KaxSimpleBlock(*this);}

		void SetKeyframe(bool b_keyframe) { bIsKeyframe = b_keyframe; }
		void SetDiscardable(bool b_discard) { bIsDiscardable = b_discard; }

		bool IsKeyframe() const    { return bIsKeyframe; }
		bool IsDiscardable() const { return bIsDiscardable; }

		operator KaxInternalBlock &() { return *this; }
};
#endif // MATROSKA_VERSION

class MATROSKA_DLL_API KaxBlockBlob {
public:
	KaxBlockBlob(BlockBlobType sblock_mode) :ParentCluster(NULL), SimpleBlockMode(sblock_mode) {
		bUseSimpleBlock = (sblock_mode != BLOCK_BLOB_NO_SIMPLE);
		Block.group = NULL;
	}

	~KaxBlockBlob() {
#if MATROSKA_VERSION >= 2
		if (bUseSimpleBlock)
			delete Block.simpleblock;
		else
#endif // MATROSKA_VERSION
			delete Block.group;
	}

	operator KaxBlockGroup &();
	operator const KaxBlockGroup &() const;
#if MATROSKA_VERSION >= 2
	operator KaxSimpleBlock &();
#endif
	operator KaxInternalBlock &();
	operator const KaxInternalBlock &() const;

	void SetBlockGroup( KaxBlockGroup &BlockRef );

	void SetBlockDuration(uint64 TimeLength);

	void SetParent(KaxCluster & aParentCluster);
	bool AddFrameAuto(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, LacingType lacing = LACING_AUTO, const KaxBlockBlob * PastBlock = NULL, const KaxBlockBlob * ForwBlock = NULL);

	bool IsSimpleBlock() const {return bUseSimpleBlock;}

	bool ReplaceSimpleByGroup();
protected:
	KaxCluster * ParentCluster;
	union {
		KaxBlockGroup *group;
#if MATROSKA_VERSION >= 2
		KaxSimpleBlock *simpleblock;
#endif // MATROSKA_VERSION
	} Block;
	bool bUseSimpleBlock;
	BlockBlobType SimpleBlockMode;
};

class MATROSKA_DLL_API KaxBlockDuration : public EbmlUInteger {
	public:
		KaxBlockDuration() {}
		KaxBlockDuration(const KaxBlockDuration & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxBlockDuration);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxBlockDuration(*this);}
};

#if MATROSKA_VERSION >= 2
class MATROSKA_DLL_API KaxBlockVirtual : public EbmlBinary {
	public:
		KaxBlockVirtual() :ParentCluster(NULL) {Data = DataBlock; Size = countof(DataBlock);}
		KaxBlockVirtual(const KaxBlockVirtual & ElementToClone);
		static EbmlElement & Create() {return *(new KaxBlockVirtual);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		bool ValidateSize() const {return true;}

		/*!
			\note override this function to generate the Data/Size on the fly, unlike the usual binary elements
		*/
		uint64 UpdateSize(bool bSaveDefault = false, bool bForceRender = false);

		void SetParent(const KaxCluster & aParentCluster) {ParentCluster = &aParentCluster;}

		EbmlElement * Clone() const {return new KaxBlockVirtual(*this);}

	protected:
		uint64 Timecode; // temporary timecode of the first frame if there are more than one
		uint16 TrackNumber;
		binary DataBlock[5];

		const KaxCluster * ParentCluster;
};
#endif // MATROSKA_VERSION

class MATROSKA_DLL_API KaxBlockAdditional : public EbmlBinary {
	public:
		KaxBlockAdditional() {}
		KaxBlockAdditional(const KaxBlockAdditional & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxBlockAdditional);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		bool ValidateSize() const {return true;}

		EbmlElement * Clone() const {return new KaxBlockAdditional(*this);}
};

class MATROSKA_DLL_API KaxBlockAdditions : public EbmlMaster {
	public:
		KaxBlockAdditions();
		KaxBlockAdditions(const KaxBlockAdditions & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxBlockAdditions);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxBlockAdditions(*this);}
};

class MATROSKA_DLL_API KaxBlockMore : public EbmlMaster {
	public:
		KaxBlockMore();
		KaxBlockMore(const KaxBlockMore & ElementToClone) :EbmlMaster(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxBlockMore);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxBlockMore(*this);}
};

class MATROSKA_DLL_API KaxBlockAddID : public EbmlUInteger {
	public:
		KaxBlockAddID() :EbmlUInteger(1) {}
		KaxBlockAddID(const KaxBlockAddID & ElementToClone) :EbmlUInteger(ElementToClone) {}
		static EbmlElement & Create() {return *(new KaxBlockAddID);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxBlockAddID(*this);}
};

class MATROSKA_DLL_API KaxCodecState : public EbmlBinary {
	public:
		KaxCodecState() {}
		KaxCodecState(const KaxCodecState & ElementToClone) :EbmlBinary(ElementToClone){}
		static EbmlElement & Create() {return *(new KaxCodecState);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		bool ValidateSize() const {return true;}

		EbmlElement * Clone() const {return new KaxCodecState(*this);}
};

END_LIBMATROSKA_NAMESPACE

#endif // LIBMATROSKA_BLOCK_H
