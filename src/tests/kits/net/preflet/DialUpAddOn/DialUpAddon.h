/* -----------------------------------------------------------------------
 * Copyright (c) 2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
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
 * ----------------------------------------------------------------------- */

/*!	\class DialUpAddon
	\brief Base class for DialUpPreflet add-ons.
	
	Dial-Up add-ons must export the following function: \n
	bool register(BMessage *addons) \n
	You should add your DialUpAddon object to the given BMessage. \n
	\n
	Most add-ons are simple pointers to a DialUpAddon object. \n
	\n
	Note for tab-addons: The BView object is deleted AFTER the DialUpAddon (except you
	remove and delete it in the DialUpAddon destructor).
*/

#ifndef _DIAL_UP_ADDON__H
#define _DIAL_UP_ADDON__H

#include <SupportDefs.h>
#include <Point.h>


#define DUN_MAXIMUM_PRIORITY			50

// add-on types
#define DUN_TAB_ADDON_TYPE				"Tab"
#define DUN_AUTHENTICATOR_ADDON_TYPE	"Authenticator"
#define DUN_DEVICE_ADDON_TYPE			"Device"
#define DUN_PROTOCOL_ADDON_TYPE			"Protocol"

// other information contained in the add-ons BMessage object
#define DUN_DELETE_ON_QUIT				"DeleteMe"
	// the DialUpAddon objects in this list will be deleted when the preflet quits
#define DUN_MESSENGER					"Messenger"
#define DUN_TAB_VIEW_RECT				"TabViewRect"
#define DUN_DEVICE_VIEW_WIDTH			"DeviceViewWidth"


class DialUpAddon {
		friend class DialUpView;

	public:
		//!	Constructor. The BMessage is the one passed to the \c register() function.
		DialUpAddon(BMessage *addons) : fAddons(addons) {}
		//!	Destructor. Does nothing.
		virtual ~DialUpAddon() {}
		
		//!	Returns the BMessage object holding all add-ons.
		BMessage *Addons() const
			{ return fAddons; }
		
		//!	Returns a name readable by humans without much technical knowledge.
		virtual const char *FriendlyName() const
			{ return NULL; }
		//!	Returns the technical name of this module.
		virtual const char *TechnicalName() const
			{ return NULL; }
		//!	Returns the name of the associated kernel module or \c NULL.
		virtual const char *KernelModuleName() const
			{ return NULL; }
		//!	Mostly used by tabs to describe where they should appear.
		virtual int32 Position() const
			{ return -1; }
		//!	Allows setting an order in which modules are asked to add the settings.
		virtual int32 Priority() const
			{ return 0; }
		
		/*!	\brief Load the given settings and profile.
			
			\param isNew Specifies if this is a newly created interface.
			
			\return \c true if loading was successful or \c false otherwise.
		*/
		virtual bool LoadSettings(BMessage *settings, BMessage *profile, bool isNew)
			{ return false; }
		/*!	\brief Returns if this module has a temporary profile.
			
			A temporary profile is never stored on the hard-disk, but only passed
			to the interface on connection. This can include passwords, for example.
		*/
		virtual bool HasTemporaryProfile() const
			{ return false; }
		//!	Are the settings or the profile modified?
		virtual void IsModified(bool *settings, bool *profile) const
			{ *settings = *profile = false; }
		/*!	\brief Save the given settings and profile.
			
			\param saveTemporary Specifies if the temporary profile should be written.
			
			\return \c true if saving was successful or \c false otherwise.
		*/
		virtual bool SaveSettings(BMessage *settings, BMessage *profile,
				bool saveTemporary)
			{ return false; }
		/*!	\brief Get the preferred view size.
			
			\return \c false if this module does not export a BView object.
		*/
		virtual bool GetPreferredSize(float *width, float *height) const
			{ return false; }
		/*!	\brief Returns the module's BView object.
			
			\param leftTop Specifies the view's left-top coordinates.
		*/
		virtual BView *CreateView(BPoint leftTop)
			{ return NULL; }

	private:
		virtual void _Reserved1() {}
		virtual void _Reserved2() {}
		virtual void _Reserved3() {}
		virtual void _Reserved4() {}

	private:
		BMessage *fAddons;
		
		int32 _reserved[7];
};


#endif
