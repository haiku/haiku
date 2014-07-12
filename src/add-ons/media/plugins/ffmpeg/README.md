This file collects knowledge about the FFMPEG-library used by the FFMPEG plugin.

As it turns out documentation on how to use the FFMPEG-libraries (libavcodec, libavformat, ...) to decode audio and video data is rather thinly spread.

There are three main resources that can be recommended for understanding how to use the FFMPEG-libraries:

1. Example source code provided by the FFMPEG project.
  - It is part of the FFMPEG source code and is located in /path/to/ffmpeg/source/doc/examples
  - Studying the example "decoding_encoding.c" is especially interesting, as it provides examples on how to decode and encode audio / video files with FFMPEG.

2. An ffmpeg and SDL Tutorial: http://dranger.com/ffmpeg/tutorial01.html
  - This tutorial contains several parts, whereby parts 5 to 8 are the most relevant parts, as they provide a lot of advanced knowledge about the usage and inner workings of FFMPEGs APIs.

3. The source code of the Chromium project: http://src.chromium.org/viewvc/chrome/trunk/src/
  - Chromium is the basis for Google's Chrome web browser and uses FFMPEG libraries to implement HTML-5 audio and video playback. Chromium's source code is well documented and in wide use. This gives a good real life reference on how to use the FFMPEG-API's and some of its gotchas.
  - For video decoding a good starting point is the file media/filters/ffmpeg_video_decoder.cc  and within it the method FFmpegVideoDecoder::FFmpegDecode().
  - For audio decoding a good starting point is the file media/filters/ffmpeg_audio_decoder.cc and within it the method FFmpegAudioDecoder::FFmpegDecode().