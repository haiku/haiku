extern "C" {
  // Your addon should implement this function.
  // Return a new instance of your BMediaExtractorAddOn subclass.
  // This function will be called multiple times, and should return a
  // new instance each time.  Return NULL if allocation fails.
  BMediaExtractorAddOn * instantiate_media_extractor_add_on();
}

// Your add-on must implement a subclass of this class
class BMediaExtractorAddOn
{
public:
  BMediaExtractorAddOn(void);
  virtual ~BMediaExtractorAddOn(void);

  //// stateless functions
  // these should work without dependency on a current stream

/* begin BFileInterface functions */
  // These are used to enumerate the set of file formats that this
  // extractor is prepared to read from.  Implementing these meaningfully
  // is important for discovering all types supported by the system.

  // Implement per BFileInterface::GetNextFileFormat
  //
  // Return codes:
  // B_OK : No error
  // B_ERROR : No more formats
  // GetNextInputFormat: required for BFileInterface functionality
  virtual status_t GetNextInputFormat(int32 * cookie,
                                      media_file_format * outFormat) = 0;
  // Implement per BFileInterface::DisposeFileFormatCookie
  // DisposeInputFormatCookie: required for BFileInterface functionality
  virtual void DisposeInputFormatCookie(int32 cookie) = 0;

/* begin transcoding functions */
  // These are used to enumerate the set of file formats that this
  // extractor is prepared to transcode to.  The default implementation
  // simply returns no support.

  // Implement per BFileInterface::GetNextFileFormat
  //
  // Return codes:
  // B_OK : No error
  // B_ERROR : No more formats
  virtual status_t GetNextOutputFormat(int32 * cookie,
                                       media_file_format * outFormat);
  // Implement per BFileInterface::DisposeFileFormatCookie
  virtual void DisposeOutputFormatCookie(int32 cookie);
/* end transcoding functions */
/* end BFileInterface functions */
  
/* begin BMediaAddOn functions */
  // These are used to discover an extractors quality rating for a
  // particular media format.
  // Implement per BMediaAddOn::SniffType
  //
  // Return codes:
  // B_OK : No error
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle that mime type
  virtual status_t SniffInputType(BMimeType & mimeType, float * outQuality) = 0;
/* begin transcoding function */
  virtual status_t SniffOutputType(BMimeType & mimeType, float * outQuality);
/* end transcoding function */
/* end BMediaAddOn functions */

  // Same as above, but for a media file format
  // The default implementation of this will iterate through your formats using
  // the appropriate interface from above, and simply return 0 for the quality
  // if it finds a matching supported format.
  //
  // Return codes:
  // B_OK : No error
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle that format
  virtual status_t SniffInputFormat(const media_file_format & format, float * outQuality);
/* begin transcoding function */
  virtual status_t SniffOutputFormat(const media_file_format & format, float * outQuality);
/* end transcoding function */

  //// state creation functions
  // calling these functions shouldn't affect the results of the stateless functions

  // Sets the current stream to source or destination
  // The default implementation for the BDataIO SetSource is to wrap
  // the BDataIO object in a buffer and call the BPositionIO SetSource.
  // The default implementation for the BFile SetSource is to send the
  // call directly to BPositionIO.  Note that it is highly recommended
  // to utilize the BNode properties of the BNodeIO/BFile object in
  // order to dynamically update your extractor state when the file
  // changes.  It is also recommended to use the BNode properties in
  // order to access the attributes of the source file; store or load
  // file specific extractor properties from here.
  // Note: the extractor is not require to return B_MEDIA_NO_HANDLER at
  // this point.  However, calling any stateful function after this
  // should return B_MEDIA_NO_HANDLER.
  //
  // Return codes:
  // B_OK : No error
  // B_NO_MEMORY : Storage for the buffer could not be allocated.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle that format
  virtual status_t SetSource(const BFile * source);
  virtual status_t SetSource(const entry_ref * source, int32 flags = 0);
  virtual status_t SetSource(const BDataIO * source);
/* begin transcoding functions */
  virtual status_t SetDestination(const BFile * source);
  virtual status_t SetDestination(const entry_ref * source, int32 flags = 0);
  virtual status_t SetDestination(const BDataIO * source);
/* end transcoding functions */

  //// stateful functions
  // Calling these functions shouldn't affect the results of the stateless functions.
  // Calling these functions before calling a state creation function should return
  // B_NO_INIT.  Calling these functions after calling a state creation function with
  // an invalid argument should return B_MEDIA_NO_HANDLER.  Generally these
  // functions may also return any appropriate Storage Kit/File System Errors, such
  // as B_FILE_NOT_FOUND, B_BUSTED_PIPE, etc.

