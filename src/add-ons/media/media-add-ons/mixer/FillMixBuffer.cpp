#include "AudioMixer.h"
#include "IOStructures.h"

#include <media/RealtimeAlloc.h>
#include <media/Buffer.h>
#include <media/TimeSource.h>
#include <media/ParameterWeb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

status_t
AudioMixer::FillMixBuffer(void *outbuffer, size_t ioSize)
{
	
	int32 outChannels = fOutput.format.u.raw_audio.channel_count;

	// we have an output buffer - now it needs to be filled!

	switch (fOutput.format.u.raw_audio.format)
	{
		case media_raw_audio_format::B_AUDIO_FLOAT:
		{

			float *outdata = (float*)outbuffer;
							
			memset(outdata, 0, ioSize); // CHANGE_THIS
																								
			int sampleCount = int(fOutput.format.u.raw_audio.buffer_size / sizeof(float));
							
			for (int s = 0; s < sampleCount; s++)
			{
				outdata[s] = 0.0; // CHANGE_THIS
			}
			
			int mixerInputCount = fMixerInputs.CountItems();
							
			for (int c = 0; c < mixerInputCount; c++)
			{
				mixer_input *channel = (mixer_input *)fMixerInputs.ItemAt(c);
														
				if (channel->enabled == true) // still broken... FIX_THIS
				{
					int32 inChannels = channel->fInput.format.u.raw_audio.channel_count;
					bool split = outChannels > inChannels;
					bool mix = outChannels < inChannels;
																	
					if (fOutput.format.u.raw_audio.frame_rate == channel->fInput.format.u.raw_audio.frame_rate)
					{
						switch(channel->fInput.format.u.raw_audio.format) 
						{
							case media_raw_audio_format::B_AUDIO_FLOAT:
							{
								
								float *indata = (float *)channel->fData;
	
								if (split)
								{	} // ADD_THIS
								else if (mix)
								{	} // ADD_THIS
								else
								{
									int baseOffset = channel->fEventOffset / 4;
									int maxOffset = int(channel->fDataSize / 4);
								
									int offsetWrap = maxOffset - baseOffset;
											
									int inputSample = baseOffset;												
												
									for (int s = 0; s < sampleCount; s++)
									{
										
										outdata[s] = indata[inputSample];
										indata[inputSample] = 0;
										
										if (s == offsetWrap)
											inputSample = 0;
										else
											inputSample ++;
										
									}
																				
									channel->fEventOffset = (channel->fEventOffset + fOutput.format.u.raw_audio.buffer_size) % 
										channel->fDataSize;

								}
																								
								break;
							}
												
							case media_raw_audio_format::B_AUDIO_SHORT:
							{
												
								int16 *indata = (int16 *)channel->fData;
											
								if (split)
								{		}
								else if (mix)
								{		}
								else
								{
									
									int baseOffset = channel->fEventOffset / 2;
									int maxOffset = int(channel->fDataSize / 2);
								
									int offsetWrap = maxOffset - baseOffset;
											
									int inputSample = baseOffset;												
												
									for (int s = 0; s < sampleCount; s++)
									{																			
																							
										outdata[s] = outdata[s] + (indata[inputSample] / 32768.0); // CHANGE_THIS										indata[inputSample] = 0;
													
										if (s == offsetWrap)
											inputSample = 0;
										else
											inputSample ++;
														
									}
																				
									channel->fEventOffset = (channel->fEventOffset + fOutput.format.u.raw_audio.buffer_size / 2) % 
										channel->fDataSize;

								}
												
								break;
							}
																
						} // input format
											
					} // samplerate
									
				} // data available
													
			} // channel loop
								
			/* 
			// The buffer is done - we still need to scale for the MainVolume...
			// then check to see if anything is clipping, adjust if needed
			*/
			
			for (int frameStart = 0; frameStart < sampleCount; frameStart += outChannels)
			{
				for (int channel = 0; channel < outChannels; channel ++)
				{
					int sample = frameStart + channel;
					outdata[sample] = outdata[sample] * fMasterGainScale[channel];
					
					//if (outdata[sample] > 1.0)
				//		outdata[sample] = 1.0;
				//	else if (outdata[sample] < -1.0)
				//		outdata[sample] = -1.0;
												
				}
			}
			
			break;
														
		} // cur-case
						
		case media_raw_audio_format::B_AUDIO_SHORT:
		{

			int16 *outdata = (int16*)outbuffer;
			int sampleCount = int(fOutput.format.u.raw_audio.buffer_size / sizeof(int16));
							
			int *clipoffset = new int[sampleCount]; // keep a running tally of +/- clipping so we don't get rollaround distortion
													// we only need this for int/char audio types - not float
															
			memset(outdata, 0, ioSize);
			memset(clipoffset, 0, sizeof(int) * sampleCount);
			
		//	for (int s = 0; s < sampleCount; s++)
		//	{
		//		clipoffset[s] = 0;
		//	}
			
			int mixerInputs = fMixerInputs.CountItems();
										
			for (int c = 0; c < mixerInputs; c++)
			{
				mixer_input *channel = (mixer_input *)fMixerInputs.ItemAt(c);

				if (channel->enabled == true) // only use if there are buffers waiting (seems to be broken atm...)
				{
				
				//	if (channel->fProducerDataStatus == B_DATA_AVAILABLE)
				//		printf("B_DATA_AVAILABLE\n");
				//	else if (channel->fProducerDataStatus == B_DATA_NOT_AVAILABLE)
				//		printf("B_DATA_NOT_AVAILABLE\n");
				//	else if (channel->fProducerDataStatus == B_PRODUCER_STOPPED)
				//		printf("B_PRODUCER_STOPPED\n");			

					int32 inChannels = channel->fInput.format.u.raw_audio.channel_count;
					bool split = outChannels > inChannels;
					bool mix = outChannels < inChannels;
							
					if (fOutput.format.u.raw_audio.frame_rate == channel->fInput.format.u.raw_audio.frame_rate)
					{
						switch(channel->fInput.format.u.raw_audio.format) 
						{
											
							case media_raw_audio_format::B_AUDIO_FLOAT:
							{
									
								float *indata = (float *)channel->fData;
	
								if (split)
								{
								
									int baseOffset = channel->fEventOffset / 4;
									int maxOffset = int(channel->fDataSize / 4);
													
									int offsetWrap = maxOffset - baseOffset;
													
									int inputSample = baseOffset;											
									
									for (int s = 0; s < sampleCount; s += 2)
									{
																										
										if ((outdata[s] + int16(32767 * indata[inputSample])) > 32767)
											clipoffset[s] = clipoffset[s] + 1;
										else if ((outdata[s] + int16(32767 * indata[inputSample])) < -32768)
											clipoffset[s] = clipoffset[s] - 1;
													
										if ((outdata[s + 1] + int16(32767 * indata[inputSample])) > 32767)
											clipoffset[s + 1] = clipoffset[s + 1] + 1;
										else if ((outdata[s] + int16(32767 * indata[inputSample])) < -32768)
											clipoffset[s + 1] = clipoffset[s + 1] - 1;
												
										outdata[s] = outdata[s] + int16(32767 * indata[inputSample]); // CHANGE_THIS mixing
										outdata[s + 1] = outdata[s];
													
										indata[inputSample] = 0;
													
										if (s == offsetWrap)
											inputSample = 0;
										else
											inputSample ++;
									}
								
								}
								else if (mix)
								{}
								else
								{											
									int baseOffset = channel->fEventOffset / 4;
									int maxOffset = int(channel->fDataSize / 4);
								
									int offsetWrap = maxOffset - baseOffset;
											
									int inputSample = baseOffset;												
												
									for (int frameStart = 0; frameStart < sampleCount; frameStart += outChannels)
									{
										for (int chan = 0; chan < outChannels; chan ++)
										{
											int sample = frameStart + chan;
					
											int inputValue = (32767 * indata[inputSample] * channel->fGainScale[chan]) + outdata[sample];
																							
											if (inputValue > 32767)
												clipoffset[sample] = clipoffset[sample] + 1;
											else if (inputValue < -32768)
												clipoffset[sample] = clipoffset[sample] - 1;
													
											outdata[sample] = inputValue; // CHANGE_THIS
											indata[inputSample] = 0;
													
											if (sample == offsetWrap)
												inputSample = 0;
											else
												inputSample ++;
												
											}
															
									}
																				
									channel->fEventOffset += (fOutput.format.u.raw_audio.buffer_size * 2);
									if (channel->fEventOffset >= channel->fDataSize)
											channel->fEventOffset -= channel->fDataSize;
								
								}
																					
								break;
				
							}										
												
							case media_raw_audio_format::B_AUDIO_SHORT:
							{
											
								int16 *indata = (int16 *)channel->fData;
										
								if (split)
								{		}
								else if (mix)
								{		}
								else
								{					
									int baseOffset = channel->fEventOffset / 2;
									int maxOffset = int(channel->fDataSize / 2);
											
									int offsetWrap = maxOffset - baseOffset;
											
									int inputSample = baseOffset;											
										
									for (int frameStart = 0; frameStart < sampleCount; frameStart += outChannels)
									{
										for (int chan = 0; chan < outChannels; chan ++)
										{
											int sample = frameStart + chan;
					
											int clipTest = (outdata[sample] + (indata[inputSample] * channel->fGainScale[chan]));
																						
											if (clipTest > 32767)
												clipoffset[sample] = clipoffset[sample] + 1;
											else if (clipTest < -32768)
												clipoffset[sample] = clipoffset[sample] - 1;
														
											outdata[sample] = int16(outdata[sample] + (indata[inputSample] * channel->fGainScale[chan]));
												indata[inputSample] = 0;
														
											if (sample == offsetWrap)
												inputSample = 0;
											else
												inputSample ++;
														
										}
									}
													
										channel->fEventOffset = (channel->fEventOffset + fOutput.format.u.raw_audio.buffer_size);
										if (channel->fEventOffset >= channel->fDataSize)
											channel->fEventOffset -= channel->fDataSize;
									
									}
									
									break;
								}
											
								case media_raw_audio_format::B_AUDIO_INT:
								{
												
									int32 *indata = (int32 *)channel->fData;
											
									if (split)
									{		
												
										int baseOffset = channel->fEventOffset / 4;
										int maxOffset = int(channel->fDataSize / 4);
													
										int offsetWrap = maxOffset - baseOffset;
													
										int inputSample = baseOffset;											
										
										for (int s = 0; s < sampleCount; s += 2)
										{
																										
											if ((outdata[s] + (indata[inputSample] / 65536)) > 32767)
												clipoffset[s] = clipoffset[s] + 1;
											else if ((outdata[s] + (indata[inputSample] / 65536)) < -32768)
												clipoffset[s] = clipoffset[s] - 1;
													
											if ((outdata[s + 1] + (indata[inputSample] / 65536)) > 32767)
												clipoffset[s + 1] = clipoffset[s + 1] + 1;
											else if ((outdata[s + 1] + (indata[inputSample] / 65536)) < -32768)
												clipoffset[s + 1] = clipoffset[s + 1] - 1;
												
											outdata[s] = int16(outdata[s] + (indata[inputSample] / 65535)); // CHANGE_THIS mixing
											outdata[s + 1] = outdata[s];
														
											indata[inputSample] = 0;
													
											if (s == offsetWrap)
												inputSample = 0;
											else
												inputSample ++;
										}
													
									}
									else if (mix)
									{		}
									else
									{
												
										int baseOffset = channel->fEventOffset / 4;
										int maxOffset = int(channel->fDataSize / 4);
												
										int offsetWrap = maxOffset - baseOffset;
										
										int inputSample = baseOffset;											
											
										for (int s = 0; s < sampleCount; s++)
										{
																										
											if ((outdata[s] + (indata[inputSample] / 65536)) > 32767)
												clipoffset[s] = clipoffset[s] + 1;
											else if ((outdata[s] + (indata[inputSample] / 65536)) < -32768)
												clipoffset[s] = clipoffset[s] - 1;
														
											outdata[s] = int16(outdata[s] + (indata[inputSample] / 65536)); // CHANGE_THIS mixing
												indata[inputSample] = 0;
													
											if (s == offsetWrap)
												inputSample = 0;
											else
												inputSample ++;
															
										}
																										
									}
												
									channel->fEventOffset = (channel->fEventOffset + (fOutput.format.u.raw_audio.buffer_size * 2)) % 
										channel->fDataSize;
											
									break;
								}
																
							}
						}
								
					}
					
				}
									
		// use our clipoffset to determine correct limits
			for (int frameStart = 0; frameStart < sampleCount; frameStart += outChannels)
			{
				for (int channel = 0; channel < outChannels; channel ++)
				{
					int sample = frameStart + channel;
					
					int scaledSample = (outdata[sample] + (65536 * clipoffset[sample])) * fMasterGainScale[channel];
					
					if (scaledSample < -32768)
						outdata[sample] = -32768;
					else if (scaledSample > 32767)
						outdata[sample] = 32767;
					else
						outdata[sample] = scaledSample; //(int16)
				}
			}

			delete clipoffset;
								
		}
	
	}

	return B_OK;

}