/****************************************************************************
** libmatroska : parse Matroska files, see http://www.matroska.org/
**
** <file/class description>
**
** Copyright (C) 2002-2003 Steve Lhomme.  All rights reserved.
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
    \version \$Id: FileKax.cpp 640 2004-07-09 21:05:36Z mosu $
    \author Steve Lhomme     <robux4 @ users.sf.net>
*/
//#include "StdInclude.h"
#include "matroska/FileKax.h"
//#include "Cluster.h"
//#include "Track.h"
//#include "Block.h"
//#include "Frame.h"
//#include "Version.h"

START_LIBMATROSKA_NAMESPACE

//typedef Track *TrackID;

FileMatroska::FileMatroska(IOCallback & output)
 :myFile(output)
#ifdef OLD
 ,myCurrReadBlock(NULL)
 ,myMinClusterSize(5*1024)      // 5KB is the min size of a cluster
 ,myMaxClusterSize(2*1024*1024) // 2MB is the max size of a cluster
 ,myCurrReadBlockTrack(0)
 ,myCurrWriteCluster(2*1024*1024) // myMaxClusterSize
 ,myCurrReadCluster(NULL)
 ,myReadBlockNumber(0)
#endif // OLD
{
#ifdef OLD
    myStreamInfo.MainHeaderSize = TypeHeader::default_size() +
		ActualHeader::default_size() + 
		ExtendedInfo::default_size() + 
		ContentInfo::default_size();
    myStreamInfo.TrackEntrySize = Track::default_size();
    myStreamInfo.BlockHeadSize = BLOCK_HEADER_SIZE;
    myStreamInfo.ClusterHeadSize = CLUSTER_HEADER_SIZE;
    myStreamInfo.ClusterFootSize = CLUSTER_TRAILER_SIZE;
#endif // OLD
}

FileMatroska::~FileMatroska()
{
//    if (myCurrCluster != NULL)
//	throw 0; // there are some data left to write
//    if (myCurrReadCluster != NULL || myCurrReadBlock != NULL)
//	throw 0; // there are some data left to write
}

#ifdef OLD
void FileMatroska::SetMaxClusterSize(const uint32 value)
{
    myMaxClusterSize = value;
    myCurrWriteCluster.setMaxSize(value);
}

void FileMatroska::Close(const uint32 aTimeLength)
{
    Flush();

    // get the file size
    myFile.setFilePointer(0,seek_end);
    myMainHeader.type_SetSize(myFile.getFilePointer());

    // rewrite the header at the beginning
    myFile.setFilePointer(0,seek_beginning);

    // get the Track-entry size
    uint32 track_entries_size = 0;
    for (size_t i=0; i<myTracks.size(); i++)
    {
	track_entries_size += myTracks[i]->default_size();
    }

    myStreamInfo.TrackEntriesSize = track_entries_size;
    myStreamInfo.TimeLength = aTimeLength;
    myMainHeader.Render(myFile, myStreamInfo);

    for (i=0; i<myTracks.size(); i++)
    {
	delete myTracks[i];
    }
}

/*!
    \warning after rendering the head, some parameters are locked
*/
uint32 FileMatroska::RenderHead(const std::string & aEncoderApp)
{
    try {
	uint32 track_entries_size = 0;
	for (size_t i=0; i<myTracks.size(); i++)
	{
	    track_entries_size += myTracks[i]->default_size();
	}

	std::string aStr = LIB_NAME;
	aStr += " ";
	aStr += VERSION;
	myStreamInfo.EncoderLib = aStr;

	myStreamInfo.EncoderApp = aEncoderApp;

	myStreamInfo.TrackEntryPosition = 0 + myStreamInfo.MainHeaderSize;
	myStreamInfo.TrackEntriesSize = myTracks.size() * myStreamInfo.TrackEntrySize;

	myStreamInfo.CodecEntryPosition = myStreamInfo.MainHeaderSize + myStreamInfo.TrackEntriesSize;
	myStreamInfo.CodecEntrySize = 4;
	for (i=0; i<myTracks.size(); i++)
	{
	    myStreamInfo.CodecEntrySize += myTracks[i]->CodecSize();
	}

	// Main Header
	uint32 result = myMainHeader.Render(myFile, myStreamInfo);

	// Track Entries
	for (i=0; i<myTracks.size(); i++)
	{
	    myTracks[i]->RenderEntry(myFile, i+1);
	}
	myStreamInfo.ClusterPosition = myStreamInfo.CodecEntryPosition + myStreamInfo.CodecEntrySize;

	// Codec Header
	result = CodecHeader::Render(myFile, myTracks);

	return result;
    }
    catch (exception & Ex)
    {
	throw Ex;
    }
}