  // inspired by BMediaFile::GetFileFormatInfo
  //
  // Fills the specified media_file_format structure with
  // information describing the file format of the stream
  // currently referenced by the BEncoder.
  // 
  // Return codes:
  // B_OK        : No error
  // B_NO_INIT   : The BEncoder doesn't reference a valid stream.
  // B_NO_MEMORY : Storage for part of the media_file_format
  //               object couldn't be allocated.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  virtual status_t GetFileFormatInfo(media_file_format * mfi) = 0;
                             
  // The extractor should implement this function in the
  // manner described for BFileInterface::SniffRef, except that
  // it uses the current Source instead of an entry_ref
  //
  // Return codes:
  // B_OK : No error
  // B_NO_INIT   : The BEncoder doesn't reference a valid stream.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  virtual status_t Sniff(char * outMimeType, float * outQuality) = 0;

  // implement per BMediaTrack::AddChunk(void)
  // 
  // Return codes:
  // B_OK        : No error
  // B_NO_INIT   : The BEncoder doesn't reference a valid stream.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  virtual status_t WriteChunk(int32 type,
                              const void * data,
                              size_t size);

/* begin weird function that is missing but parallels add chunk */
  // implement per BMediaTrack::ReadChunk(void) <- missing????
  // umm.. has the same semantics as AddChunk, yeah that's it...
  // 
  // Return codes:
  // B_OK        : No error
  // B_NO_INIT   : The BEncoder doesn't reference a valid stream.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  virtual status_t ReadChunk(int32 * outType,
                             const void * outData,
                             size_t * outSize);
/* end weird function that is missing but parallels add chunk */

  // The extractor should do any cleanup required.  After
  // this function returns, the source object should be
  // closed and deleted by the caller, not by Close().
  // The default implementation simply returns B_OK.
  //
  // Return codes:
  // B_OK : No error
  // B_NO_INIT   : The BEncoder doesn't reference a valid stream.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  virtual status_t Close(void);

  //// shared state functions
  // The ParameterWeb interface is used to publish both file specific
  // and extractor specific options.  Accessing a file specific parameter
  // before calling a state creation function should return B_NO_INIT.
  // Accessing a file parameter after calling a state creation function
  // with an invalid argument should return B_MEDIA_NO_HANDLER.  Accessing
  // extractor specific options should never return these errors, but may
  // return other errors.

  // the extractor should provide several basic parameters
  // through this interface, such as B_TRACK_COUNT, and B_DURATION
  // see also BMediaFile::GetParameterValue
  // hmmm... how to pick which stream parameters apply to?
  // could use a bitwise or with B_OUTPUT_STREAM (and a
  // B_INPUT_STREAM for completeness)
  //
  // Return codes:
  // B_OK : No error
  // B_NO_INIT   : The BEncoder doesn't reference a valid stream.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  virtual status_t GetParameterValue(int32 id, const void * value, 
                                     size_t * size) = 0;

  // the extractor may optionally supply parameters for the
  // user to configure, such as buffering information(?)
  // see also BMediaFile::SetParameterValue
  //
  // Return codes:
  // B_OK : No error
  // B_NO_INIT   : The BEncoder doesn't reference a valid stream.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  virtual status_t SetParameterValue(int32 id, const void * value,
                                     size_t size);

  // The extractor may return a BParameterWeb for browsing or
  // configuring the extractor's parameters.  Returns NULL if the
  // extractor doesn't support this.  The default implementation
  // simply returns NULL.  Note: if the Source is not in a good
  // state, this web may not include file specific parameters.
  //
  // As a suggestion you should use groups to gather parameters
  // related to the encoder and separate them from parameters
  // related to the input stream and output stream (if applicable)
  //  
  // See also BMediaFile::Web
  virtual BParameterWeb * Web(void) { return NULL; }

  // The extractor may return a BView for browsing or configuring
  // the extractor's parameters.  Returns NULL if the extractor
  // doesn't support this.  The default implementation simply
  // returns NULL.
  virtual BView * GetParameterView(void) (void) { return NULL; }

