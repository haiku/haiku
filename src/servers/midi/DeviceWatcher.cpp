/*
 * Copyright (c) 2003 Matthijs Hollemans
 * Copyright (c) 2003 Jerome Leveque
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
 * DEALINGS IN THE SOFTWARE.
 */

#include <Application.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>

#include "DeviceWatcher.h"
#include "PortDrivers.h"
#include "debug.h"

//------------------------------------------------------------------------------

DeviceWatcher::DeviceWatcher()
{
	largeIcon = new BBitmap(BRect(0, 0, 31, 31), B_CMAP8);
	miniIcon  = new BBitmap(BRect(0, 0, 15, 15), B_CMAP8);

	app_info info;
	be_app->GetAppInfo(&info);
	BFile file(&info.ref, B_READ_ONLY);

	BResources res;
	if (res.SetTo(&file) == B_OK)
	{
		size_t size;
		const void* bits;

		bits = res.LoadResource(B_LARGE_ICON_TYPE, 10, &size);
		largeIcon->SetBits(bits, size, 0, B_CMAP8);

		bits = res.LoadResource(B_MINI_ICON_TYPE, 11, &size);
		miniIcon->SetBits(bits, size, 0, B_CMAP8);
	}
}

//------------------------------------------------------------------------------

DeviceWatcher::~DeviceWatcher()
{
	delete largeIcon;
	delete miniIcon;
}

//------------------------------------------------------------------------------

void DeviceWatcher::Start()
{
	// We need to do this from a separate thread, otherwise we will deadlock.
	// The reason is that we instantiate a BMidiRoster object, which sends a
	// message to the midi_server to register itself, and blocks until it gets
	// a response. But since we _are_ the midi_server we will never be able to
	// send that response if our main thread is already blocking.

	resume_thread(spawn_thread(
		SpawnThread, "DeviceWatcher", B_NORMAL_PRIORITY, this));
}

//------------------------------------------------------------------------------

int32 DeviceWatcher::SpawnThread(void* data)
{
	((DeviceWatcher*) data)->ScanDevices("/dev/midi");
	return 0;
}

//------------------------------------------------------------------------------

void DeviceWatcher::ScanDevices(const char* path)
{
	BDirectory dir(path);
	if (dir.InitCheck() == B_OK)
	{
		BEntry entry;
		while (dir.GetNextEntry(&entry) == B_OK)
		{
			BPath name;
			entry.GetPath(&name);
			if (entry.IsDirectory())
			{
				ScanDevices(name.Path());
			}
			else
			{
				int fd = open(name.Path(), O_RDWR | O_EXCL);
				if (fd >= 0)
				{
					BMidiEndpoint* endp;

					endp = new MidiPortConsumer(fd, name.Path());
					SetIcons(endp);
					endp->Register();

					endp = new MidiPortProducer(fd, name.Path());
					SetIcons(endp);
					endp->Register();
				}
			}
		}
	}
}

//------------------------------------------------------------------------------

void DeviceWatcher::SetIcons(BMidiEndpoint* endp)
{
	BMessage msg;

	msg.AddData(
		"be:large_icon", B_LARGE_ICON_TYPE, largeIcon->Bits(), largeIcon->BitsLength());
	
	msg.AddData(
		"be:mini_icon", B_MINI_ICON_TYPE, miniIcon->Bits(), miniIcon->BitsLength());

	endp->SetProperties(&msg);
}

//------------------------------------------------------------------------------
