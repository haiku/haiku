/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2004 Steve Lhomme.  All rights reserved.
**
** This file is part of libmatroska.
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
	\version \$Id: FileKax.h,v 1.5 2004/04/14 23:26:17 robux4 Exp $
	\author Steve Lhomme     <robux4 @ users.sf.net>
*/
#ifndef LIBMATROSKA_FILE_H
#define LIBMATROSKA_FILE_H

//#include <vector>

#include "matroska/KaxTypes.h"
#include "ebml/IOCallback.h"
//#include "MainHeader.h"
//#include "TrackType.h"
//#include "StreamInfo.h"
//#include "Cluster.h"
//#include "CodecHeader.h"

using namespace LIBEBML_NAMESPACE;

START_LIBMATROSKA_NAMESPACE

//class Track;
//class Frame;

/*!
    \class MATROSKA_DLL_API FileMatroska
    \brief General container of all the parameters and data of an Matroska file
    \todo Handle the filename and next filename
    \todo Handle the IOCallback selection/type
*/
class MATROSKA_DLL_API FileMatroska {
    public:
		FileMatroska(IOCallback & output);
		~FileMatroska();
#ifdef OLD
	uint32 RenderHead(const std::string & aEncoderApp);
	uint32 ReadHead();
	uint32 ReadTracks();
	uint32 ReadCodec();
	void Close(const uint32 aTimeLength);

	inline void type_SetInfo(const std::string & aStr) {myMainHeader.type_SetInfo(aStr);}
	inline void type_SetAds(const std::string & aStr) {myMainHeader.type_SetAds(aStr);}
	inline void type_SetSize(const std::string & aStr) {myMainHeader.type_SetSize(aStr);}
	inline void type_SetSize(const uint64 aSize) {myMainHeader.type_SetSize(aSize);}

	inline uint8 GetTrackNumber() const { return myTracks.size(); }

	void track_SetName(Track * aTrack, const std::string & aName);
	void track_SetLaced(Track * aTrack, const bool bLaced = true);

	Track * CreateTrack(const track_type aType);
	inline Track * GetTrack(const uint8 aTrackNb) const
	{
	    if (aTrackNb > myTracks.size())
		return NULL;
	    else
		return myTracks[aTrackNb-1];
	}

	void Track_GetInfo(const Track * aTrack, TrackInfo & aTrackInfo) const;
	
	void Track_SetInfo_Audio(Track * aTrack, const TrackInfoAudio & aTrackInfo);
	void Track_GetInfo_Audio(const Track * aTrack, TrackInfoAudio & aTrackInfo) const;

	void Track_SetInfo_Video(Track * aTrack, const TrackInfoVideo & aTrackInfo);
	void Track_GetInfo_Video(const Track * aTrack, TrackInfoVideo & aTrackInfo) const;

	void SelectReadingTrack(Track * aTrack, bool select = true);

	/*!
	    \return whether the frame has been added or not
	*/
	bool AddFrame(Track * aTrack, const uint32 aTimecode, const binary *aFrame, const uint32 aFrameSize,
		     const bool aKeyFrame = true, const bool aBFrame = false);

	/*!
	    \return whether the frame has been read or not
	*/
	bool ReadFrame(Track * & aTrack, uint32 & aTimecode, const binary * & aFrame, uint32 & aFrameSize,
		     bool & aKeyFrame, bool & aBFrame);

	/*
	    Render the pending cluster to file
	*/
	void Flush();

	void SetMaxClusterSize(const uint32 value);
	void SetMinClusterSize(const uint32 value) {myMinClusterSize = value;}

    protected:
	MainHeader myMainHeader;

	std::vector<Track *> myTracks;
	std::vector<uint8> mySelectedTracks;

//	Track *findTrack(Track * aTrack) const;

	Cluster  myCurrWriteCluster; /// \todo merge with the write one ?
	uint32   myReadBlockNumber;
	Cluster  myCurrReadCluster;
	binary * myCurrReadBlock;      ///< The buffer containing the current read block
	uint32   myCurrReadBlockSize;  ///< The size of the buffer containing the current read block
	uint8    myCurrReadBlockTrack; ///< The track number of the current track to read

	uint32 myMaxClusterSize;
	uint32 myMinClusterSize;

	StreamInfo myStreamInfo;

	CodecHeader myCodecHeader;

	inline bool IsMyTrack(const Track * aTrack) const;
	inline bool IsReadingTrack(const uint8 aTrackNum) const;
#endif // OLD
	IOCallback & myFile;

};

END_LIBMATROSKA_NAMESPACE

#endif // FILE_KAX_HPP