/* begin seek/sync functions for the extractor */
  // The extractor will seek first on the seek track, just like
  // BMediaTrack::SeekToTime.  Like SeekToTime, it accepts a flag
  // argument which tells how to find the nearest acceptable frame.
  // After finding this frame, it will also seek any other open
  // streams in an extractor-dependent fashion.  Usually the seek
  // stream will be a video stream.  If seeked to a keyframe, for
  // example, the audio stream will be seeked to an appropriate time.
  //
  // This may be more efficient than seeking the seek track through
  // the BMediaTrack interface, and then calling Sync() here.  It
  // should not be less efficient.
  //
  // See also BMediaTrack::SeekToTime
  // see above for additions to media_seek_type (used for flags)
  // seekMode per BFile::Seek, only SEEK_SET is required
  //
  // Return codes:
  // B_OK : No error
  // B_UNSUPPORTED : This extractor does not support general seeking
  //                 for this stream.
  // B_NO_INIT   : The BEncoder doesn't reference a valid stream.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  virtual status_t SeekToTime(bigtime_t * ioTime,
                              int32 mediaSeekFlags = 0,
                              int32 seekMode = 0) = 0;
  
  // The extractor will seek first on the seek track, just like
  // BMediaTrack::SeekToFrame.  Like SeekToFrame, it accepts a flag
  // argument which tells how to find the nearest acceptable frame.
  // After finding this frame, it will also seek any other open
  // streams in an extractor-dependent fashion.  Usually the seek
  // stream will be a video stream.  If seeked to a keyframe, for
  // example, the audio stream will be seeked to an appropriate time.
  //
  // This may be more efficient than seeking the seek track through
  // the BMediaTrack interface, and then calling Sync() here.  It
  // should not be less efficient.
  //
  // See also BMediaTrack::SeekToFrame
  // see above for additions to media_seek_type (used for flags)
  // seekMode per BFile::Seek, only SEEK_SET is required
  //
  // Return codes:
  // B_OK : No error
  // B_UNSUPPORTED : This extractor does not support general seeking
  //                 for this stream.
  // B_NO_INIT   : The BEncoder doesn't reference a valid stream.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  virtual status_t SeekToFrame(int64 * ioFrame,
                               int32 mediaSeekFlags = 0,
                               int32 seekMode = 0) = 0;

/* begin seek extensions functions */
  // The extractor will seek first on the seek track.  It goes to
  // a position defined by ioChunk*chunkSize, where chunkSize is
  // defined by the decoder.  For example, some streams are not byte
  // streams, but rather bitstreams.  In this case the chunkSize may
  // correspond to 1 bit.  Like the other MediaTrack Seeks, it
  // accepts a flag argument which tells how to find the nearest
  // acceptable frame.
  // After finding this frame, it will also seek any other open
  // streams in an extractor-dependent fashion.  Usually the seek
  // stream will be a video stream.  If seeked to a keyframe, for
  // example, the audio stream will be seeked to an appropriate time.
  //
  // This may be more efficient than seeking the seek track through
  // the BMediaTrack interface, and then calling Sync() here.  It
  // should not be less efficient.
  //
  // see above for additions to media_seek_type (used for flags)
  // seekMode per BFile::Seek, only SEEK_SET is required
  //
  // Return codes:
  // B_OK : No error
  // B_UNSUPPORTED : This extractor does not support general seeking
  //                 for this stream.
  // B_NO_INIT   : The BEncoder doesn't reference a valid stream.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  virtual status_t SeekToChunk(int64 * ioChunk,
                               int32 mediaSeekFlags = 0,
                               int32 seekMode = 0) = 0;
  
  // The extractor will seek first on the seek track.  It goes to a
  // position defined by numerator/(duration of this file).  For
  // example: Seek(LONG_LONG_MAX/2) would seek halfway through the
  // stream.  Like the other MediaTrack Seeks, it accepts a flag
  // argument which tells how to find the nearest acceptable frame.
  // If the seekMode is SEEK_SET it will seek a fraction of the way
  // back to the beginning from the current location.  If the seekMode
  // is SEEK_END it will seek a fraction of the way to the end from
  // the current location.  If the seekMode is SEEK_CUR it will seek
  // as above. (fraction of the entire file duration)
  // After finding this frame, it will also seek any other open
  // streams in an extractor-dependent fashion.  Usually the seek
  // stream will be a video stream.  If seeked to a keyframe, for
  // example, the audio stream will be seeked to an appropriate time.
  //
  // This may be a lot more efficient than seeking to a time or frame
  // for some streams. (in particular, nonindexed streams)
  //
  // This may be more efficient than seeking the seek track through
  // the BMediaTrack interface, and then calling Sync() here.  It
  // should not be less efficient.
  //
  // Note: because the duration may change over time (if the file is
  // being written to, for example) the result of seeking with a
  // particular numerator may also change.  It will usually be later,
  // but could also be earlier.
  //
  // see above for additions to media_seek_type (used for flags)
  // seekMode per BFile::Seek, only SEEK_CUR is required
  //
  // Return codes:
  // B_OK : No error
  // B_UNSUPPORTED : This extractor does not support general seeking
  //                 for this stream.
  // B_NO_INIT     : The BEncoder doesn't reference a valid stream.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  virtual status_t Seek(int64 * numerator,
                        int32 mediaSeekFlags = 0,
                        int32 seekMode = 0) = 0;
