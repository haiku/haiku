/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2004 Steve Lhomme.  All rights reserved.
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
	\version \$Id: KaxCluster.h,v 1.10 2004/04/14 23:26:17 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
	\author Julien Coloos    <suiryc @ users.sf.net>

*/
#ifndef LIBMATROSKA_CLUSTER_H
#define LIBMATROSKA_CLUSTER_H

#include "matroska/KaxTypes.h"
#include "ebml/EbmlMaster.h"
#include "matroska/KaxTracks.h"
#include "matroska/KaxBlock.h"
#include "matroska/KaxCues.h"
#include "matroska/KaxClusterData.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

class KaxSegment;

class MATROSKA_DLL_API KaxCluster : public EbmlMaster {
	public:
		KaxCluster();
		KaxCluster(const KaxCluster & ElementToClone);
		static EbmlElement & Create() {return *(new KaxCluster);}
		const EbmlCallbacks & Generic() const {return ClassInfos;}
		static const EbmlCallbacks ClassInfos;
		operator const EbmlId &() const {return ClassInfos.GlobalId;}
		EbmlElement * Clone() const {return new KaxCluster(*this);}

		/*!
			\brief Addition of a frame without references

			\param the timecode is expressed in nanoseconds, relative to the beggining of the Segment
		*/
		bool AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, KaxBlockGroup * & MyNewBlock, LacingType lacing = LACING_AUTO);
		/*!
			\brief Addition of a frame with a backward reference (P frame)
			\param the timecode is expressed in nanoseconds, relative to the beggining of the Segment

		*/
		bool AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, KaxBlockGroup * & MyNewBlock, const KaxBlockGroup & PastBlock, LacingType lacing = LACING_AUTO);

		/*!
			\brief Addition of a frame with a backward+forward reference (B frame)
			\param the timecode is expressed in nanoseconds, relative to the beggining of the Segment

		*/
		bool AddFrame(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, KaxBlockGroup * & MyNewBlock, const KaxBlockGroup & PastBlock, const KaxBlockGroup & ForwBlock, LacingType lacing = LACING_AUTO);

		/*!
			\brief Render the data to the stream and retrieve the position of BlockGroups for later cue entries
		*/
		uint32 Render(IOCallback & output, KaxCues & CueToUpdate, bool bSaveDefault = false);

		/*!
			\return the global timecode of this Cluster
		*/
		uint64 GlobalTimecode() const;

		KaxBlockGroup & GetNewBlock();
		
		/*!
			\brief release all the frames of all Blocks
			\note this is a convenience to be able to keep Clusters+Blocks in memory (for future reference) withouht being a memory hog
		*/
		void ReleaseFrames();

		/*!
			\brief return the position offset compared to the beggining of the Segment
		*/
		uint64 GetPosition() const;

		void SetParent(const KaxSegment & aParentSegment) {ParentSegment = &aParentSegment;}

		void SetPreviousTimecode(uint64 aPreviousTimecode, int64 aTimecodeScale) {
			bPreviousTimecodeIsSet = true; 
			PreviousTimecode = aPreviousTimecode;
			SetGlobalTimecodeScale(aTimecodeScale);
		}

		/*!
			\note dirty hack to get the mandatory data back after reading
			\todo there should be a better way to get mandatory data
		*/
		void InitTimecode(uint64 aTimecode, int64 aTimecodeScale) {
			SetGlobalTimecodeScale(aTimecodeScale);
			MinTimecode = MaxTimecode = PreviousTimecode = aTimecode * TimecodeScale;
			bFirstFrameInside = bPreviousTimecodeIsSet = true;
		}

		int16 GetBlockLocalTimecode(uint64 GlobalTimecode) const;

		uint64 GetBlockGlobalTimecode(int16 LocalTimecode);

		void SetGlobalTimecodeScale(uint64 aGlobalTimecodeScale) {
			TimecodeScale = aGlobalTimecodeScale;
			bTimecodeScaleIsSet = true;
		}
		uint64 GlobalTimecodeScale() const {
			assert(bTimecodeScaleIsSet); 
			return TimecodeScale;
		}

		bool SetSilentTrackUsed()
		{
			bSilentTracksUsed = true;
			return FindFirstElt(KaxClusterSilentTracks::ClassInfos, true) != NULL;
		}

		bool AddBlockBlob(KaxBlockBlob * NewBlob);

		const KaxSegment *GetParentSegment() const { return ParentSegment; }

	protected:
		KaxBlockBlob     * currentNewBlob;
		std::vector<KaxBlockBlob*> Blobs;
		KaxBlockGroup    * currentNewBlock;
		const KaxSegment * ParentSegment;

		uint64 MinTimecode, MaxTimecode, PreviousTimecode;
		int64  TimecodeScale;

		bool bFirstFrameInside; // used to speed research
		bool bPreviousTimecodeIsSet;
		bool bTimecodeScaleIsSet;
		bool bSilentTracksUsed;

		/*!
			\note method used internally
		*/
		bool AddFrameInternal(const KaxTrackEntry & track, uint64 timecode, DataBuffer & buffer, KaxBlockGroup * & MyNewBlock, const KaxBlockGroup * PastBlock, const KaxBlockGroup * ForwBlock, LacingType lacing);

};

END_LIBMATROSKA_NAMESPACE

#endif // LIBMATROSKA_CLUSTER_H