/*!
    \return 0 if the track was not created, or a valid track number
*/
Track * FileMatroska::CreateTrack(const track_type aType)
{
    myTracks.push_back(new Track(aType));
    return myTracks.back();
}

/*Track *FileMatroska::findTrack(Track * aTrack) const
{
    for (size_t i=0; i<myTracks.size(); i++)
    {
	if (myTracks[i] == aTrack)
	    return myTracks[i];
    }

    return NULL;
}*/

void FileMatroska::track_SetName(Track * aTrack, const std::string & aName)
{
    if (IsMyTrack(aTrack))
    {
        aTrack->SetName(aName);
    }
}

void FileMatroska::track_SetLaced(Track * aTrack, const bool bLaced)
{
    if (IsMyTrack(aTrack))
    {
        aTrack->SetLaced(bLaced);
    }
}

bool FileMatroska::AddFrame(Track * aTrack, const uint32 aTimecode, const binary *aFrame, const uint32 aFrameSize,
					   const bool aKeyFrame, const bool aBFrame)
{
    try {
	// make sure we know that track
	if (IsMyTrack(aTrack))
	{
	    // pass the cluster to the track
	    // handle the creation of a new cluster if needed
	    if (aTrack->AddFrame(aTimecode, aFrame, aFrameSize, aKeyFrame, aBFrame))
	    {
		while (!aTrack->SerialiseBlock(myCurrWriteCluster))
		{
		    /// \todo handle errors
		    uint32 aNbBlock;
		    myStreamInfo.ClusterSize += myCurrWriteCluster.Render(myFile, aNbBlock);
		    myStreamInfo.NumberBlock += aNbBlock;
		    myCurrWriteCluster.Flush();
		}
	    }
		return true;
	}
	return false;
    }
    catch (exception & Ex)
    {
	throw Ex;
    }
}

void FileMatroska::Flush()
{
    uint32 aNbBlock;
    myStreamInfo.ClusterSize += myCurrWriteCluster.Render(myFile,aNbBlock);
    myStreamInfo.NumberBlock += aNbBlock;
}

uint32 FileMatroska::ReadHead()
{
    try {
	uint32 result = myMainHeader.Read(myFile, myStreamInfo);

	return result;
    }
    catch (exception & Ex)
    {
	throw Ex;
    }
}

uint32 FileMatroska::ReadTracks()
{
    try {
	uint32 result = 0;

	// seek to the start of the Track Entries
	myFile.setFilePointer(myStreamInfo.TrackEntryPosition);
	// get the number of Track Entries
	uint8 TrackNumber = myStreamInfo.TrackEntriesSize / myStreamInfo.TrackEntrySize;
	// read all the Track Entries
	myTracks.clear();
	for (uint8 TrackIdx = 0; TrackIdx<TrackNumber; TrackIdx ++) {
	    Track * tmpTrack = Track::ReadEntry(myFile, TrackIdx+1, myStreamInfo);
	    if (tmpTrack == NULL)
		throw 0;
	    
	    myTracks.push_back(tmpTrack);
	}

	return result;
    }
    catch (exception & Ex)
    {
	throw Ex;
    }
}

uint32 FileMatroska::ReadCodec()
{
    try {
	// seek to the start of the Track Entries
	myFile.setFilePointer(myStreamInfo.CodecEntryPosition);

	uint32 result = CodecHeader::Read(myFile, myTracks);

	return result;
    }
    catch (exception & Ex)
    {
	throw Ex;
    }
}

inline bool FileMatroska::IsMyTrack(const Track * aTrack) const
{
    if (aTrack == 0)
	throw 0;

    for (std::vector<Track*>::const_iterator i = myTracks.begin(); i != myTracks.end(); i ++)
    {
	if (*i == aTrack)
	    break;
    }

    if (i != myTracks.end())
	return true;
    else
	return false;
}

