#ifndef NETWORKSETUPADDON_H
#define NETWORKSETUPADDON_H

#include <View.h>

class NetworkSetupProfile;

class NetworkSetupAddOn {
	public:
		NetworkSetupAddOn();
		virtual ~NetworkSetupAddOn();
		
		virtual BView *			CreateView(BRect *bounds);
		virtual status_t		Save(); 
		virtual status_t		Revert();
		
		virtual const char *	Name();

		status_t				ProfileChanged(NetworkSetupProfile *new_profile);
		NetworkSetupProfile * 	Profile();		
		bool 					IsDirty();
		void 					SetDirty(bool dirty = true);
		
	private:
		bool 					is_dirty;
		NetworkSetupProfile *	profile;
};

inline bool 					NetworkSetupAddOn::IsDirty() { return is_dirty; };
inline void 					NetworkSetupAddOn::SetDirty(bool dirty) { is_dirty = dirty; };
inline NetworkSetupProfile * 	NetworkSetupAddOn::Profile() { return profile; };

extern "C" {

typedef NetworkSetupAddOn * (*network_setup_addon_instantiate)(void);
NetworkSetupAddOn * get_addon(void);

}

#endif // ifdef NETWORKSETUPADDON_H