/* end seek extensions functions */

  // Using the location from the seek stream, seeks any other open
  // streams in an extractor-dependent fashion.  Usually the seek
  // stream will be a video stream.  If seeked to a keyframe, for
  // example, the audio stream will be seeked to an appropriate time.
  //
  // Note: if not supplied, the seek stream will be the current one
  // as retrieved by GetParameterValue, not zero.  Sync() will do
  // this check for you.
  //
  // Return codes:
  // B_OK : No error
  // B_UNSUPPORTED : This extractor does not support general syncing
  //                 for this stream.
  // B_NO_INIT   : The BEncoder doesn't reference a valid stream.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  virtual status_t Sync(int32 seekStream = 0) = 0;
/* end seek/sync functions for the extractor */
  
  // Returns a thing that is useful for MediaTrack to do its business.
  //
  // May simply include state but will probably include a pointer back
  // to this object, and will likely call functions that are defined by
  // subclasses of this extractor.  For example, the subclass may define
  // a function like this:
  // SeekTrackToFrame(BTrack * track, int64 ioFrame, int32 flags = 0) {
  // ... }
  // and then when SeekToFrame is called on the BTrack object the work
  // would be done by the Extractor.
  //
  // Also, any track extracted using this function will be seeked by
  // the extractor seek functions.  Any track not extracted by this
  // function will not be seeked.  If the seekMode parameter is
  // supplied as SEEK_CUR the track will be seeked before being
  // returned, as per Sync().  However because this involves only
  // one track it may be more efficient than retrieving the track and
  // then calling Sync();  If seekMode is SEEK_SET then the current
  // seek time for the track will be no later than the earliest
  // seekable time.  If seekMode is SEEK_END the current seek time
  // for the track will be no earlier than the earliest seekable time.
  // Note: for non-seekable tracks, this may may no difference.
  // The default for seekMode is SEEK_SET.
  //
  // If the seek parameter is passed as false, no pre-seeking will be
  // performed on the track.  The current seek time may be arbitrary
  // or even illegal.  Attempting to decode data from the track in
  // this state will result in an error if the state is illegal.
  //
  // Return codes:
  // B_OK : No error
  // B_STREAM_NOT_FOUND
  // B_BAD_INDEX : The index supplied does not correspond to a valid
  //               track in this stream.
  // B_NO_INIT   : The BEncoder doesn't reference a valid stream.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  virtual BTrack * TrackAt(int32 index, int32 seekMode = 0,
                                        bool seek = true) = 0;
  
  // Disclaims interest in a particular track.  After releasing a
  // track the track will no longer be seeked by the extractor.
  //
  // Return codes:
  // B_OK : No error
  // B_BAD_TYPE  : This track does not correspond to this extractor.
  // B_NO_INIT   : The BEncoder doesn't reference a valid stream.
  // B_MEDIA_NO_HANDLER : This extractor doesn't handle this format
  status_t ReleaseTrack(BTrack * track);

protected:
  // use to negotiate the format for this track
  // straight BMediaTrack::DecodedFormat behavior
  virtual status_t NegotiateOutputFormat(BTrack * track,
                                         media_format * ioFormat) = 0;

  // get/set information about a particular track
  virtual status_t GetParameterValue(BTrack * track, int32 id,
                                     const void * value, size_t * size) = 0;
  virtual status_t SetParameterValue(BTrack * track, int32 id,
                                     const void * value, size_t size);
  virtual BParameterWeb * Web(BTrack * track) { return NULL; }
  virtual BView * GetParameterView(BTrack * track) { return NULL; }

  // seek only this particular track to the given time
  // straight BMediaTrack::SeekToTime behavior
  virtual status_t SeekToTime(BTrack * track,
                              bigtime_t * ioTime,
                              int32 mediaSeekFlags = 0,
                              int32 seekMode = 0) = 0;
  // seek only this particular track to the given frame
  // straight BMediaTrack::SeekToFrame behavior
  virtual status_t SeekToFrame(BTrack * track,
                               int64 * ioFrame,
                               int32 mediaSeekFlags = 0,
                               int32 seekMode = 0) = 0;
  // seek only this particular track to the given chunk
  // straight BMediaTrack::SeekToChunk behavior
  virtual status_t SeekToChunk(BTrack * track,
                               int64 * ioChunk,
                               int32 mediaSeekFlags = 0,
                               int32 seekMode = 0) = 0;
  // seek only this particular track to the given chunk
  // straight BMediaTrack::Seek behavior
  virtual status_t Seek(BTrack * track,
                        int64 * numerator,
                        int32 mediaSeekFlags = 0,
                        int32 seekMode = 0) = 0;

  // read a chunk from this track only
  // straight BMediaTrack::ReadChunk behavior
  virtual status_t ReadChunk(BTrack * track,
                             char ** outBuffer, int32 * ioSize,
                             media_header * outHeader = NULL) = 0;
  // read frames from this track only
  // straight BMediaTrack::ReadChunk behavior
  virtual status_t ReadFrames(BTrack * track,
                              void * outBuffer, int64 * outFrameCount,
                              media_header * outHeader = NULL,
                              media_decode_info * info = NULL) = 0;