void FileMatroska::SelectReadingTrack(Track * aTrack, bool select)
{
    if (IsMyTrack(aTrack))
    {
	// here we have the right track
	// check if it's not already selected
	for (std::vector<uint8>::iterator j = mySelectedTracks.begin();
	    j != mySelectedTracks.end(); j ++)
	{
	    if (*j == aTrack->TrackNumber())
		break;
	}

	if (select && j == mySelectedTracks.end())
	    mySelectedTracks.push_back(aTrack->TrackNumber());
	else if (!select && j != mySelectedTracks.end())
	    mySelectedTracks.erase(j);

	std::sort(mySelectedTracks.begin(), mySelectedTracks.end());
    }
}

inline bool FileMatroska::IsReadingTrack(const uint8 aTrackNumber) const
{
    for (std::vector<uint8>::const_iterator trackIdx = mySelectedTracks.begin();
         trackIdx != mySelectedTracks.end() && *trackIdx < aTrackNumber;
	 trackIdx++)
    {}

    if (trackIdx == mySelectedTracks.end())
	return false;
    else
	return true;
}

//

void FileMatroska::Track_GetInfo(const Track * aTrack, TrackInfo & aTrackInfo) const
{
    if (IsMyTrack(aTrack))
    {
	aTrack->GetInfo(aTrackInfo);
    }
}

// Audio related getters/setters

void FileMatroska::Track_GetInfo_Audio(const Track * aTrack, TrackInfoAudio & aTrackInfo) const
{
    if (IsMyTrack(aTrack))
    {
	aTrack->GetInfoAudio(aTrackInfo);
    }
}

void FileMatroska::Track_SetInfo_Audio(Track * aTrack, const TrackInfoAudio & aTrackInfo)
{
    if (IsMyTrack(aTrack))
    {
	aTrack->SetInfoAudio(aTrackInfo);
    }
}

// Video related getters/setters

void FileMatroska::Track_GetInfo_Video(const Track * aTrack, TrackInfoVideo & aTrackInfo) const
{
    if (IsMyTrack(aTrack))
    {
	aTrack->GetInfoVideo(aTrackInfo);
    }
}

void FileMatroska::Track_SetInfo_Video(Track * aTrack, const TrackInfoVideo & aTrackInfo)
{
    if (IsMyTrack(aTrack))
    {
	aTrack->SetInfoVideo(aTrackInfo);
    }
}

/*!
    \todo exit when there is no Block left
*/
bool FileMatroska::ReadFrame(Track * & aTrack, uint32 & aTimecode, const binary * & aFrame, uint32 & aFrameSize,
						bool & aKeyFrame, bool & aBFrame)
{
    if (myCurrReadBlockTrack == 0)
    {
	do {
	    if (myReadBlockNumber >= myStreamInfo.NumberBlock)
	    {
//		myReadBlockNumber = myStreamInfo.NumberBlock;
		return false;
	    }

	    // get the next frame in the file
	    if (!myCurrReadCluster.BlockLeft())
	    {
		myCurrReadCluster.Flush();
		try {
		    myCurrReadCluster.FindHead(myFile);
		}
		catch (exception & Ex)
		{
		    return false;
		}
	    }

	    myCurrReadCluster.GetBlock( myCurrReadBlock, myCurrReadBlockSize, myCurrReadBlockTrack );
	    myReadBlockNumber++;
	} while (!IsReadingTrack(myCurrReadBlockTrack));

	// get the track associated (normally from myTracks)
	aTrack = myTracks[myCurrReadBlockTrack-1];
	// get the next frame from the current block
	aTrack->HandleBlock(myCurrReadBlock, myCurrReadBlockSize);
    }
    else
    {
	// get the track associated (normally from myTracks)
	aTrack = myTracks[myCurrReadBlockTrack-1];
    }
    
    Frame *  myReadFrame;
    aTrack->GetNextFrame(aTimecode, myReadFrame, aKeyFrame, aBFrame);
    aFrame = myReadFrame->buf();
    aFrameSize = myReadFrame->length();

    if (aTrack->NoFrameLeft())
    {
	aTrack->FlushBlock();
	myCurrReadBlockTrack = 0;
    }

    return true;
}
#endif // OLD

END_LIBMATROSKA_NAMESPACE
