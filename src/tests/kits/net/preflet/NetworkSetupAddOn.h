#ifndef NETWORKSETUPADDON_H
#define NETWORKSETUPADDON_H

#include <View.h>
#include <kernel/image.h>		// for image_id

class NetworkSetupProfile;

class NetworkSetupAddOn {
	public:
		NetworkSetupAddOn(image_id addon_image);
		virtual ~NetworkSetupAddOn();
		
		virtual BView *			CreateView(BRect *bounds);
		virtual status_t		Save(); 
		virtual status_t		Revert();
		
		virtual const char *	Name();
		// virtual const BBitmap *	Icon();

		virtual status_t		ProfileChanged(NetworkSetupProfile *new_profile);
		
		NetworkSetupProfile * 	Profile();
		bool 					IsDirty();
		void 					SetDirty(bool dirty = true);
		image_id				AddonImageId();
		
	private:
		bool 					is_dirty;
		NetworkSetupProfile *	profile;
		image_id				addon_image;
};

inline bool 					NetworkSetupAddOn::IsDirty() { return is_dirty; };
inline void 					NetworkSetupAddOn::SetDirty(bool dirty) { is_dirty = dirty; };
inline NetworkSetupProfile * 	NetworkSetupAddOn::Profile() { return profile; };
inline image_id					NetworkSetupAddOn::AddonImageId() { return addon_image; };

extern "C" {

#define NETWORK_SETUP_ADDON_INSTANCIATE_FUNC_NAME "get_nth_addon"
typedef NetworkSetupAddOn * (*network_setup_addon_instantiate)(image_id image, int index);

extern NetworkSetupAddOn * get_nth_addon(image_id image, int index);

}

#endif // ifdef NETWORKSETUPADDON_H

