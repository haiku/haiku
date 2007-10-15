/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __CONTROLLER_H
#define __CONTROLLER_H


#include <Entry.h>
#include <MediaDefs.h>
#include <MediaNode.h>

class BDiscreteParameter;
class BParameterWeb;
class VideoView;
class VideoNode;

class Controller {
public:
							Controller();
	virtual 				~Controller();

	void					SetVideoView(VideoView *view);
	void					SetVideoNode(VideoNode *node);
	
	void					DisconnectInterface();
	status_t				ConnectInterface(int i);
	
	bool					IsInterfaceAvailable(int i);
	int						CurrentInterface();
	
	int						ChannelCount();
	int						CurrentChannel();
	status_t				SelectChannel(int i);
	const char *			ChannelName(int i);
	
	void					VolumeUp();
	void					VolumeDown();

private:
	status_t				ConnectNodes();
	status_t				DisconnectNodes();

private:
	int						fCurrentInterface;
	int						fCurrentChannel;
	VideoView *				fVideoView;
	VideoNode *				fVideoNode;
	BParameterWeb *			fWeb;
	BDiscreteParameter *	fChannelParam;
	bool					fConnected;
	media_input				fInput;
	media_output			fOutput;
};

#endif	// __CONTROLLER_H