/* begin read extensions functions */
  // read units of time from this track only
  virtual status_t ReadTime(BTrack * track,
                            void * outBuffer, int64 * outTimeCount,
                            media_header * outHeader = NULL,
                            media_decode_info * info = NULL) = 0;
  // for completeness sake?
  // read a percentage from this track only
  virtual status_t Read(BTrack * track,
                        void * outBuffer, int64 * outNumerator,
                        media_header * outHeader = NULL,
                        media_decode_info * info = NULL) = 0;
/* end read extensions functions */

private:
  // this class is used by individual tracks
  // as a private interface to the extractor
  class BTrack {
    // use to negotiate the format for this track
    virtual status_t NegotiateOutputFormat(media_format * ioFormat) {
      return BExtractor::NegotiateOutputFormat(ioFormat);
    }
    
    // access to parameters for this track
    virtual status_t GetParameterValue(int32 id, const void * value, 
                                       size_t * size) {
      return BExtractor::GetParameterValue(this,id,value,size);
    }
    virtual status_t SetParameterValue(int32 id, const void * value,
                                       size_t size) {
      return BExtractor::SetParameterValue(this,id,value,size);
    }
    virtual BParameterWeb * Web(void) {
      return BExtractor::Web(this);
    }
    virtual BView * GetParameterView(void) {
      return BExtractor::GetParameterView(this);
    }
    
    // access to seek functionality on this track
    virtual status_t SeekToTime(bigtime_t * ioTime,
                              int32 mediaSeekFlags = 0,
                              int32 seekMode = 0) {
      return BExtractor::SeekToTime(this,ioTime,mediaSeekFlags,seekMode);
    }
    virtual status_t SeekToFrame(int64 * ioFrame,
                                 int32 mediaSeekFlags = 0,
                                 int32 seekMode = 0) {
      return BExtractor::SeekToFrame(this,ioFrame,mediaSeekFlags,seekMode);
    }
    virtual status_t SeekToChunk(int64 * ioChunk,
                                 int32 mediaSeekFlags = 0,
                                 int32 seekMode = 0) {
      return BExtractor::SeekToChunk(this,ioChunk,mediaSeekFlags,seekMode);
    }
    virtual status_t Seek(int64 * numerator,
                          int32 mediaSeekFlags = 0,
                          int32 seekMode = 0) {
      return BExtractor::Seek(this,numerator,mediaSeekFlags,seekMode);
    }

    // access to readers for this track
    virtual status_t ReadChunk(char ** outBuffer, int32 * ioSize,
                               media_header * outHeader = NULL) {
      return BExtractor::ReadChunk(this,outBuffer,ioSize,outHeader);
    }
    virtual status_t ReadFrames(void * outBuffer, int64 * outFrameCount,
                                media_header * outHeader = NULL,
                                media_decode_info * info = NULL) {
      return BExtractor::ReadFrames(this,outBuffer,outFrameCount,outHeader,info);
    }
/* begin read extensions functions */
	virtual status_t ReadTime(void * outBuffer, int64 * outTimeCount,
                              media_header * outHeader = NULL,
                              media_decode_info * info = NULL) {
      return BExtractor::ReadTime(this,outBuffer,outTimeCount,outHeader,info);
    }
    // for completeness sake?
    virtual status_t Read(void * outBuffer, int64 * outNumerator,
                          media_header * outHeader = NULL,
                          media_decode_info * info = NULL) {
      return BExtractor::Read(this,outBuffer,outNumerator,outHeader,info);
    }
/* end read extensions functions */
    // pad me
  };
  
  // pad me
};


