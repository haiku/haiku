#ifndef NETWORKSETUPADDON_H
#define NETWORKSETUPADDON_H

#include <View.h>

class NetworkProfile;

class NetworkSetupAddOn {
	public:
		NetworkSetupAddOn();
		virtual ~NetworkSetupAddOn();
		
		virtual BView *		CreateView(BRect *bounds);
		virtual status_t	Save(); 
		virtual status_t	Revert();
		
		virtual const char *Name();


		status_t			SetProfile(NetworkProfile *new_profile);
		NetworkProfile * 	Profile();		
		bool 				IsDirty();
		void 				SetDirty(bool dirty = true);
		
	private:
		bool 				fIsDirty;
		NetworkProfile *	fProfile;
};

inline bool 			NetworkSetupAddOn::IsDirty() { return fIsDirty; };
inline void 			NetworkSetupAddOn::SetDirty(bool dirty) { fIsDirty = dirty; };
inline NetworkProfile * NetworkSetupAddOn::Profile() { return fProfile; };

extern "C" {

typedef NetworkSetupAddOn * (*network_setup_addon_instantiate)(void);
NetworkSetupAddOn * get_addon(void);

}

#endif // ifdef NETWORKSETUPADDON_H

